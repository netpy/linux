// SPDX-License-Identifier: GPL-2.0
/*
 * f2fs compress support
 *
 * Copyright (c) 2019 Chao Yu <chao@kernel.org>
 */

#include <linux/fs.h>
#include <linux/f2fs_fs.h>
#include <linux/moduleparam.h>
#include <linux/writeback.h>
#include <linux/backing-dev.h>
#include <linux/lzo.h>
#include <linux/lz4.h>
#include <linux/zstd.h>
#include <linux/pagevec.h>

#include "f2fs.h"
#include "node.h"
#include "segment.h"
#include <trace/events/f2fs.h>

static struct kmem_cache *cic_entry_slab;
static struct kmem_cache *dic_entry_slab;

/* 
 * “为压缩集群分配 page 指针数组” 的 快速路径 + 回退路径 实现，
 * 优先走 per-SB slab，大数组才用 kmalloc，既省内存又避免碎片
 */
static void *page_array_alloc(struct inode *inode, int nr)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	unsigned int size = sizeof(struct page *) * nr;	/* 计算总字节数 */

	/* 若大小 ≤ per-SB slab 对象大小（默认 16×8=128 B）→ 走 slab 快速路径 */
	if (likely(size <= sbi->page_array_slab_size))
		return f2fs_kmem_cache_alloc(sbi->page_array_slab,
					GFP_F2FS_ZERO, false, F2FS_I_SB(inode));
	/* 大于 slab 对象（大 cluster）→ 回退到 kmalloc */
	return f2fs_kzalloc(sbi, size, GFP_NOFS);
}

/* 
 * “释放 page 指针数组” 的 对称实现——与 page_array_alloc 完全配对，
 * 小对象回 per-SB slab，大对象回 kmalloc，保证 零内存泄漏、零错配。
 */
static void page_array_free(struct inode *inode, void *pages, int nr)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	unsigned int size = sizeof(struct page *) * nr;	/* 计算总字节数 */

	if (!pages)
		return;	/* 空指针直接返回 */

	/* 若大小 ≤ per-SB slab 对象大小 → 走 slab 释放路径 */
	if (likely(size <= sbi->page_array_slab_size))
		kmem_cache_free(sbi->page_array_slab, pages);
	else
		kfree(pages);	/* 大于 slab 对象 → kfree 释放 */
}

struct f2fs_compress_ops {
	int (*init_compress_ctx)(struct compress_ctx *cc);
	void (*destroy_compress_ctx)(struct compress_ctx *cc);
	int (*compress_pages)(struct compress_ctx *cc);
	int (*init_decompress_ctx)(struct decompress_io_ctx *dic);
	void (*destroy_decompress_ctx)(struct decompress_io_ctx *dic);
	int (*decompress_pages)(struct decompress_io_ctx *dic);
	bool (*is_level_valid)(int level);
};

static unsigned int offset_in_cluster(struct compress_ctx *cc, pgoff_t index)
{
	return index & (cc->cluster_size - 1);
}

static pgoff_t cluster_idx(struct compress_ctx *cc, pgoff_t index)
{
	/* log_cluster_size 默认 4 → 16 页为一簇；*/
	return index >> cc->log_cluster_size;
}

static pgoff_t start_idx_of_cluster(struct compress_ctx *cc)
{
	return cc->cluster_idx << cc->log_cluster_size;
}

// 判定一页是否是压缩子页:三条件过滤 + magic 校验——四步确认压缩子页。
bool f2fs_is_compressed_page(struct page *page)
{
	/* 1. 必须有 PagePrivate 标记，否则肯定不是压缩页 */
	if (!PagePrivate(page))
		return false;
	/* 2. page->private 必须非空 */
	if (!page_private(page))
		return false;
	/* 3. 不能是“非指针”私有数据（例如 swap entry） */
	if (page_private_nonpointer(page))
		return false;
	/* 4. 私有数据必须指向 decompress_io_ctx，且头部 magic 正确 */
	f2fs_bug_on(F2FS_P_SB(page),
		*((u32 *)page_private(page)) != F2FS_COMPRESSED_PAGE_MAGIC);
	return true;
}

// 把压缩后的数据页和解密/解压所需的信息一起登记到页缓存里，方便以后读、写、解密时快速找到
/*
 * 把压缩后的数据页登记到页缓存，同时挂好解压/解密所需的私有数据。
 * 登记后，folio 以“集群首页索引”为键，整簇共享一个页缓存槽位。
 */
static void f2fs_set_compressed_page(struct page *page,
		struct inode *inode, pgoff_t index, void *data)
{
	/* 拿到所属 folio（压缩按 folio 粒度）*/
	struct folio *folio = page_folio(page);

	/* 挂私有数据：压缩头、集群号、iv 等 */
	// 这里传入的data，在压缩流程中就是cic,在解压流程中就是dic
	folio_attach_private(folio, (void *)data);

	/* i_crypto_info and iv index */
	/* 以下两行把 folio 重新定位到“集群首页索引”并绑定 inode 地址空间 */
	folio->index = index;	/* 集群首页索引（4 页对齐）*/
	folio->mapping = inode->i_mapping;	/* 挂靠到 inode 的地址空间 */
}

// “批量释放压缩上下文中的页引用/锁” 的最小工具函数，
// “把 cc->rpages[0..len-1] 里的页，要么解锁，要么减引用，空槽跳过。”
static void f2fs_drop_rpages(struct compress_ctx *cc, int len, bool unlock)
{
	int i;

	for (i = 0; i < len; i++) {
		if (!cc->rpages[i])
			continue;	/* 空槽跳过 */
		if (unlock)
			unlock_page(cc->rpages[i]);	 /* 解锁（第二次抓页后使用）*/
		else
			put_page(cc->rpages[i]);	 /* 减引用（第一次预读后使用）*/
	}
}

static void f2fs_put_rpages(struct compress_ctx *cc)
{
	f2fs_drop_rpages(cc, cc->cluster_size, false);
}

static void f2fs_unlock_rpages(struct compress_ctx *cc, int len)
{
	f2fs_drop_rpages(cc, len, true);
}

// “批量释放压缩集群页引用/锁” 的通用工具函数：
// “把 cc->rpages[0..15] 里的页，按需重新标记为 dirty，再统一解锁或减引用。”
static void f2fs_put_rpages_wbc(struct compress_ctx *cc,
		struct writeback_control *wbc, bool redirty, int unlock)
{
	unsigned int i;

	for (i = 0; i < cc->cluster_size; i++) {
		if (!cc->rpages[i])
			continue;	/* 空槽跳过 */

		/* 1. 若需要重新变脏 → 标记页为 dirty（用于回退场景）*/
		if (redirty)
			redirty_page_for_writepage(wbc, cc->rpages[i]);
		/* 2. 统一释放页：unlock=1 解锁，unlock=0 减引用*/
		f2fs_put_page(cc->rpages[i], unlock);
	}
}

struct folio *f2fs_compress_control_folio(struct folio *folio)
{
	struct compress_io_ctx *ctx = folio->private;

	return page_folio(ctx->rpages[0]);
}

/* “给压缩集群一次性分配 page 指针数组，后续攒页、压缩、写盘都靠它。”*/
int f2fs_init_compress_ctx(struct compress_ctx *cc)
{
	if (cc->rpages)	/* 如果已经分配过，直接返回（允许重复调用）*/
		return 0;
	/* 通过 per-SB slab 分配 cluster_size 个 page* 指针 */
	cc->rpages = page_array_alloc(cc->inode, cc->cluster_size);
	return cc->rpages ? 0 : -ENOMEM;	/* 成功返回 0，失败返回 -ENOMEM */
}

//  “释放压缩上下文所有数组并归零” 的最小工具函数：
// “把 rpages[]、cpages[] 数组全部回 slab/kmalloc，所有计数器清零；
// 若不再 reuse，把窗口索引也重置。”
void f2fs_destroy_compress_ctx(struct compress_ctx *cc, bool reuse)
{
	/* 1. 释放 rpages[] 数组（per-SB slab 或 kmalloc）*/
	page_array_free(cc->inode, cc->rpages, cc->cluster_size);
	cc->rpages = NULL;
	cc->nr_rpages = 0;	/* 已释放页数清零 */
	cc->nr_cpages = 0;	/* 2. 清零压缩相关计数器 */
	cc->valid_nr_cpages = 0;
	if (!reuse)	/* 3. 若不再 reuse（例如第一次预读后）→ 把窗口索引也重置 */
		cc->cluster_idx = NULL_CLUSTER;
}

// “把一页加入压缩集群上下文” 的最小实现，无锁、无内存分配，只做 三件事情：
// 断言：必须能合并（页号必须在当前 16 页窗口内）；
// 计算页在窗口内的偏移 → 直接塞进 rpages[]；
// 更新计数器和窗口索引。
void f2fs_compress_ctx_add_page(struct compress_ctx *cc, struct folio *folio)
{
	unsigned int cluster_ofs;

	/* 断言：必须能合并（页号必须在当前 16 页窗口内）*/
	if (!f2fs_cluster_can_merge_page(cc, folio->index))
		f2fs_bug_on(F2FS_I_SB(cc->inode), 1);

	/* 计算页在 16 页窗口内的偏移（0-15）*/
	cluster_ofs = offset_in_cluster(cc, folio->index);
	/* 把 folio 的首页指针塞进 rpages[偏移]*/
	cc->rpages[cluster_ofs] = folio_page(folio, 0);
	/* 更新计数器和当前窗口索引*/
	cc->nr_rpages++;
	cc->cluster_idx = cluster_idx(cc, folio->index);
}

#ifdef CONFIG_F2FS_FS_LZO
//  LZO 压缩算法初始化压缩上下文 的实现：为 LZO 压缩分配内存缓冲区，并计算压缩后最大长度。
static int lzo_init_compress_ctx(struct compress_ctx *cc)
{
	cc->private = f2fs_vmalloc(F2FS_I_SB(cc->inode),	/* 为 LZO 压缩分配工作内存 */
					LZO1X_MEM_COMPRESS);
	if (!cc->private)	/* 分配失败返回 -ENOMEM */
		return -ENOMEM;

	/* 计算压缩后最大长度（最坏情况） */
	cc->clen = lzo1x_worst_compress(PAGE_SIZE << cc->log_cluster_size);
	return 0;	/* 成功返回 0 */
}

//  LZO 压缩算法销毁压缩上下文 的最简实现：
// “释放初始化时分配的 LZO 工作内存，指针清零。”
static void lzo_destroy_compress_ctx(struct compress_ctx *cc)
{
	vfree(cc->private);	/* 释放 vmalloc 的工作内存 */
	cc->private = NULL;	/* 指针清零，防止野指针 */
}

// LZO 压缩算法执行压缩操作 的实现：
// “使用 LZO 算法压缩输入缓冲区 cc->rbuf 到输出缓冲区 cc->cbuf->cdata，并更新压缩后长度 cc->clen。”
static int lzo_compress_pages(struct compress_ctx *cc)
{
	int ret;
	/* 调用 LZO 压缩函数 */
	// cc->cbuf->cdata：输出缓冲区，用于存放压缩后的数据。
	ret = lzo1x_1_compress(cc->rbuf, cc->rlen, cc->cbuf->cdata,
					&cc->clen, cc->private);
	if (ret != LZO_E_OK) {
		/* 压缩失败，记录错误 */
		f2fs_err_ratelimited(F2FS_I_SB(cc->inode),
				"lzo compress failed, ret:%d", ret);
		return -EIO;	/* 返回 I/O 错误 */
	}
	return 0;	/* 成功返回 0 */
}

// LZO 解压缩算法执行解压缩操作 的最简实现：
// “用 LZO 把 15 页压缩数据解压成 16 页原始数据，失败或长度不符就报错。”
static int lzo_decompress_pages(struct decompress_io_ctx *dic)
{
	int ret;

	/* 1. 一次性解压：压缩缓冲区 → 原始缓冲区 */
	ret = lzo1x_decompress_safe(dic->cbuf->cdata, dic->clen,
						dic->rbuf, &dic->rlen);
	if (ret != LZO_E_OK) {	 /* 解压失败 */
		f2fs_err_ratelimited(F2FS_I_SB(dic->inode),
				"lzo decompress failed, ret:%d", ret);
		return -EIO;	/* 返回 I/O 错误 */
	}

	/* 2. 校验解压后长度必须等于 16 页（64 KB）*/
	if (dic->rlen != PAGE_SIZE << dic->log_cluster_size) {
		f2fs_err_ratelimited(F2FS_I_SB(dic->inode),
				"lzo invalid rlen:%zu, expected:%lu",
				dic->rlen, PAGE_SIZE << dic->log_cluster_size);
		return -EIO;	/* 长度不符 → I/O 错误 */
	}
	return 0;	/* 成功返回 0 */
}

static const struct f2fs_compress_ops f2fs_lzo_ops = {
	.init_compress_ctx	= lzo_init_compress_ctx,
	.destroy_compress_ctx	= lzo_destroy_compress_ctx,
	.compress_pages		= lzo_compress_pages,
	.decompress_pages	= lzo_decompress_pages,
};
#endif

#ifdef CONFIG_F2FS_FS_LZ4
// LZ4 压缩算法初始化压缩上下文 的实现：
// “根据配置选择 LZ4 或 LZ4HC 压缩算法，分配工作内存，并设置压缩后最大长度。”
static int lz4_init_compress_ctx(struct compress_ctx *cc)
{
	unsigned int size = LZ4_MEM_COMPRESS;	 /* 默认 LZ4 压缩算法的工作内存大小 */

#ifdef CONFIG_F2FS_FS_LZ4HC
	if (F2FS_I(cc->inode)->i_compress_level)	/* 如果启用了 LZ4HC（高压缩比）且压缩级别非零 */
		size = LZ4HC_MEM_COMPRESS;	/* 使用 LZ4HC 的工作内存大小 */
#endif

	cc->private = f2fs_vmalloc(F2FS_I_SB(cc->inode), size);	/* 分配工作内存 */
	if (!cc->private)
		return -ENOMEM;	/* 分配失败返回 -ENOMEM */

	/*
	 * we do not change cc->clen to LZ4_compressBound(inputsize) to
	 * adapt worst compress case, because lz4 compressor can handle
	 * output budget properly.
	 */
	/*
	 * 我们不将 cc->clen 设置为 LZ4_compressBound(inputsize) 来适应最坏压缩情况，
	 * 因为 LZ4 压缩器可以正确处理输出预算。
	 */
	cc->clen = cc->rlen - PAGE_SIZE - COMPRESS_HEADER_SIZE;
	return 0;
}

// LZ4 压缩算法销毁压缩上下文 的最简实现：
// “释放初始化时分配的 LZ4/LZ4HC 工作内存，指针清零。”
static void lz4_destroy_compress_ctx(struct compress_ctx *cc)
{
	vfree(cc->private);	/* 释放 vmalloc 的工作内存 */
	cc->private = NULL;	/* 指针清零，防止野指针 */
}

// LZ4 压缩算法执行压缩操作 的实现：
// “根据配置选择 LZ4 或 LZ4HC 压缩算法，
// 压缩输入缓冲区 cc->rbuf 到输出缓冲区 cc->cbuf->cdata，并更新压缩后长度 cc->clen。”
static int lz4_compress_pages(struct compress_ctx *cc)
{
	int len = -EINVAL;	/* 初始化返回值为无效值 */
	unsigned char level = F2FS_I(cc->inode)->i_compress_level;	/* 获取压缩级别 */

	if (!level)	/* 如果未设置压缩级别，使用默认的 LZ4 压缩 */
		len = LZ4_compress_default(cc->rbuf, cc->cbuf->cdata, cc->rlen,
						cc->clen, cc->private);
#ifdef CONFIG_F2FS_FS_LZ4HC
	else	/* 如果启用了 LZ4HC 并且压缩级别非零，使用 LZ4HC 压缩 */
		len = LZ4_compress_HC(cc->rbuf, cc->cbuf->cdata, cc->rlen,
					cc->clen, level, cc->private);
#endif
	if (len < 0)	/* 检查压缩结果 */
		return len;	/* 压缩失败，返回错误码 */
	if (!len)	/* 压缩后长度为 0，返回 -EAGAIN 表示无压缩效果 */
		return -EAGAIN;

	cc->clen = len;	/* 更新压缩后长度 */
	return 0;	/* 成功返回 0 */
}

// LZ4 解压缩算法执行解压缩操作 的最简实现：
// “用 LZ4 把 15 页压缩数据解压成 16 页原始数据，长度不符或失败就报错。”
static int lz4_decompress_pages(struct decompress_io_ctx *dic)
{
	int ret;

	/* 1. 一次性解压：压缩缓冲区 → 原始缓冲区 */
	ret = LZ4_decompress_safe(dic->cbuf->cdata, dic->rbuf,
						dic->clen, dic->rlen);
	if (ret < 0) {	/* 解压失败 */
		f2fs_err_ratelimited(F2FS_I_SB(dic->inode),
				"lz4 decompress failed, ret:%d", ret);
		return -EIO;	/* 返回 I/O 错误 */
	}

	/* 2. 校验解压后长度必须等于 16 页（64 KB）*/
	if (ret != PAGE_SIZE << dic->log_cluster_size) {
		f2fs_err_ratelimited(F2FS_I_SB(dic->inode),
				"lz4 invalid ret:%d, expected:%lu",
				ret, PAGE_SIZE << dic->log_cluster_size);
		return -EIO;	/* 长度不符 → I/O 错误 */
	}
	return 0;	/* 成功返回 0 */
}

static bool lz4_is_level_valid(int lvl)
{
#ifdef CONFIG_F2FS_FS_LZ4HC
	return !lvl || (lvl >= LZ4HC_MIN_CLEVEL && lvl <= LZ4HC_MAX_CLEVEL);
#else
	return lvl == 0;
#endif
}

static const struct f2fs_compress_ops f2fs_lz4_ops = {
	.init_compress_ctx	= lz4_init_compress_ctx,
	.destroy_compress_ctx	= lz4_destroy_compress_ctx,
	.compress_pages		= lz4_compress_pages,
	.decompress_pages	= lz4_decompress_pages,
	.is_level_valid		= lz4_is_level_valid,
};
#endif

#ifdef CONFIG_F2FS_FS_ZSTD
// Zstandard (zstd) 压缩算法初始化压缩上下文 的实现：
// “根据压缩级别初始化 Zstd 压缩流，分配工作内存，并设置压缩后最大长度。”
static int zstd_init_compress_ctx(struct compress_ctx *cc)
{
	zstd_parameters params;	/* Zstd 参数结构 */
	zstd_cstream *stream;	/* Zstd 压缩流句柄 */
	void *workspace;	/* 工作内存 */
	unsigned int workspace_size;	/* 工作内存大小 */
	unsigned char level = F2FS_I(cc->inode)->i_compress_level;	/* 压缩级别 */

	/* Need to remain this for backward compatibility */
	if (!level)	/* 如果未设置压缩级别，使用默认值 */
		level = F2FS_ZSTD_DEFAULT_CLEVEL;
	/* 根据压缩级别和输入长度获取 Zstd 参数 */
	params = zstd_get_params(level, cc->rlen);
	/* 计算所需工作内存大小 */
	workspace_size = zstd_cstream_workspace_bound(&params.cParams);
	/* 分配工作内存 */
	workspace = f2fs_vmalloc(F2FS_I_SB(cc->inode), workspace_size);
	if (!workspace)
		return -ENOMEM;	/* 分配失败返回 -ENOMEM */

	/* 初始化 Zstd 压缩流 */
	stream = zstd_init_cstream(&params, 0, workspace, workspace_size);
	if (!stream) {
		f2fs_err_ratelimited(F2FS_I_SB(cc->inode),
				"%s zstd_init_cstream failed", __func__);
		vfree(workspace);	/* 释放工作内存 */
		return -EIO;	/* 初始化失败返回 -EIO */
	}
	/* 保存工作内存和压缩流句柄到压缩上下文 */
	cc->private = workspace;
	cc->private2 = stream;

	/* 设置压缩后最大长度 */
	cc->clen = cc->rlen - PAGE_SIZE - COMPRESS_HEADER_SIZE;
	return 0;	/* 成功返回 0 */
}

// Zstandard（zstd）压缩算法销毁压缩上下文 的实现：
// “释放初始化时分配的 Zstd 工作内存和压缩流句柄，指针全部清零。”
static void zstd_destroy_compress_ctx(struct compress_ctx *cc)
{
	vfree(cc->private);	/* 释放 vmalloc 的工作内存（zstd 工作区）*/
	cc->private = NULL;	/* 清零工作内存指针 */
	cc->private2 = NULL;	/* 清零压缩流句柄指针（zstd_cstream *）*/
}

//  Zstandard (zstd) 压缩算法执行压缩操作 的实现：
// “使用 Zstd 流式压缩接口，把输入缓冲区 cc->rbuf 压缩到输出缓冲区 cc->cbuf->cdata，并返回压缩后长度。”
// “Zstd 流式压缩：一次性压完 16 页 → 刷剩余 → 缓冲区不足就回退原样写。”
static int zstd_compress_pages(struct compress_ctx *cc)
{
	zstd_cstream *stream = cc->private2;	/* 取出初始化时保存的 zstd 压缩流句柄 */
	zstd_in_buffer inbuf;	 /* 输入缓冲区描述符 */
	zstd_out_buffer outbuf;	 /* 输出缓冲区描述符 */
	int src_size = cc->rlen;	/* 输入数据总长度 */
	int dst_size = src_size - PAGE_SIZE - COMPRESS_HEADER_SIZE;	/* 输出缓冲区最大长度（15 页 - 头）*/
	int ret;

	/* 1. 填充输入缓冲区描述符 */
	inbuf.pos = 0;
	inbuf.src = cc->rbuf;	/* 输入数据起始地址 */
	inbuf.size = src_size;	/* 输入数据长度 */

	/* 2. 填充输出缓冲区描述符 */
	outbuf.pos = 0;
	outbuf.dst = cc->cbuf->cdata;	/* 输出数据起始地址 */
	outbuf.size = dst_size;	/* 输出缓冲区最大长度 */

	/* 3. 流式压缩：一次性压缩全部输入数据 */
	ret = zstd_compress_stream(stream, &outbuf, &inbuf);
	if (zstd_is_error(ret)) {	/* 压缩失败 */
		f2fs_err_ratelimited(F2FS_I_SB(cc->inode),
				"%s zstd_compress_stream failed, ret: %d",
				__func__, zstd_get_error_code(ret));
		return -EIO;	/* 返回 I/O 错误 */
	}

	/* 4. 结束压缩流（刷出剩余数据）*/
	ret = zstd_end_stream(stream, &outbuf);
	if (zstd_is_error(ret)) {	/* 结束失败 */
		f2fs_err_ratelimited(F2FS_I_SB(cc->inode),
				"%s zstd_end_stream returned %d",
				__func__, zstd_get_error_code(ret));
		return -EIO;	/* 返回 I/O 错误 */
	}

	/*
	 * there is compressed data remained in intermediate buffer due to
	 * no more space in cbuf.cdata
	 */
	// 5. 若仍有数据未刷出（输出缓冲区不足）→ 视为膨胀，回退原样写
	if (ret)
		return -EAGAIN;	/* 膨胀信号 → 上层回退原样写 */

	/* 6. 成功：记录实际压缩后长度 */
	cc->clen = outbuf.pos;	/* 成功返回 0 */
	return 0;
}

// 给 zstd 解压分配并初始化流上下文
// 算大小 → 申请 workspace → 初始化 dstream → 挂到 dic 私有，三步完成 zstd 解压环境搭建。
static int zstd_init_decompress_ctx(struct decompress_io_ctx *dic)
{
	zstd_dstream *stream;	/* zstd 解压流句柄 */
	void *workspace;	/* zstd 工作内存 */
	unsigned int workspace_size;	 /* 所需工作内存大小 */
	unsigned int max_window_size =	/* 按簇大小算窗口上限 */
			MAX_COMPRESS_WINDOW_SIZE(dic->log_cluster_size);

	/* 1. 计算 zstd 解压所需工作区大小 */
	workspace_size = zstd_dstream_workspace_bound(max_window_size);
	/* 2. 申请工作区（vmalloc 保证大页也能满足） */
	workspace = f2fs_vmalloc(F2FS_I_SB(dic->inode), workspace_size);
	if (!workspace)
		return -ENOMEM;
	/* 3. 初始化 dstream；失败则释放内存并报错 */
	stream = zstd_init_dstream(max_window_size, workspace, workspace_size);
	if (!stream) {
		f2fs_err_ratelimited(F2FS_I_SB(dic->inode),
				"%s zstd_init_dstream failed", __func__);
		vfree(workspace);
		return -EIO;
	}
	/* 4. 把 workspace 和 stream 挂到 dic 私有指针，供后续解压使用 */
	dic->private = workspace;
	dic->private2 = stream;

	return 0;	/* 成功 */
}

// 释放 zstd 解压私有内存
static void zstd_destroy_decompress_ctx(struct decompress_io_ctx *dic)
{
	vfree(dic->private);	/* 释放 zstd_init_decompress_ctx 里 vmalloc 的 workspace */
	dic->private = NULL;	/* 清指针防悬垂 */
	dic->private2 = NULL;	/* private2 指向的 dstream 句柄随 workspace 一起失效 */
}

// Zstandard（zstd）解压缩算法执行解压缩操作 的实现：
// “用 Zstd 流式解压接口，把 15 页压缩数据解压成 16 页原始数据，失败或长度不符就报错。”
static int zstd_decompress_pages(struct decompress_io_ctx *dic)
{
	zstd_dstream *stream = dic->private2;	/* 取出初始化时保存的 zstd 解压流句柄 */
	zstd_in_buffer inbuf;	/* 输入缓冲区描述符 */
	zstd_out_buffer outbuf;	/* 输出缓冲区描述符 */
	int ret;

	/* 1. 填充输入缓冲区描述符 */
	inbuf.pos = 0;
	inbuf.src = dic->cbuf->cdata;	/* 压缩数据起始地址 */
	inbuf.size = dic->clen;	/* 压缩数据长度 */

	/* 2. 填充输出缓冲区描述符 */
	outbuf.pos = 0;
	outbuf.dst = dic->rbuf;	/* 解压目标地址 */
	outbuf.size = dic->rlen;	/* 解压目标长度（16 页）*/

	/* 3. 流式解压：一次性解压全部输入数据 */
	ret = zstd_decompress_stream(stream, &outbuf, &inbuf);
	if (zstd_is_error(ret)) {	/* 解压失败 */
		f2fs_err_ratelimited(F2FS_I_SB(dic->inode),
				"%s zstd_decompress_stream failed, ret: %d",
				__func__, zstd_get_error_code(ret));
		return -EIO;	/* 返回 I/O 错误 */
	}

	/* 4. 校验解压后长度必须等于 16 页（64 KB）*/
	if (dic->rlen != outbuf.pos) {
		f2fs_err_ratelimited(F2FS_I_SB(dic->inode),
				"%s ZSTD invalid rlen:%zu, expected:%lu",
				__func__, dic->rlen,
				PAGE_SIZE << dic->log_cluster_size);
		return -EIO;	 /* 长度不符 → I/O 错误 */
	}

	return 0;	/* 成功返回 0 */
}

static bool zstd_is_level_valid(int lvl)
{
	return lvl >= zstd_min_clevel() && lvl <= zstd_max_clevel();
}

static const struct f2fs_compress_ops f2fs_zstd_ops = {
	.init_compress_ctx	= zstd_init_compress_ctx,
	.destroy_compress_ctx	= zstd_destroy_compress_ctx,
	.compress_pages		= zstd_compress_pages,
	.init_decompress_ctx	= zstd_init_decompress_ctx,
	.destroy_decompress_ctx	= zstd_destroy_decompress_ctx,
	.decompress_pages	= zstd_decompress_pages,
	.is_level_valid		= zstd_is_level_valid,
};
#endif

#ifdef CONFIG_F2FS_FS_LZO
#ifdef CONFIG_F2FS_FS_LZORLE
//  LZO-RLE 压缩算法执行压缩操作 的最简实现：
// “调用 LZO-RLE 压缩函数，把 16 页原始数据压到输出缓冲区，失败就报错的纯包装器。”
static int lzorle_compress_pages(struct compress_ctx *cc)
{
	int ret;
	/* 直接调用 LZO-RLE 压缩函数 */
	ret = lzorle1x_1_compress(cc->rbuf, cc->rlen, cc->cbuf->cdata,
					&cc->clen, cc->private);
	if (ret != LZO_E_OK) {	/* 压缩失败 */
		f2fs_err_ratelimited(F2FS_I_SB(cc->inode),
				"lzo-rle compress failed, ret:%d", ret);
		return -EIO;	/* 返回 I/O 错误 */
	}
	return 0;	/* 成功返回 0 */
}

static const struct f2fs_compress_ops f2fs_lzorle_ops = {
	.init_compress_ctx	= lzo_init_compress_ctx,
	.destroy_compress_ctx	= lzo_destroy_compress_ctx,
	.compress_pages		= lzorle_compress_pages,
	.decompress_pages	= lzo_decompress_pages,
};
#endif
#endif

static const struct f2fs_compress_ops *f2fs_cops[COMPRESS_MAX] = {
#ifdef CONFIG_F2FS_FS_LZO
	&f2fs_lzo_ops,
#else
	NULL,
#endif
#ifdef CONFIG_F2FS_FS_LZ4
	&f2fs_lz4_ops,
#else
	NULL,
#endif
#ifdef CONFIG_F2FS_FS_ZSTD
	&f2fs_zstd_ops,
#else
	NULL,
#endif
#if defined(CONFIG_F2FS_FS_LZO) && defined(CONFIG_F2FS_FS_LZORLE)
	&f2fs_lzorle_ops,
#else
	NULL,
#endif
};

// 压缩后端是否就绪
bool f2fs_is_compress_backend_ready(struct inode *inode)
{
	if (!f2fs_compressed_file(inode))	/* 文件没开压缩 → 算就绪 */
		return true;
	/* 有压缩：看算法对应的 f2fs_compress_ops 是否已注册 */
	return f2fs_cops[F2FS_I(inode)->i_compress_algorithm];
}

bool f2fs_is_compress_level_valid(int alg, int lvl)
{
	const struct f2fs_compress_ops *cops = f2fs_cops[alg];

	if (cops->is_level_valid)
		return cops->is_level_valid(lvl);

	return lvl == 0;
}

static mempool_t *compress_page_pool;	/* 压缩页池全局变量 */
static int num_compress_pages = 512;	/* 默认预分配的压缩页数量 */
module_param(num_compress_pages, uint, 0444);	/* 允许用户通过模块参数调整预分配数量 */
MODULE_PARM_DESC(num_compress_pages,
		"Number of intermediate compress pages to preallocate");

/* 初始化压缩页池 */
int __init f2fs_init_compress_mempool(void)
{
	/* 创建页池，预分配 num_compress_pages 个页 */
	compress_page_pool = mempool_create_page_pool(num_compress_pages, 0);
	return compress_page_pool ? 0 : -ENOMEM;	/* 成功返回 0，失败返回 -ENOMEM */
}

/* 销毁压缩页池 */
void f2fs_destroy_compress_mempool(void)
{
	mempool_destroy(compress_page_pool);	/* 销毁页池 */
}

// “从 F2FS 的压缩页池里分配一页并加锁” 的实现：
// “从压缩页池里分配一页，加锁后返回。”
static struct page *f2fs_compress_alloc_page(void)
{
	struct page *page;
	/* 从压缩页池里分配一页 */
	// 使用 GFP_NOFS 标志分配内存，表示在文件系统操作中分配内存时避免触发文件系统操作，防止死锁。
	page = mempool_alloc(compress_page_pool, GFP_NOFS);
	lock_page(page);	/* 加锁 */

	return page;	/* 返回加锁后的页 */
}

// “把压缩临时页归还给 F2FS 压缩页池” 的底层实现：
// “解除页的所有关联，解锁，然后放回压缩页池，供下次复用。”
static void f2fs_compress_free_page(struct page *page)
{
	struct folio *folio;

	if (!page)
		return;	/* 空指针直接返回 */
	folio = page_folio(page);	/* 拿到 folio 句柄 */
	/* 1. 解除 folio 的 private 标记（压缩临时标记）*/
	folio_detach_private(folio);
	/* 2. 解除地址空间映射（防止 page cache 误用）*/
	folio->mapping = NULL;
	/* 3. 解锁页（必须解锁后才能归还池）*/
	folio_unlock(folio);
	/* 4. 把页放回压缩页池（mempool）*/
	mempool_free(page, compress_page_pool);
}

#define MAX_VMAP_RETRIES	3

// “将一组页映射到连续的虚拟内存空间” 的实现：
// “尝试将 count 个页映射到连续的虚拟内存，失败则重试，最多重试 MAX_VMAP_RETRIES 次。”
static void *f2fs_vmap(struct page **pages, unsigned int count)
{
	int i;
	void *buf = NULL;

	for (i = 0; i < MAX_VMAP_RETRIES; i++) {	/* 尝试多次映射，直到成功或达到最大重试次数 */
		buf = vm_map_ram(pages, count, -1);	/* 尝试将 pages[] 中的 count 个页映射到连续虚拟内存 */
		if (buf)
			break;	/* 映射成功，退出循环 */
		vm_unmap_aliases();	/* 映射失败，清理别名 */
	}
	return buf;	/* 返回映射后的虚拟地址，失败返回 NULL */
}

/*
 * F2FS 压缩核心函数：
 * 把 16 页原始数据压成 < 16 页，填头、校验、补 0，准备落盘：
 * “vmap 原始页 → 算法压 → vmap 压缩页 → 填头/校验/补0 → 多余页释放”
 * 成功返回 0，膨胀或错误返回负值，上层回退原样写。
 */
static int f2fs_compress_pages(struct compress_ctx *cc)
{
	struct f2fs_inode_info *fi = F2FS_I(cc->inode);
	/* 根据 inode 标记选择压缩算法（LZ4/ZSTD）*/
	const struct f2fs_compress_ops *cops =
				f2fs_cops[fi->i_compress_algorithm];
	unsigned int max_len, new_nr_cpages;	/* 压缩后最大长度 & 实际页数 */
	u32 chksum = 0;	/* 校验和 */
	int i, ret;
	/* trace 点：开始压缩 */
	trace_f2fs_compress_pages_start(cc->inode, cc->cluster_idx,
				cc->cluster_size, fi->i_compress_algorithm);
	/* 算法私有初始化（LZ4/ZSTD 句柄等）*/
	if (cops->init_compress_ctx) {
		ret = cops->init_compress_ctx(cc);
		if (ret)
			goto out;	/* 初始化失败 → 跳出 */
	}
	/* 计算压缩后最大长度：头 + 原始数据 - 1 页（保证压后 ≤ 15 页）*/
	max_len = COMPRESS_HEADER_SIZE + cc->clen;
	/* 向上取整到页数 → 最多 15 页 */
	cc->nr_cpages = DIV_ROUND_UP(max_len, PAGE_SIZE);
	cc->valid_nr_cpages = cc->nr_cpages;	/* 当前有效页数 */
	/* 为压缩后数据分配页数组（per-SB slab）*/
	cc->cpages = page_array_alloc(cc->inode, cc->nr_cpages);
	if (!cc->cpages) {
		ret = -ENOMEM;
		goto destroy_compress_ctx;	/* 内存失败 → 清理上下文 */
	}
	/* 逐页分配物理页（kmalloc 压缩页）*/
	for (i = 0; i < cc->nr_cpages; i++)
		cc->cpages[i] = f2fs_compress_alloc_page();
	/* vmap：把 16 页原始页映射到连续内核虚拟地址（输入缓冲区）*/
	cc->rbuf = f2fs_vmap(cc->rpages, cc->cluster_size);
	if (!cc->rbuf) {
		ret = -ENOMEM;
		goto out_free_cpages;	/* 映射失败 → 清理已分配页 */
	}
	/* vmap：把压缩后页映射到连续内核虚拟地址（输出缓冲区）*/
	cc->cbuf = f2fs_vmap(cc->cpages, cc->nr_cpages);
	if (!cc->cbuf) {
		ret = -ENOMEM;
		goto out_vunmap_rbuf;	/* 映射失败 → 清理 */
	}
	/* 真正压缩：算法返回压缩后长度到 cc->clen */
	ret = cops->compress_pages(cc);
	if (ret)
		goto out_vunmap_cbuf;	/* 压缩失败 → 清理 */
	/* 最大允许长度：15 页 - 头（保证压后 ≤ 15 页）*/
	max_len = PAGE_SIZE * (cc->cluster_size - 1) - COMPRESS_HEADER_SIZE;
	/* 若压后长度 > 15 页 - 头 → 视为膨胀，回退原样写 */
	if (cc->clen > max_len) {
		ret = -EAGAIN;	/* 膨胀信号 → 上层会回退原样写 */
		goto out_vunmap_cbuf;
	}
	/* 填头：压缩后长度（小端）*/
	cc->cbuf->clen = cpu_to_le32(cc->clen);
	/* 若 inode 开启校验和 → 计算 CRC32 并填头 */
	if (fi->i_compress_flag & BIT(COMPRESS_CHKSUM))
		chksum = f2fs_crc32(cc->cbuf->cdata, cc->clen);
	cc->cbuf->chksum = cpu_to_le32(chksum);
	/* 保留字段清零（向后兼容）*/
	for (i = 0; i < COMPRESS_DATA_RESERVED_SIZE; i++)
		cc->cbuf->reserved[i] = cpu_to_le32(0);
	/* 重新计算实际占用页数（压后长度 + 头）*/
	new_nr_cpages = DIV_ROUND_UP(cc->clen + COMPRESS_HEADER_SIZE, PAGE_SIZE);

	/* zero out any unused part of the last page */
	/* 把最后一页未用部分清零（防止信息泄露）*/
	memset(&cc->cbuf->cdata[cc->clen], 0,
			(new_nr_cpages * PAGE_SIZE) -
			(cc->clen + COMPRESS_HEADER_SIZE));
	/* 解除 vmap 映射（输入/输出缓冲区）*/
	vm_unmap_ram(cc->cbuf, cc->nr_cpages);
	vm_unmap_ram(cc->rbuf, cc->cluster_size);
	/* 释放多余的压缩页（压后 < 15 页时）*/
	for (i = new_nr_cpages; i < cc->nr_cpages; i++) {
		f2fs_compress_free_page(cc->cpages[i]);
		cc->cpages[i] = NULL;
	}
	/* 算法私有清理（ZSTD 句柄等）*/
	if (cops->destroy_compress_ctx)
		cops->destroy_compress_ctx(cc);
	/* 最终有效页数 = 实际占用页数 */
	cc->valid_nr_cpages = new_nr_cpages;

	trace_f2fs_compress_pages_end(cc->inode, cc->cluster_idx,
							cc->clen, ret);
	return 0;	/* 成功返回 */

/* ===== 错误清理路径 ===== */
out_vunmap_cbuf:
	vm_unmap_ram(cc->cbuf, cc->nr_cpages);
out_vunmap_rbuf:
	vm_unmap_ram(cc->rbuf, cc->cluster_size);
out_free_cpages:
	/* 释放所有已分配的压缩页 */
	for (i = 0; i < cc->nr_cpages; i++) {
		if (cc->cpages[i])
			f2fs_compress_free_page(cc->cpages[i]);
	}
	/* 释放压缩页数组对象（per-SB slab）*/
	page_array_free(cc->inode, cc->cpages, cc->nr_cpages);
	cc->cpages = NULL;
destroy_compress_ctx:
	/* 算法私有清理 */
	if (cops->destroy_compress_ctx)
		cops->destroy_compress_ctx(cc);
out:	/* trace 点：结束（带错误码）*/
	trace_f2fs_compress_pages_end(cc->inode, cc->cluster_idx,
							cc->clen, ret);
	return ret;	/* 返回错误码 */
}

static int f2fs_prepare_decomp_mem(struct decompress_io_ctx *dic,
		bool pre_alloc);
static void f2fs_release_decomp_mem(struct decompress_io_ctx *dic,
		bool bypass_destroy_callback, bool pre_alloc);

// 完成压缩簇的完整解压流程
// 先映射，再解压，校检和，后释放，尾唤醒——五步完成簇解压。
void f2fs_decompress_cluster(struct decompress_io_ctx *dic, bool in_task)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(dic->inode);
	struct f2fs_inode_info *fi = F2FS_I(dic->inode);
	const struct f2fs_compress_ops *cops =	/* 算法编号 */
			f2fs_cops[fi->i_compress_algorithm];
	bool bypass_callback = false;	/* 是否跳过算法 destroy 回调 */
	int ret;

	trace_f2fs_decompress_pages_start(dic->inode, dic->cluster_idx,
				dic->cluster_size, fi->i_compress_algorithm);
	/* 1. 如果之前任何一步已标记失败，直接走错误出口 */
	if (dic->failed) {
		ret = -EIO;
		goto out_end_io;
	}

	/* 2. 建立连续内核映射：cbuf(压缩数据) + rbuf(解压结果) */
	ret = f2fs_prepare_decomp_mem(dic, false);
	if (ret) {
		bypass_callback = true;	/* 映射失败，后面不再调用算法 destroy */
		goto out_release;
	}
	/* 3. 取出头部长度字段，并计算原始数据总长度 */
	dic->clen = le32_to_cpu(dic->cbuf->clen);	/* 压缩后长度 */
	dic->rlen = PAGE_SIZE << dic->log_cluster_size;	/* 解压后长度 */

	/* 4. 长度异常直接当腐败处理 */
	if (dic->clen > PAGE_SIZE * dic->nr_cpages - COMPRESS_HEADER_SIZE) {
		ret = -EFSCORRUPTED;	/* 避免在 IRQ 上下文里调用 f2fs_commit_super */

		/* Avoid f2fs_commit_super in irq context */
		if (!in_task)
			f2fs_handle_error_async(sbi, ERROR_FAIL_DECOMPRESSION);
		else
			f2fs_handle_error(sbi, ERROR_FAIL_DECOMPRESSION);
		goto out_release;
	}
	/* 5. 真正调用算法解压（lz4/zstd 等） */
	ret = cops->decompress_pages(dic);
	/* 6. 若开启校验和，计算并比对 */
	if (!ret && (fi->i_compress_flag & BIT(COMPRESS_CHKSUM))) {
		u32 provided = le32_to_cpu(dic->cbuf->chksum);
		u32 calculated = f2fs_crc32(dic->cbuf->cdata, dic->clen);

		if (provided != calculated) {
			if (!is_inode_flag_set(dic->inode, FI_COMPRESS_CORRUPT)) {
				set_inode_flag(dic->inode, FI_COMPRESS_CORRUPT);
				f2fs_info_ratelimited(sbi,
					"checksum invalid, nid = %lu, %x vs %x",
					dic->inode->i_ino,
					provided, calculated);
			}
			set_sbi_flag(sbi, SBI_NEED_FSCK);
		}
	}

out_release:
	/* 7. 解除 rbuf/cbuf 映射，必要时调用算法 destroy 回调 */
	f2fs_release_decomp_mem(dic, bypass_callback, false);

out_end_io:
	trace_f2fs_decompress_pages_end(dic->inode, dic->cluster_idx,
							dic->clen, ret);
	/* 8. 唤醒等待队列、解锁页、更新统计、释放 dic */
	f2fs_decompress_end_io(dic, ret, in_task);
}

/*
 * This is called when a page of a compressed cluster has been read from disk
 * (or failed to be read from disk).  It checks whether this page was the last
 * page being waited on in the cluster, and if so, it decompresses the cluster
 * (or in the case of a failure, cleans up without actually decompressing).
 */
// 压缩簇单个子页 I/O 完成后的统一收尾:失败打标记，成功塞缓存；倒数第一页触发簇解压——三步完成子页收尾。
/*
 * 压缩簇内某一子页读盘结束（成功或失败）时调用：
 * 1. 更新统计；
 * 2. 记录失败/缓存命中页；
 * 3. 若为最后一个子页，则触发簇级解压或错误清理。
 */
void f2fs_end_read_compressed_page(struct page *page, bool failed,
		block_t blkaddr, bool in_task)
{
	struct decompress_io_ctx *dic =
			(struct decompress_io_ctx *)page_private(page);
	struct f2fs_sb_info *sbi = F2FS_I_SB(dic->inode);
	/* 1. 读计数减 1（对应 F2FS_RD_DATA） */
	dec_page_count(sbi, F2FS_RD_DATA);

	if (failed)	/* 2. 只要任一子页失败，整个簇标记失败 */
		WRITE_ONCE(dic->failed, true);
	/* 3. 成功且处于进程上下文，把该子页丢进压缩缓存（COMPRESS_MAPPING） */
	else if (blkaddr && in_task)
		f2fs_cache_compressed_page(sbi, page,
					dic->inode->i_ino, blkaddr);
	/* 4. 原子减到 0 说明这是最后一个子页，进入簇级解压或清理 */
	if (atomic_dec_and_test(&dic->remaining_pages))
		f2fs_decompress_cluster(dic, in_task);
}

/* 判断当前页号是否落在已存在的压缩集群窗口内 */
static bool is_page_in_cluster(struct compress_ctx *cc, pgoff_t index)
{
	/* 1. 集群索引未初始化 → 视为“在集群内”（允许开始攒新集群）*/
	if (cc->cluster_idx == NULL_CLUSTER)
		return true;
	/* 2. 已初始化 → 必须落在同一 16 页窗口（cluster_idx = 页号 >> 4）*/
	return cc->cluster_idx == cluster_idx(cc, index);
}

bool f2fs_cluster_is_empty(struct compress_ctx *cc)
{
	return cc->nr_rpages == 0;
}

static bool f2fs_cluster_is_full(struct compress_ctx *cc)
{
	return cc->cluster_size == cc->nr_rpages;
}

/* “当前页能否继续塞进同一个压缩集群” 的快速判断函数 */
bool f2fs_cluster_can_merge_page(struct compress_ctx *cc, pgoff_t index)
{
	/* 1. 集群里一页都还没有 → 随便合并（开始攒新集群）*/
	if (f2fs_cluster_is_empty(cc))	
		return true;
	/* 2. 集群已有页 → 必须落在同一 16 页窗口内才允许合并 */
	return is_page_in_cluster(cc, index);
}

//  “判断从 pages[] 数组的某个索引开始，是否恰好有一段 连续 16 页 且 页号连续（可选 uptodate）” 
// 的纯逻辑函数，决定是否值得/能够一次性压缩这 16 页。
bool f2fs_all_cluster_page_ready(struct compress_ctx *cc, struct page **pages,
				int index, int nr_pages, bool uptodate)
{
	/* 起始页号（文件内逻辑页号）*/
	unsigned long pgidx = page_folio(pages[index])->index;
	/* uptodate=true 时从 0 开始检查，false 时跳过第 0 页（调用者已保证）*/
	int i = uptodate ? 0 : 1;

	/*
	 * when uptodate set to true, try to check all pages in cluster is
	 * uptodate or not.
	 */
	/*
	 * 当 uptodate=true 时，要求 **起始页号必须 16 对齐**（cluster 窗口边界），
	 * 否则不检查（防止跨窗口脏页混合）
	 */
	if (uptodate && (pgidx % cc->cluster_size))
		return false;
	/* 剩余页数 < 16 → 不够一个 cluster，直接放弃 */
	if (nr_pages - index < cc->cluster_size)
		return false;
	/* 逐页检查：连续 16 页且页号递增 1 */
	for (; i < cc->cluster_size; i++) {
		struct folio *folio = page_folio(pages[index + i]);

		if (folio->index != pgidx + i)
			return false;	/* 页号必须连续（index + i）*/
		/* uptodate=true 时还要求页内容是最新的（已读上盘）*/
		if (uptodate && !folio_test_uptodate(folio))
			return false;
	}

	return true;	/* 全部通过 → 可以一次性压缩这 16 页 */
}

/*
 *  “当前 16 页 cluster 里有没有落在文件末尾之后的页” 的纯逻辑判断——
 *  只要有一页 past-EOF → 视为 invalid，禁止压缩（因为压缩后无法还原原样）。 
 * past-EOF 页 不是 文件内容，而是 “索引号 ≥ 当前文件大小” 的页，出现场景主要有 3 个：
 * 1. fallocate 预扩展（最常见）
 * fallocate -l 10G file 把 i_size 先推到 10G，但 实际数据页还没写。
 * 后续第一次写某个偏移时，F2FS 会 一次性抓取 16 页 cluster，里面就可能包含 超出 i_size 的页号 → past-EOF。
 * 2. 压缩写路径攒页
 * 压缩以 cluster=16 页 为单位抓取，写指针落在 cluster 后半段 时，前半段页号 < i_size，后半段 ≥ i_size → 形成 past-EOF。
 * 3. truncate 后写回延迟
 * truncate(new_size) 把 i_size 缩小，但 页缓存里仍有旧的高索引页 正在回写；
 * 回写线程抓页时把这些 “已无效”的高索引页 带进 cluster → past-EOF。
 * 总结：“文件大小被提前拉大，或缓存里残留高索引页，导致页号 ≥ 当前 i_size” → past-EOF 页，压缩前必须剔除。
 */
static bool cluster_has_invalid_data(struct compress_ctx *cc)
{
	loff_t i_size = i_size_read(cc->inode);	/* 文件当前大小（EOF）*/
	unsigned nr_pages = DIV_ROUND_UP(i_size, PAGE_SIZE);	 /* 换算成页数（含最后一页不足 4K）*/
	int i;
	/* 逐页检查：只要有一页号 ≥ nr_pages → 视为无效 */
	for (i = 0; i < cc->cluster_size; i++) {
		struct page *page = cc->rpages[i];

		f2fs_bug_on(F2FS_I_SB(cc->inode), !page);	/* 断言：页必须存在 */

		/* beyond EOF */
		if (page_folio(page)->index >= nr_pages)
			return true;	/* 页号 ≥ 文件总页数 → 落在 EOF 之后，无效！*/
	}
	return false;	 /* 全部页都在 0…nr_pages-1 → 有效 */
}

// 校验压缩簇地址表是否合法
// 首块须对齐；后续只能连续 V 或连续 N；一出现 C/N+V 混搭就报错并标 fsck。
bool f2fs_sanity_check_cluster(struct dnode_of_data *dn)
{
#ifdef CONFIG_F2FS_CHECK_FS
	struct f2fs_sb_info *sbi = F2FS_I_SB(dn->inode);
	unsigned int cluster_size = F2FS_I(dn->inode)->i_cluster_size;
	int cluster_end = 0;	 /* 记录第一个空洞出现的位置 */
	unsigned int count;
	int i;
	char *reason = "";	/* 出错原因字符串 */

	/* 仅对压缩簇首块（COMPRESS_ADDR）做检查 */
	if (dn->data_blkaddr != COMPRESS_ADDR)
		return false;

	/* [..., COMPR_ADDR, ...] */
	/* 1. 簇首必须在簇对齐位置，否则格式错误 */
	if (dn->ofs_in_node % cluster_size) {
		reason = "[*|C|*|*]";
		goto out;
	}
	/* 2. 扫描后续子块，只允许两种合法形态：
	 *    [C|V|V|V]  或  [C|N|N|N]  （C=COMPRESS_ADDR，V=有效块，N=NEW/NULL）
	 *  一旦出现 [C|...|C|...] 或 [C|N|V|...] 即判非法。
	 */
	for (i = 1, count = 1; i < cluster_size; i++, count++) {
		block_t blkaddr = data_blkaddr(dn->inode, dn->node_folio,
							dn->ofs_in_node + i);

		/* [COMPR_ADDR, ..., COMPR_ADDR] */
		/* 2a. 中间又出现 COMPRESS_ADDR → 非法 */
		if (blkaddr == COMPRESS_ADDR) {
			reason = "[C|*|C|*]";
			goto out;
		}
		/* 2b. 遇到空洞（NEW/NULL）记录位置 */
		if (!__is_valid_data_blkaddr(blkaddr)) {
			if (!cluster_end)
				cluster_end = i;
			continue;
		}
		/* [COMPR_ADDR, NULL_ADDR or NEW_ADDR, valid_blkaddr] */
		/* 2c. 空洞之后又有有效块 → 非法 [C|N|V|...] */
		if (cluster_end) {
			reason = "[C|N|N|V]";
			goto out;
		}
	}

	/* 3. 非 RELEASED 状态下，簇内子块数必须等于 cluster_size */
	f2fs_bug_on(F2FS_I_SB(dn->inode), count != cluster_size &&
		!is_inode_flag_set(dn->inode, FI_COMPRESS_RELEASED));

	return false;	/* 校验通过 */
out:
	/* 4. 打印警告并标记需要 fsck */
	f2fs_warn(sbi, "access invalid cluster, ino:%lu, nid:%u, ofs_in_node:%u, reason:%s",
			dn->inode->i_ino, dn->nid, dn->ofs_in_node, reason);
	set_sbi_flag(sbi, SBI_NEED_FSCK);
	return true;	/* 校验失败 */
#else
	return false;	/* 未开启 CONFIG_F2FS_CHECK_FS 直接放行 */
#endif
}

// 统计一个压缩簇里真正占用的物理块数
static int __f2fs_get_cluster_blocks(struct inode *inode,
					struct dnode_of_data *dn)
{
	unsigned int cluster_size = F2FS_I(inode)->i_cluster_size;
	int count, i;

	/* 从第 0 个子块开始，逐个判断地址是否有效 */
	for (i = 0, count = 0; i < cluster_size; i++) {
		block_t blkaddr = data_blkaddr(dn->inode, dn->node_folio,
							dn->ofs_in_node + i);

		if (__is_valid_data_blkaddr(blkaddr))
			count++;	/* 有效块才计数 */
	}

	return count;
}

// 拿到指定压缩簇的块数/是否压缩
// 先定位簇首 → sanity check → 按类型返回压缩块数/是否压缩/原始块数，三步完成统计。
static int __f2fs_cluster_blocks(struct inode *inode, unsigned int cluster_idx,
				enum cluster_check_type type)
{
	struct dnode_of_data dn;
	/* 簇首页号 */
	unsigned int start_idx = cluster_idx <<
				F2FS_I(inode)->i_log_cluster_size;
	int ret;

	set_new_dnode(&dn, inode, NULL, NULL, 0);
	ret = f2fs_get_dnode_of_data(&dn, start_idx, LOOKUP_NODE);
	if (ret) {
		if (ret == -ENOENT)	/* 空洞节点 → 按 0 块处理 */
			ret = 0;
		goto fail;
	}
	/* 1. 先做一次地址表 sanity check，非法格式直接报腐败 */
	if (f2fs_sanity_check_cluster(&dn)) {
		ret = -EFSCORRUPTED;
		goto fail;
	}
	/* 2. 根据请求类型返回不同计数 */
	if (dn.data_blkaddr == COMPRESS_ADDR) {
		if (type == CLUSTER_COMPR_BLKS)	/* 2a. 要“压缩块数”→ 首块 1 + 后续有效子块数 */
			ret = 1 + __f2fs_get_cluster_blocks(inode, &dn);
		else if (type == CLUSTER_IS_COMPR)	/* 2b. 只要“是否压缩”→ 返回 1 表示是 */
			ret = 1;
	} else if (type == CLUSTER_RAW_BLKS) {
		/* 2c. 要“原始块数”→ 统计簇内所有有效块（非压缩场景）*/
		ret = __f2fs_get_cluster_blocks(inode, &dn);
	}
fail:
	f2fs_put_dnode(&dn);
	return ret;
}

/* return # of compressed blocks in compressed cluster */
static int f2fs_compressed_blocks(struct compress_ctx *cc)
{
	return __f2fs_cluster_blocks(cc->inode, cc->cluster_idx,
		CLUSTER_COMPR_BLKS);
}

/* return # of raw blocks in non-compressed cluster */
// 返回该 cluster 在“逻辑展开视角下”拥有的 raw data block 数量
// 这里的 raw 指的是 未压缩的逻辑数据块,并不等同于“磁盘上实际占用的 block 数”
// 对指定 inode 的第 cluster_idx 个 cluster，查询其 逻辑上应当存在的 raw block 数。
static int f2fs_decompressed_blocks(struct inode *inode,
				unsigned int cluster_idx)
{
	return __f2fs_cluster_blocks(inode, cluster_idx,
		// CLUSTER_RAW_BLKS 是一个枚举参数，用来告诉底层函数：“关心的是 cluster 的 raw（逻辑）block 数，
		// 而不是压缩后的物理 block 数。”
		CLUSTER_RAW_BLKS);
}

/* return whether cluster is compressed one or not */
int f2fs_is_compressed_cluster(struct inode *inode, pgoff_t index)
{
	return __f2fs_cluster_blocks(inode,
		index >> F2FS_I(inode)->i_log_cluster_size,
		CLUSTER_IS_COMPR);
}

/* return whether cluster contains non raw blocks or not */
// 该 cluster 是否“不是完整的一组 raw（普通）数据块”
// 也就是：是否存在 hole / 压缩 / 特殊布局。
// 给定 page index 所在的 cluster，是否是一个 sparse cluster。
bool f2fs_is_sparse_cluster(struct inode *inode, pgoff_t index)
{
	// 将 page index 转换为 cluster index
	unsigned int cluster_idx = index >> F2FS_I(inode)->i_log_cluster_size;

	// f2fs_decompressed_blocks:cluster 在“逻辑视角下”应该有多少个 data block
	// 对普通 cluster：返回 i_cluster_size
	// 对 sparse / hole cluster：返回 < i_cluster_size
	// 对压缩 cluster：仍然返回 i_cluster_size
	// 这个 cluster 的逻辑 block 数不足一个完整 cluster
	return f2fs_decompressed_blocks(inode, cluster_idx) !=
		F2FS_I(inode)->i_cluster_size;
}

/*
 * 是否值得对当前 cluster 启动压缩” 的 快速熔断器——
 * 全部条件通过才返回 true，否则立即回退到原样写，避免进入高成本压缩路径。
 */
static bool cluster_may_compress(struct compress_ctx *cc)
{
	/* 1. 文件没开压缩属性 → 直接放弃 */
	if (!f2fs_need_compress_data(cc->inode))
		return false;
	/* 2. 原子写（sqlite 等）→ 压缩会破环崩溃一致性，禁止 */
	if (f2fs_is_atomic_file(cc->inode))
		return false;
	/* 3. 还没攒满 16 页 → 继续攒，不压（防止小 cluster 膨胀）*/
	if (!f2fs_cluster_is_full(cc))
		return false;
	/* 4. checkpoint 已损坏 → 任何数据变换都禁止 */
	if (unlikely(f2fs_cp_error(F2FS_I_SB(cc->inode))))
		return false;
	/* 5. 集群内存在 past-EOF 页 → 压缩后无法还原，禁止 */
	return !cluster_has_invalid_data(cc);
}

// 把压缩整簇所有页一次性标成写回状态:簇内几页就标几页，存在即置写回，原子批量打标记。
static void set_cluster_writeback(struct compress_ctx *cc)
{
	int i;

	/* 遍历整簇每一页 */
	for (i = 0; i < cc->cluster_size; i++) {
		if (cc->rpages[i])	/* 页存在才处理 */
			set_page_writeback(cc->rpages[i]);	/* 把它标成写回状态 */
	}
}

// 撤销已提交压缩簇的写回（writeback）
// 等已提交 IO 完成 → 对已提交页重新加锁、清 GC 标志、强制结束 writeback——三步撤销写回。
static void cancel_cluster_writeback(struct compress_ctx *cc,
			struct compress_io_ctx *cic, int submitted)
{
	int i;

	/* Wait for submitted IOs. */
	/* 1. 如果已提交超过 1 页，等它们全部完成 */
	if (submitted > 1) {
		/* 1a. 强制把剩余 BIO 推下去 */
		f2fs_submit_merged_write(F2FS_I_SB(cc->inode), DATA);
		/* 1b. 轮询直到 pending_pages 降到预期值（已提交页全部回写完成）*/
		while (atomic_read(&cic->pending_pages) !=
					(cc->valid_nr_cpages - submitted + 1))
			f2fs_io_schedule_timeout(DEFAULT_IO_TIMEOUT);
	}

	/* Cancel writeback and stay locked. */
	/* 2. 对已提交页：重新加锁、清标志、取消 writeback 状态 */
	for (i = 0; i < cc->cluster_size; i++) {
		if (i < submitted) {
			inode_inc_dirty_pages(cc->inode);	/* 重新计入脏页计数 */
			lock_page(cc->rpages[i]);	/* 重新锁住，避免并发写 */
		}
		/* 清 GC-in-flight 标记，恢复普通页状态 */
		clear_page_private_gcing(cc->rpages[i]);
		/* 若页仍处于 writeback，强制结束 writeback（清标志并唤醒等待者）*/
		if (folio_test_writeback(page_folio(cc->rpages[i])))
			end_page_writeback(cc->rpages[i]);
	}
}

// 把整簇页重新标脏并打上 GC-in-flight 标记
// 逐页标脏再加 GC 标记，两步告诉内核：这些页要重新写盘。
static void set_cluster_dirty(struct compress_ctx *cc)
{
	int i;
	/* 遍历簇内每一页 */
	for (i = 0; i < cc->cluster_size; i++)
		if (cc->rpages[i]) {
			set_page_dirty(cc->rpages[i]);	/* 重新加入脏页链表 */
			/* 标记正在 GC 搬移，防止并发写 */
			set_page_private_gcing(cc->rpages[i]);
		}
}

// inline 压缩覆盖写” 的核心实现——
// “把 16 页连续窗口一次性读到内存，保证全部 uptodate，然后把首页交给上层做覆盖写。”
// “先抓 16 页 → 读上盘 → 锁页 → 等写回 → 全部 uptodate → 返回首页给上层覆盖写”
// 任何失败（truncate/EIO）→ 解锁+重试整个窗口。
static int prepare_compress_overwrite(struct compress_ctx *cc,
		struct page **pagep, pgoff_t index, void **fsdata)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(cc->inode);
	struct address_space *mapping = cc->inode->i_mapping;
	struct folio *folio;
	sector_t last_block_in_bio;
	fgf_t fgp_flag = FGP_LOCK | FGP_WRITE | FGP_CREAT;
	pgoff_t start_idx = start_idx_of_cluster(cc);
	int i, ret;

// 窗口合法性 & 资源准备
retry:
	ret = f2fs_is_compressed_cluster(cc->inode, start_idx);
	if (ret <= 0)	/* 不是压缩簇或失败 → 直接退出 */
		return ret;

	ret = f2fs_init_compress_ctx(cc);	/* 分配 rpages[] 数组 */
	if (ret)
		return ret;

	/* keep folio reference to avoid page reclaim */
	// 抓页：两次抓页，因为“一次抓页”确实能把数据读进来，但无法同时满足「不锁页预读」+「加锁等待写回」两种语义要求。
	// 第一次：无锁预读  快速把 16 页全部读进来（不阻塞写回线程）；不等待写回完成，不抢页锁；只 分类（uptodate/非 uptodate）并 记录非 uptodate 页。
	// 第二次：加锁等待写回 只处理第一次留下的非 uptodate 页；加页锁 + 等待写回完成 + 再次校验 uptodate；保证覆盖写时页内容与磁盘一致，避免压缩后无法还原。

	// 第一次抓页 + 读上盘（uptodate）
	// 第一次抓页：不锁页，不等待写回；
	// 只是把 16 页全部“预读”到内存并 加入 ctx->rpages[]；
	// 非 uptodate 的页被留下，已 uptodate 的页直接释放。
	// “抓页 = 找页 + 拿引用 +（可选）加锁 +（可选）等状态”——
	// 把页从页缓存里“抓到”自己手里，后续才能读/写/压缩。
	for (i = 0; i < cc->cluster_size; i++) {
		folio = f2fs_filemap_get_folio(mapping, start_idx + i,
				fgp_flag, GFP_NOFS);
		if (IS_ERR(folio)) {
			ret = PTR_ERR(folio);
			goto unlock_pages;	/* 抓失败 → 清理并退出 */
		}
		/* 已经 uptodate → 直接放掉引用（不锁）*/
		// 后续 覆盖写 会 重新生成新数据，旧内容不再需要。
		// “uptodate 页 = 磁盘已同步，旧数据无用，先放掉引用省内存；
		// 真正写时再重新抓、加锁、生成新数据。”
		if (folio_test_uptodate(folio))
			f2fs_folio_put(folio, true);
		else	/* 非 uptodate → 加入 ctx 待读 */
			// 记录哪些页还需要第二次「加锁+等待写回」
			f2fs_compress_ctx_add_page(cc, folio);
	}

	// 第二次抓页 + 锁页 + 等写回 +  uptodate 校验
	/* 若第一次后仍有非 uptodate 页 → 一次性读上盘 */
	// “把第一次无锁预读时标记为『非 uptodate』的压缩子块，
	// 一次性读盘 → 解压缩 → 全部变成 uptodate → 立即释放第一次资源 → 重新初始化空上下文，供第二次加锁写使用。”
	if (!f2fs_cluster_is_empty(cc)) {
		struct bio *bio = NULL;
		// 把 15 个压缩子块追加到 bio 链（不提交）
		ret = f2fs_read_multi_pages(cc, &bio, cc->cluster_size,
					&last_block_in_bio, NULL, true);
		f2fs_put_rpages(cc);
		f2fs_destroy_compress_ctx(cc, true);	/* 读完成释放第一次资源 */
		if (ret)
			goto out;
		
		// 提交整个 bio 链（一次性下发到 UFS）
		if (bio)
			f2fs_submit_read_bio(sbi, bio, DATA);	/* 提交读 BIO */

		ret = f2fs_init_compress_ctx(cc);	/* 重新初始化压缩上下文（第二次用）*/
		if (ret)
			goto out;
	}

	/* 第二次：逐页加锁 + 等写回完成 + 再次 uptodate 校验 */
	for (i = 0; i < cc->cluster_size; i++) {
		f2fs_bug_on(sbi, cc->rpages[i]);	/* 断言：第二次不应重复分配 */

		folio = filemap_lock_folio(mapping, start_idx + i);	/* **加页锁** */
		if (IS_ERR(folio)) {
			/* folio could be truncated */
			goto release_and_retry;	/* 页可能被 truncate → 重试 */
		}

		//  “确保当前 folio 的写操作已经完成，防止与压缩覆盖写冲突”，这是 写一致性保护 的关键操作。
		// 只有当 folio 不在写回状态时，才能安全地进行压缩覆盖写，否则可能导致数据损坏或数据丢失。
		f2fs_folio_wait_writeback(folio, DATA, true, true);	/* 等写回完成 */
		f2fs_compress_ctx_add_page(cc, folio);	 /* 加入 ctx */

		/* 仍非 uptodate → 视为 IO 错误，重试整个 cluster */
		if (!folio_test_uptodate(folio)) {
			f2fs_handle_page_eio(sbi, folio, DATA);
release_and_retry:
			f2fs_put_rpages(cc);
			f2fs_unlock_rpages(cc, i + 1);
			f2fs_destroy_compress_ctx(cc, true);
			goto retry;	/* 重试整个 16 页窗口 */
		}
	}

	// 返回首页给上层做覆盖写
	if (likely(!ret)) {
		*fsdata = cc->rpages;	/* 返回整个 16 页数组 */
		*pagep = cc->rpages[offset_in_cluster(cc, index)];	/* 返回要覆盖的那一页 */
		return cc->cluster_size;	/* 返回 16（上层知道一次性处理 16 页）*/
	}

unlock_pages:	// 错误清理路径
	f2fs_put_rpages(cc);
	f2fs_unlock_rpages(cc, i);
	f2fs_destroy_compress_ctx(cc, true);
out:
	return ret;
}

// “为 inline 压缩覆盖写（compress-overwrite）准备上下文” 的 入口封装——
// 只负责填充 compress_ctx 头信息，
// 然后把活交给 prepare_compress_overwrite() 去做真正的 页抓取、校验、uptodate 检查。
int f2fs_prepare_compress_overwrite(struct inode *inode,
		/*  输出：返回准备好的页；起始页号；输出：私有数据句柄 */
		struct page **pagep, pgoff_t index, void **fsdata)
{
	struct compress_ctx cc = {
		.inode = inode,
		.log_cluster_size = F2FS_I(inode)->i_log_cluster_size,	/* 默认 4（16 页）*/
		.cluster_size = F2FS_I(inode)->i_cluster_size,	/* 16 */
		.cluster_idx = index >> F2FS_I(inode)->i_log_cluster_size,	/* 16 页对齐窗口编号 */
		.rpages = NULL,	/* 页指针数组待分配 */
		.nr_rpages = 0,	/* 页数待填充 */
	};
	/* 交给底层函数做真正的“抓 16 页 + 检查 uptodate + 返回首页”工作 */
	return prepare_compress_overwrite(&cc, pagep, index, fsdata);
}

//  “压缩覆盖写结束时的清理函数”：
// “把压缩集群的页引用释放掉，清空上下文，标记集群为脏（若写了新数据），并返回是否是集群首页。”
bool f2fs_compress_write_end(struct inode *inode, void *fsdata,
					pgoff_t index, unsigned copied)

{
	/* 初始化压缩上下文 */
	struct compress_ctx cc = {
		.inode = inode,
		.log_cluster_size = F2FS_I(inode)->i_log_cluster_size,
		.cluster_size = F2FS_I(inode)->i_cluster_size,
		.rpages = fsdata,
	};
	struct folio *folio = page_folio(cc.rpages[0]);	/* 拿集群首页 */
	bool first_index = (index == folio->index);	/* 判断是否是集群首页 */

	if (copied)	/* 若有新数据写入 → 标记集群为脏 */
		set_cluster_dirty(&cc);
	/* 释放集群页引用，不涉及 wbc，释放 1 个集群 */
	f2fs_put_rpages_wbc(&cc, NULL, false, 1);
	f2fs_destroy_compress_ctx(&cc, false);	/* 清空压缩上下文 */

	return first_index;	/* 返回是否是集群首页 */
}

// 截断压缩簇尾部多余数据
// 先判是否压缩簇 → 非压缩走老路；压缩簇拿解压页 → 从后往前清零尾部 → 重新压缩写回。
int f2fs_truncate_partial_cluster(struct inode *inode, u64 from, bool lock)
{
	void *fsdata = NULL;
	struct page *pagep;
	int log_cluster_size = F2FS_I(inode)->i_log_cluster_size;
	/* 算出包含 from 的簇首页号 */
	pgoff_t start_idx = from >> (PAGE_SHIFT + log_cluster_size) <<
							log_cluster_size;
	int err;
	/* 1. 先判断该簇是否被压缩 */
	err = f2fs_is_compressed_cluster(inode, start_idx);
	if (err < 0)
		return err;

	/* truncate normal cluster */
	/* 2. 非压缩簇 → 走普通截断路径 */
	if (!err)
		return f2fs_do_truncate_blocks(inode, from, lock);

	/* truncate compressed cluster */
	/* 3. 压缩簇：申请解压上下文，返回所有原始页指针 */
	err = f2fs_prepare_compress_overwrite(inode, &pagep,
						start_idx, &fsdata);

	/* should not be a normal cluster */
	/* 必须返回正数（页数组），否则 BUG */
	f2fs_bug_on(F2FS_I_SB(inode), err == 0);

	if (err <= 0)
		return err;

	/* 4. err > 0 表示已拿到解压后页数组 rpages[] */
	if (err > 0) {
		struct page **rpages = fsdata;
		int cluster_size = F2FS_I(inode)->i_cluster_size;
		int i;

		/* 4a. 从后往前逐页清零尾部数据 */
		for (i = cluster_size - 1; i >= 0; i--) {
			struct folio *folio = page_folio(rpages[i]);
			loff_t start = folio->index << PAGE_SHIFT;

			if (from <= start) {
				/* 整页都在截断点之后 → 全清零 */
				folio_zero_segment(folio, 0, folio_size(folio));
			} else {
				/* 页内部分数据需要保留 → 只清零尾部 */
				folio_zero_segment(folio, from - start,
						folio_size(folio));
				break;	/* 前面页无需处理 */
			}
		}
		/* 4b. 把修改后的页写回磁盘（重新压缩 + 落盘）*/
		f2fs_compress_write_end(inode, fsdata, start_idx, true);
	}
	return 0;
}

//  F2FS 压缩写路径的总入口——
// “把 16 页原始数据压缩成 ≤15 页，填头、写盘、更新元数据，失败时回退原样写。”
static int f2fs_write_compressed_pages(struct compress_ctx *cc,
					int *submitted,
					struct writeback_control *wbc,
					enum iostat_type io_type)
{
	struct inode *inode = cc->inode;
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct f2fs_inode_info *fi = F2FS_I(inode);
	/* 构造压缩写 I/O 控制块 */
	struct f2fs_io_info fio = {
		.sbi = sbi,
		.ino = cc->inode->i_ino,
		.type = DATA,
		.op = REQ_OP_WRITE,
		.op_flags = wbc_to_write_flags(wbc),
		.old_blkaddr = NEW_ADDR,
		.page = NULL,
		.encrypted_page = NULL,
		.compressed_page = NULL,
		.io_type = io_type,
		.io_wbc = wbc,
		.encrypted = fscrypt_inode_uses_fs_layer_crypto(cc->inode) ?
									1 : 0,
	};
	struct folio *folio;
	struct dnode_of_data dn;
	struct node_info ni;
	struct compress_io_ctx *cic;	/* 每-cluster 异步 I/O 上下文 */
	pgoff_t start_idx = start_idx_of_cluster(cc);	/* 16 页对齐窗口起始页号 */
	unsigned int last_index = cc->cluster_size - 1;
	loff_t psize;	/* 文件当前末尾位置 */
	int i, err;
	bool quota_inode = IS_NOQUOTA(inode);

	// Step 1：前置检查与资源加锁
	/* we should bypass data pages to proceed the kworker jobs */
	if (unlikely(f2fs_cp_error(sbi))) {	// checkpoint 损坏 → 直接失败；
		mapping_set_error(inode->i_mapping, -EIO);
		goto out_free;
	}

	if (quota_inode) {	// 配额 inode → 拿 node_write 读锁，防止与 checkpoint 竞争；
		/*
		 * We need to wait for node_write to avoid block allocation during
		 * checkpoint. This can only happen to quota writes which can cause
		 * the below discard race condition.
		 */
		f2fs_down_read(&sbi->node_write);
	} else if (!f2fs_trylock_op(sbi)) {	// 普通文件 → 拿 全局 op 锁，保证块分配互斥。
		goto out_free;
	}

	// Step 2：从 node 块读出 16 个压缩子块地址
	// dn 指向 direct node，里面保存 15 个压缩子块地址（第 0 块是头）。
	set_new_dnode(&dn, cc->inode, NULL, NULL, 0);

	// 这里只是取了第一个block的地址，后面的其他block从他开始，向后累加即可。
	err = f2fs_get_dnode_of_data(&dn, start_idx, LOOKUP_NODE);	// start_idx 是 16 页对齐窗口起始页号；
	if (err)
		goto out_unlock_op;
	/* 断言：16 个子块必须已分配（NULL_ADDR 表示未分配）*/
	for (i = 0; i < cc->cluster_size; i++) {
		if (data_blkaddr(dn.inode, dn.node_folio,
					dn.ofs_in_node + i) == NULL_ADDR)
			goto out_put_dnode;
	}

	folio = page_folio(cc->rpages[last_index]);
	psize = folio_pos(folio) + folio_size(folio);	/* 文件末尾位置 */

	/* 取 node 版本号，用于后续 out-place 写一致性校验 */
	err = f2fs_get_node_info(fio.sbi, dn.nid, &ni, false);
	if (err)
		goto out_put_dnode;

	fio.version = ni.version;

	// Step 3：分配压缩 I/O 上下文（cic）
	cic = f2fs_kmem_cache_alloc(cic_entry_slab, GFP_F2FS_ZERO, false, sbi);
	if (!cic)
		goto out_put_dnode;

	cic->magic = F2FS_COMPRESSED_PAGE_MAGIC;	/* 标记为压缩页 */
	cic->inode = inode;
	atomic_set(&cic->pending_pages, cc->valid_nr_cpages);	/* 待解压页计数 */
	cic->rpages = page_array_alloc(cc->inode, cc->cluster_size);	// rpages[] 保存 原始 16 页指针，解压时用作目标。
	if (!cic->rpages)
		goto out_put_cic;

	cic->nr_rpages = cc->cluster_size;

	// Step 4：逐子块写盘（注意循环次数的限制，是压缩后的block数量）
	for (i = 0; i < cc->valid_nr_cpages; i++) {
		/* 把压缩页标记为“压缩页”并挂到 cic 上 */
		f2fs_set_compressed_page(cc->cpages[i], inode,
				page_folio(cc->rpages[i + 1])->index, cic);
		fio.compressed_page = cc->cpages[i];
		
		/* 取第 i+1 个子块地址（第 0 块是头，不落盘）*/
		fio.old_blkaddr = data_blkaddr(dn.inode, dn.node_folio,
						dn.ofs_in_node + i + 1);	/* 第 i 个子块地址 */

		/* wait for GCed page writeback via META_MAPPING */
		/* 等待旧块写回完成（防止 GC 搬迁冲突）*/
		f2fs_wait_on_block_writeback(inode, fio.old_blkaddr);	/* 等 GC 写回 */

		if (fio.encrypted) {	/* 加密路径：先加密再落盘 */
			fio.page = cc->rpages[i + 1];
			err = f2fs_encrypt_one_page(&fio);
			if (err)
				goto out_destroy_crypt;
			cc->cpages[i] = fio.encrypted_page;
		}
	}

	/* 标记集群为 writeback 状态（防止并发 truncate）*/
	set_cluster_writeback(cc);

	/* 把原始 16 页指针拷到 cic->rpages[]（供解压时用作目标）*/
	for (i = 0; i < cc->cluster_size; i++)
		cic->rpages[i] = cc->rpages[i];

	// 逐子块 out-place 写（15 次循环）,此时写的是压缩之后的block
	for (i = 0; i < cc->cluster_size; i++, dn.ofs_in_node++) {
		block_t blkaddr;

		blkaddr = f2fs_data_blkaddr(&dn);	/* 当前子块地址 */
		fio.page = cc->rpages[i];
		fio.old_blkaddr = blkaddr;

		/* cluster header */
		/* 第 0 块是 cluster header，只标记 COMPRESS_ADDR，不落盘 */
		if (i == 0) {
			if (blkaddr == COMPRESS_ADDR)
				fio.compr_blocks++;	/* 统计压缩块数 */
			if (__is_valid_data_blkaddr(blkaddr))
				f2fs_invalidate_blocks(sbi, blkaddr, 1);	/* 作废旧块 */
			f2fs_update_data_blkaddr(&dn, COMPRESS_ADDR);	/* 标记为压缩头 */
			goto unlock_continue;
		}

		/* 统计压缩子块数 */
		if (fio.compr_blocks && __is_valid_data_blkaddr(blkaddr))
			fio.compr_blocks++;

		/* 超出有效压缩页范围 → 作废子块并标记为 NEW_ADDR */
		if (i > cc->valid_nr_cpages) {
			if (__is_valid_data_blkaddr(blkaddr)) {
				f2fs_invalidate_blocks(sbi, blkaddr, 1);
				f2fs_update_data_blkaddr(&dn, NEW_ADDR);
			}
			goto unlock_continue;
		}

		f2fs_bug_on(fio.sbi, blkaddr == NULL_ADDR);	/* 断言：子块必须已分配 */

		if (fio.encrypted)	/* 加密/压缩页赋值 */
			fio.encrypted_page = cc->cpages[i - 1];
		else
			fio.compressed_page = cc->cpages[i - 1];

		cc->cpages[i - 1] = NULL;	/* 移交 ownership 给 fio */
		fio.submitted = 0;
		/* 真正提交 BIO：out-place 写压缩子块 */
		f2fs_outplace_write_data(&dn, &fio);
		// Step 7：错误回退（膨胀或 I/O 失败）
		if (unlikely(!fio.submitted)) {	/* BIO 未提交 → 回退 */
			cancel_cluster_writeback(cc, cic, i);

			/* To call fscrypt_finalize_bounce_page */
			i = cc->valid_nr_cpages;
			*submitted = 0;
			goto out_destroy_crypt;	/* 回退到原样写 */
		}
		(*submitted)++;	/* 累计已提交页数 */
unlock_continue:
		inode_dec_dirty_pages(cc->inode);	/* 减少 inode 脏页计数 */
		unlock_page(fio.page);	/* 解锁当前页 */
	}

	// Step 5：元数据更新与统计
	if (fio.compr_blocks)	// compr_blocks → 更新 inode 的「压缩块数」字段；
		f2fs_i_compr_blocks_update(inode, fio.compr_blocks - 1, false);
	f2fs_i_compr_blocks_update(inode, cc->valid_nr_cpages, true);
	add_compr_block_stat(inode, cc->valid_nr_cpages);	// add_compr_block_stat → 全局压缩统计；

	set_inode_flag(cc->inode, FI_APPEND_WRITE);	// FI_APPEND_WRITE → 标记文件尾部被写，影响下次分配。

	// Step 6：清理与解锁
	f2fs_put_dnode(&dn);	// 释放 node 块引用；
	if (quota_inode)	// 释放全局/节点锁；
		f2fs_up_read(&sbi->node_write);
	else
		f2fs_unlock_op(sbi);

	spin_lock(&fi->i_size_lock);
	if (fi->last_disk_size < psize)
		fi->last_disk_size = psize;	/* 更新 inode 末尾位置 */
	spin_unlock(&fi->i_size_lock);

	f2fs_put_rpages(cc);	// 解锁 16 页；
	page_array_free(cc->inode, cc->cpages, cc->nr_cpages);	/* 释放压缩页数组 */
	cc->cpages = NULL;
	f2fs_destroy_compress_ctx(cc, false);	// 销毁压缩上下文（保留窗口索引，供下一轮复用）。
	return 0;	/* 成功返回 */

/* ===== 错误清理路径 ===== */
out_destroy_crypt:
	page_array_free(cc->inode, cic->rpages, cc->cluster_size);

	for (--i; i >= 0; i--) {	/* 释放加密 bounce 页 */
		if (!cc->cpages[i])
			continue;
		fscrypt_finalize_bounce_page(&cc->cpages[i]);
	}
out_put_cic:
	kmem_cache_free(cic_entry_slab, cic);
out_put_dnode:
	f2fs_put_dnode(&dn);
out_unlock_op:
	if (quota_inode)
		f2fs_up_read(&sbi->node_write);
	else
		f2fs_unlock_op(sbi);
out_free:
	/* 膨胀或 I/O 失败 → 回退原样写：释放压缩页并返回 -EAGAIN */
	for (i = 0; i < cc->valid_nr_cpages; i++) {
		f2fs_compress_free_page(cc->cpages[i]);
		cc->cpages[i] = NULL;
	}
	page_array_free(cc->inode, cc->cpages, cc->nr_cpages);
	cc->cpages = NULL;
	return -EAGAIN;	/* 告诉上层“回退到原样写”*/
}

// 压缩写 BIO 完成后的统一收尾
// 单页失败→标记错误；最后一页→批量清标记、结束 writeback、释放上下文
void f2fs_compress_write_end_io(struct bio *bio, struct page *page)
{
	struct f2fs_sb_info *sbi = bio->bi_private;
	struct compress_io_ctx *cic =
			(struct compress_io_ctx *)page_private(page);	/* 取回压缩写上下文 */
	enum count_type type = WB_DATA_TYPE(page,
				f2fs_is_compressed_page(page));	/* 按压缩页类型计数 */
	int i;
	/* 1. I/O 失败 → 标记 mapping 错误，让上层知道数据不可用 */
	if (unlikely(bio->bi_status != BLK_STS_OK))
		mapping_set_error(cic->inode->i_mapping, -EIO);
	/* 2. 释放压缩子页本身（bounce/缓存页）*/
	f2fs_compress_free_page(page);
	/* 3. 更新写计数（WB_DATA_TYPE 已区分 CP/普通数据）*/
	dec_page_count(sbi, type);
	/* 4. 还有别的压缩子页未完成 → 直接返回 */
	if (atomic_dec_return(&cic->pending_pages))
		return;
	/* 5. 最后一个子页完成：批量结束所有原始页的 writeback 状态 */
	for (i = 0; i < cic->nr_rpages; i++) {
		WARN_ON(!cic->rpages[i]);
		clear_page_private_gcing(cic->rpages[i]);	/* 清 GC 搬移标记 */
		end_page_writeback(cic->rpages[i]);	/* 唤醒等待者 */
	}
	/* 6. 释放原始页指针数组和压缩写上下文本身 */
	page_array_free(cic->inode, cic->rpages, cic->nr_rpages);
	kmem_cache_free(cic_entry_slab, cic);
}

// 把压缩簇回退成普通簇并逐页写盘
// 先算占用→重标脏→锁 node→逐页走正常 writepage→解锁再平衡
static int f2fs_write_raw_pages(struct compress_ctx *cc,
					int *submitted_p,
					struct writeback_control *wbc,
					enum iostat_type io_type)
{
	struct address_space *mapping = cc->inode->i_mapping;
	struct f2fs_sb_info *sbi = F2FS_M_SB(mapping);
	int submitted, compr_blocks, i;
	int ret = 0;
	/* 1. 先计算当前压缩簇实际占用块数（<0 表示出错）*/
	compr_blocks = f2fs_compressed_blocks(cc);
	/* 2. 把所有原始页重新标脏并解锁，让后续 writepage 能抓到它们 */
	for (i = 0; i < cc->cluster_size; i++) {
		if (!cc->rpages[i])
			continue;

		redirty_page_for_writepage(wbc, cc->rpages[i]);
		unlock_page(cc->rpages[i]);
	}

	if (compr_blocks < 0)
		return compr_blocks;

	/* overwrite compressed cluster w/ normal cluster */
	/* 3. 若压缩簇里曾有真实块，需要锁全局 node 操作，防止并发 GC 移动 */
	if (compr_blocks > 0)
		f2fs_lock_op(sbi);
	/* 4. 逐页走普通 writepage 路径：压缩簇→正常簇 */
	for (i = 0; i < cc->cluster_size; i++) {
		struct folio *folio;

		if (!cc->rpages[i])
			continue;
		folio = page_folio(cc->rpages[i]);
retry_write:
		folio_lock(folio);
		/* 4a. 页已不在本映射，跳过 */
		if (folio->mapping != mapping) {
continue_unlock:
			folio_unlock(folio);
			continue;
		}
		/* 4b. 页不再脏，跳过 */
		if (!folio_test_dirty(folio))
			goto continue_unlock;
		/* 4c. 页正在回写，视同步模式决定等待或跳过 */
		if (folio_test_writeback(folio)) {
			if (wbc->sync_mode == WB_SYNC_NONE)
				goto continue_unlock;
			f2fs_folio_wait_writeback(folio, DATA, true, true);
		}
		/* 4d. 抢脏标记失败，跳过 */
		if (!folio_clear_dirty_for_io(folio))
			goto continue_unlock;

		submitted = 0;
		/* 4e. 走单页写路径，传入 compr_blocks 告诉底层这是“回退写” */
		ret = f2fs_write_single_data_page(folio, &submitted,
						NULL, NULL, wbc, io_type,
						compr_blocks, false);
		if (ret) {
			if (ret == 1) {
				ret = 0;	/* 已写，不算错误 */
			} else if (ret == -EAGAIN) {
				ret = 0;
				/*
				 * for quota file, just redirty left pages to
				 * avoid deadlock caused by cluster update race
				 * from foreground operation.
				 */
				// 配额文件场景：避免集群更新与前台操作死锁，直接重新标脏后延时重试。
				if (IS_NOQUOTA(cc->inode))
					goto out;
				f2fs_io_schedule_timeout(DEFAULT_IO_TIMEOUT);
				goto retry_write;	/* 重试本页 */
			}
			goto out;	/* 真错误，跳出 */
		}

		*submitted_p += submitted;	/* 累加已提交页数 */
	}

out:	/* 5. 如果前面加了全局锁，这里释放 */
	if (compr_blocks > 0)
		f2fs_unlock_op(sbi);
	/* 6. 尝试平衡 FS（必要时触发 GC）*/
	f2fs_balance_fs(sbi, true);
	return ret;
}

/*
 * F2FS 压缩写路径总入口：
 * “先尝试压缩，失败就回退到普通写，最后统一清理”
 */
int f2fs_write_multi_pages(struct compress_ctx *cc,
					int *submitted,
					struct writeback_control *wbc,
					enum iostat_type io_type)
{
	int err;

	*submitted = 0;	/* 本轮已提交页数清零 */
	/* ===== 1. 尝试压缩 ===== */
	if (cluster_may_compress(cc)) {	/* 快速熔断：压缩开关+页够+无错误 */
		err = f2fs_compress_pages(cc);	 /* 真正压缩：返回 0 成功，-EAGAIN 膨胀，负值错误 */
		if (err == -EAGAIN) {	/* 压缩后膨胀 → 回退到原样写 */
			add_compr_block_stat(cc->inode, cc->cluster_size);
			goto write;	/* 跳转到原样写分支 */
		} else if (err) {	/* 内存失败/CRC 错误等 → 释放资源并返回错误 */
			f2fs_put_rpages_wbc(cc, wbc, true, 1);
			goto destroy_out;
		}
		/* 压缩成功 → 写压缩后数据 */
		err = f2fs_write_compressed_pages(cc, submitted,
							wbc, io_type);
		if (!err)	/* 压缩写成功，提前返回 */
			return 0;
		/* 压缩写返回 -EAGAIN（落盘失败）→ 继续走原样写 */
		f2fs_bug_on(F2FS_I_SB(cc->inode), err != -EAGAIN);
	}
write:	/* ===== 2. 原样写分支（压缩失败或膨胀）===== */
	/* 断言：原样写分支不应有 submitted */
	f2fs_bug_on(F2FS_I_SB(cc->inode), *submitted);

	err = f2fs_write_raw_pages(cc, submitted, wbc, io_type);	/* 正常日志写 */
	f2fs_put_rpages_wbc(cc, wbc, false, 0);	/* 释放页锁但不释放 page 本身（原样写已处理）*/
destroy_out:	/* ===== 3. 出口 ===== */
	f2fs_destroy_compress_ctx(cc, false);	/* 释放压缩上下文 & page 数组 */
	return err;	/* 返回 0 或负值错误 */
}

static inline bool allow_memalloc_for_decomp(struct f2fs_sb_info *sbi,
		bool pre_alloc)
{
	return pre_alloc ^ f2fs_low_mem_mode(sbi);
}

// 为解压算法提前准备内存环境
// 能预分配就申请 tpages/cpages → 双缓冲映射 rbuf/cbuf → 算法私有 ctx 初始化，三步把解压内存环境搭好。
static int f2fs_prepare_decomp_mem(struct decompress_io_ctx *dic,
		bool pre_alloc)
{
	/* 1. 取出对应压缩算法的回调函数集（lz4/lz4hc/zstd） */
	const struct f2fs_compress_ops *cops =
		f2fs_cops[F2FS_I(dic->inode)->i_compress_algorithm];
	int i;

	/* 2. 若系统内存紧张且不允许预分配，直接跳过（后面走慢路径单页解压） */
	if (!allow_memalloc_for_decomp(F2FS_I_SB(dic->inode), pre_alloc))
		return 0;

	/* 3. 申请“临时页指针数组” tpages，长度 = 簇大小（解压后页数） */
	dic->tpages = page_array_alloc(dic->inode, dic->cluster_size);
	if (!dic->tpages)
		return -ENOMEM;

	/* 4. 填充 tpages：已有 rpages 直接复用；缺页就新申请 */
	for (i = 0; i < dic->cluster_size; i++) {
		if (dic->rpages[i]) {
			dic->tpages[i] = dic->rpages[i];	/* 复用原始页，避免双倍占用 */
			continue;
		}
		/* 缺页时申请新页，后续解压结果直接写进来 */
		dic->tpages[i] = f2fs_compress_alloc_page();
	}
	/* 5. 把 tpages 整个映射为连续虚拟地址 rbuf，供算法一次性读写 */
	dic->rbuf = f2fs_vmap(dic->tpages, dic->cluster_size);
	if (!dic->rbuf)
		return -ENOMEM;
	/* 6. 同理把 cpages 映射为连续虚拟地址 cbuf，存放待解压数据 */
	dic->cbuf = f2fs_vmap(dic->cpages, dic->nr_cpages);
	if (!dic->cbuf)
		return -ENOMEM;
	/* 7. 若算法需要私有上下文（zstd 流、lz4 状态等），回调初始化 */
	if (cops->init_decompress_ctx)
		return cops->init_decompress_ctx(dic);

	return 0;	/* 一切就绪，返回成功 */
}

// 释放解压算法私有内存并解除映射
static void f2fs_release_decomp_mem(struct decompress_io_ctx *dic,
		bool bypass_destroy_callback, bool pre_alloc)
{
	/* 1. 取出当前压缩算法对应的回调函数集（lz4/zstd 等） */
	const struct f2fs_compress_ops *cops =
		f2fs_cops[F2FS_I(dic->inode)->i_compress_algorithm];

	/* 2. 如果系统内存紧张且不允许分配，直接返回（本函数只负责“预分配”路径的清理） */
	if (!allow_memalloc_for_decomp(F2FS_I_SB(dic->inode), pre_alloc))
		return;

	/* 3. 若上层未要求跳过回调，且算法提供了 destroy 钩子，则先释放算法私有资源
	 *    （例如 zstd 的 dstream 句柄、workspace 等） 
	 */
	if (!bypass_destroy_callback && cops->destroy_decompress_ctx)
		cops->destroy_decompress_ctx(dic);

	// 4. 解除压缩数据缓冲区 cbuf 的连续内核映射（vm_unmap_ram 与前面的 vm_map_ram 成对）
	if (dic->cbuf)
		vm_unmap_ram(dic->cbuf, dic->nr_cpages);
	/* 5. 同理解除解压结果缓冲区 rbuf 的连续内核映射 */
	if (dic->rbuf)
		vm_unmap_ram(dic->rbuf, dic->cluster_size);
}

static void f2fs_free_dic(struct decompress_io_ctx *dic,
		bool bypass_destroy_callback);

// 为解压 I/O 分配并初始化上下文（dic）
// “先拿 dic 壳，再配 rpages/cpages 两数组，挂好引用和私有，最后给算法留内存——四大步搞定解压上下文。
struct decompress_io_ctx *f2fs_alloc_dic(struct compress_ctx *cc)
{
	struct decompress_io_ctx *dic;	// 返回值
	pgoff_t start_idx = start_idx_of_cluster(cc);	/* 簇首页号 */
	struct f2fs_sb_info *sbi = F2FS_I_SB(cc->inode);
	int i, ret;

	/* 1. 从 slab 分配 dic 本体并清零 */
	dic = f2fs_kmem_cache_alloc(dic_entry_slab, GFP_F2FS_ZERO, false, sbi);
	if (!dic)
		return ERR_PTR(-ENOMEM);

	/* 2. 为解压后的原始页数组分配指针空间 */
	dic->rpages = page_array_alloc(cc->inode, cc->cluster_size);
	if (!dic->rpages) {
		kmem_cache_free(dic_entry_slab, dic);
		return ERR_PTR(-ENOMEM);
	}

	/* 3. 填充 dic 头部字段 */
	dic->magic = F2FS_COMPRESSED_PAGE_MAGIC;
	dic->inode = cc->inode;
	atomic_set(&dic->remaining_pages, cc->nr_cpages);	/* 待解压子页计数 */
	dic->cluster_idx = cc->cluster_idx;
	dic->cluster_size = cc->cluster_size;
	dic->log_cluster_size = cc->log_cluster_size;
	dic->nr_cpages = cc->nr_cpages;
	refcount_set(&dic->refcnt, 1);	/* 引用计数初始 1 */
	dic->failed = false;
	dic->need_verity = f2fs_need_verity(cc->inode, start_idx);

	/* 4. 把 cc 里已挂好的原始页指针全部搬进来 */
	for (i = 0; i < dic->cluster_size; i++)
		dic->rpages[i] = cc->rpages[i];
	dic->nr_rpages = cc->cluster_size;	/* 已解压原始页计数 */

	/* 5. 为解压后的压缩页数组分配指针空间 */
	dic->cpages = page_array_alloc(dic->inode, dic->nr_cpages);
	if (!dic->cpages) {
		ret = -ENOMEM;
		goto out_free;
	}

	/* 6. 逐个分配压缩子页并挂到 dic->cpages[]，同时把页私有指针指向 dic */
	for (i = 0; i < dic->nr_cpages; i++) {
		struct page *page;

		page = f2fs_compress_alloc_page();	/* 申请一页用于存压缩数据 */
		f2fs_set_compressed_page(page, cc->inode,
					start_idx + i + 1, dic);	/* 私有数据挂 dic */
		dic->cpages[i] = page;
	}

	/* 7. 为解压算法预先分配/映射内存 */
	ret = f2fs_prepare_decomp_mem(dic, true);
	if (ret)
		goto out_free;

	return dic;	/* 成功返回 dic */

out_free:
	f2fs_free_dic(dic, true);	/* 失败回滚 */
	return ERR_PTR(ret);
}

// 彻底释放解压上下文 dic
// 先放算法内存，再按 tpages/cpages/rpages 顺序还页，最后把 dic 壳子扔回 slab——五步全清。
static void f2fs_free_dic(struct decompress_io_ctx *dic,
		bool bypass_destroy_callback)
{
	int i;
	/* 1. 先释放算法私有内存（workspace、流句柄等）并解除 rbuf/cbuf 映射 */
	f2fs_release_decomp_mem(dic, bypass_destroy_callback, true);
	/* 2. 释放临时页数组 tpages：只释放在本函数中新申请的页，rpages[i] 已存在就跳过 */
	if (dic->tpages) {
		for (i = 0; i < dic->cluster_size; i++) {
			if (dic->rpages[i])	/* 复用页，不归这里释放 */
				continue;
			if (!dic->tpages[i])	/* 空指针保护 */
				continue;
			f2fs_compress_free_page(dic->tpages[i]);
		}
		page_array_free(dic->inode, dic->tpages, dic->cluster_size);
	}
	/* 3. 释放压缩子页数组 cpages */
	if (dic->cpages) {
		for (i = 0; i < dic->nr_cpages; i++) {
			if (!dic->cpages[i])
				continue;
			f2fs_compress_free_page(dic->cpages[i]);
		}
		page_array_free(dic->inode, dic->cpages, dic->nr_cpages);
	}
	/* 4. 释放原始页指针数组 rpages（仅指针数组本身，页归 page cache）*/
	page_array_free(dic->inode, dic->rpages, dic->nr_rpages);
	/* 5. 把 dic 结构体还给 slab */
	kmem_cache_free(dic_entry_slab, dic);
}

static void f2fs_late_free_dic(struct work_struct *work)
{
	struct decompress_io_ctx *dic =
		container_of(work, struct decompress_io_ctx, free_work);

	f2fs_free_dic(dic, false);
}

// 安全释放 dic
// 引用归零才回收；进程上下文直接 free，中断上下文延后扔 workqueue。
static void f2fs_put_dic(struct decompress_io_ctx *dic, bool in_task)
{
	/* 引用计数减 1，减到 0 才真正回收 */
	if (refcount_dec_and_test(&dic->refcnt)) {
		if (in_task) {
			/* 当前上下文就是进程上下文，可直接释放 */
			f2fs_free_dic(dic, false);
		} else {
			/* 中断/软中断里不能 vfree，扔给 post_read_wq 延后回收 */
			INIT_WORK(&dic->free_work, f2fs_late_free_dic);
			queue_work(F2FS_I_SB(dic->inode)->post_read_wq,
					&dic->free_work);
		}
	}
}

// 在 workqueue 中对整簇解压页做 fs-verity 校验
// 逐页 verity→置/清 uptodate→解锁→put_dic
static void f2fs_verify_cluster(struct work_struct *work)
{
	struct decompress_io_ctx *dic =
		container_of(work, struct decompress_io_ctx, verity_work);
	int i;

	/* Verify, update, and unlock the decompressed pages. */
	/* 逐页调用 fs-verity 校验，失败则清 uptodate，成功则置位 */
	for (i = 0; i < dic->cluster_size; i++) {
		struct page *rpage = dic->rpages[i];

		if (!rpage)
			continue;

		if (fsverity_verify_page(rpage))	/* 通过校验 */
			SetPageUptodate(rpage);
		else	/* 校验失败 */
			ClearPageUptodate(rpage);
		unlock_page(rpage);	/* 唤醒等待者 */
	}
	/* 所有页处理完，释放对 dic 的引用（引用为 0 时回收内存）*/
	f2fs_put_dic(dic, true);
}

/*
 * This is called when a compressed cluster has been decompressed
 * (or failed to be read and/or decompressed).
 */
// 压缩簇 I/O 完结后收尾
// verity 先插队；无 verity 就批量置 uptodate 并解锁页，最后 put_dic 收工。
/*
 * 压缩簇 I/O 完结回调：无论解压成功还是失败，都要把结果同步到 pagecache
 * 并唤醒等待者，最后释放 dic。
 */
void f2fs_decompress_end_io(struct decompress_io_ctx *dic, bool failed,
				bool in_task)
{
	int i;

	/* 1. 若解压成功且文件启用了 fs-verity，把校验工作交给 verity 工作队列
	 *    注意：不能在本函数所在解压 workqueue 里直接做，避免递归死锁 
	 */
	if (!failed && dic->need_verity) {
		/*
		 * Note that to avoid deadlocks, the verity work can't be done
		 * on the decompression workqueue.  This is because verifying
		 * the data pages can involve reading metadata pages from the
		 * file, and these metadata pages may be compressed.
		 */
		INIT_WORK(&dic->verity_work, f2fs_verify_cluster);
		fsverity_enqueue_verify_work(&dic->verity_work);
		return;	/* verity 完成后再继续收尾 */
	}

	/* Update and unlock the cluster's pagecache pages. */
	/* 2. 正常路径：把每页 uptodate 状态写回并解锁，唤醒等待者 */
	for (i = 0; i < dic->cluster_size; i++) {
		struct page *rpage = dic->rpages[i];

		if (!rpage)
			continue;

		if (failed)
			ClearPageUptodate(rpage);	/* 失败页标记非最新，后续会重读/报错 */
		else
			SetPageUptodate(rpage);	/* 成功页标记最新，避免再次读盘 */
		unlock_page(rpage);	/* 唤醒因 lock_page 睡眠的进程 */
	}

	/*
	 * Release the reference to the decompress_io_ctx that was being held
	 * for I/O completion.
	 */
	/* 3. 释放 dic 本身：引用计数减 0 时自动回收所有内存 */
	f2fs_put_dic(dic, in_task);
}

/*
 * Put a reference to a compressed folio's decompress_io_ctx.
 *
 * This is called when the folio is no longer needed and can be freed.
 */
// 释放一页压缩子页所持有的 dic 引用
/*
 * 压缩子页即将被释放时，递减其对解压上下文 dic 的引用计数；
 * 当引用归零时，dic 本身也会被回收。
 */
void f2fs_put_folio_dic(struct folio *folio, bool in_task)
{
	struct decompress_io_ctx *dic = folio->private;	/* 取回之前绑定的 dic */

	f2fs_put_dic(dic, in_task);	/* 引用计数减 1，归零则延后或立即释放 dic */
}

/*
 * check whether cluster blocks are contiguous, and add extent cache entry
 * only if cluster blocks are logically and physically contiguous.
 */
// 检查压缩簇内子块是否逻辑且物理连续，以决定是否可加入 extent cache:
// 跳过占位符，逐块比对地址是否连续，遇空洞或跳号即返回 0，否则返回连续子块数。
unsigned int f2fs_cluster_blocks_are_contiguous(struct dnode_of_data *dn,
						unsigned int ofs_in_node)
{
	bool compressed = data_blkaddr(dn->inode, dn->node_folio,
					ofs_in_node) == COMPRESS_ADDR;
	int i = compressed ? 1 : 0;	/* 压缩簇跳过首占位符 COMPRESS_ADDR */
	block_t first_blkaddr = data_blkaddr(dn->inode, dn->node_folio,
							ofs_in_node + i);
	/* 从第 2 个子块开始，检查是否严格连续 */
	for (i += 1; i < F2FS_I(dn->inode)->i_cluster_size; i++) {
		block_t blkaddr = data_blkaddr(dn->inode, dn->node_folio,
							ofs_in_node + i);

		if (!__is_valid_data_blkaddr(blkaddr))
			break;	/* 遇到空洞或非有效块，立即中断，连续失败 */
		/* 期望地址 = 首块 + (i - 是否跳过占位符) */
		if (first_blkaddr + i - (compressed ? 1 : 0) != blkaddr)
			return 0;
	}
	/* 不连续，返回 0 */
	return compressed ? i - 1 : i;
}

const struct address_space_operations f2fs_compress_aops = {
	.release_folio = f2fs_release_folio,
	.invalidate_folio = f2fs_invalidate_folio,
	.migrate_folio	= filemap_migrate_folio,
};

struct address_space *COMPRESS_MAPPING(struct f2fs_sb_info *sbi)
{
	return sbi->compress_inode->i_mapping;
}

// 批量失效压缩缓存页:
// 没开缓存就跳过；开了就把 COMPRESS_MAPPING 里一段页刷掉，两步完成压缩缓存失效。
void f2fs_invalidate_compress_pages_range(struct f2fs_sb_info *sbi,
				block_t blkaddr, unsigned int len)
{
	if (!sbi->compress_inode)	/* 1. 如果挂载时没开 compress_cache，直接返回 */
		return;
	/* 2. 以块号为索引，把 COMPRESS_MAPPING 里对应范围的页刷出缓存 */
	invalidate_mapping_pages(COMPRESS_MAPPING(sbi), blkaddr, blkaddr + len - 1);
}

// 把刚读到的压缩子页塞进 COMPRESS_CACHE
// 三检查→不存在就申请→挂映射→拷数据→标 uptodate，五步把压缩子页塞进专用缓存。
void f2fs_cache_compressed_page(struct f2fs_sb_info *sbi, struct page *page,
						nid_t ino, block_t blkaddr)
{
	struct folio *cfolio;
	int ret;

	/* 1. 挂载时没开 compress_cache 直接放弃 */
	if (!test_opt(sbi, COMPRESS_CACHE))
		return;

	/* 2. 块号非法也放弃 */
	if (!f2fs_is_valid_blkaddr(sbi, blkaddr, DATA_GENERIC_ENHANCE_READ))
		return;
	/* 3. 系统内存紧张时放弃 */
	if (!f2fs_available_free_memory(sbi, COMPRESS_PAGE))
		return;
	/* 4. 缓存里已存在 → 直接退出（不重复插入） */
	cfolio = filemap_get_folio(COMPRESS_MAPPING(sbi), blkaddr);
	if (!IS_ERR(cfolio)) {
		f2fs_folio_put(cfolio, false);
		return;
	}
	/* 5. 申请新 folio（单页，0 阶）*/
	cfolio = filemap_alloc_folio(__GFP_NOWARN | __GFP_IO, 0);
	if (!cfolio)
		return;
	/* 6. 把 folio 挂到 COMPRESS_MAPPING，索引 = blkaddr */
	ret = filemap_add_folio(COMPRESS_MAPPING(sbi), cfolio,
						blkaddr, GFP_NOFS);
	if (ret) {
		f2fs_folio_put(cfolio, false);
		return;
	}
	/* 7. 用 page->private 存 inode number，便于老化/失效 */
	set_page_private_data(&cfolio->page, ino);
	/* 8. 拷贝压缩数据，标记 uptodate */
	memcpy(folio_address(cfolio), page_address(page), PAGE_SIZE);
	folio_mark_uptodate(cfolio);
	f2fs_folio_put(cfolio, true);	/* 释放锁，保留在缓存中 */
}

// 尝试从压缩缓存命中并拷回数据
// 开缓存 → 按块号找压缩页 → uptodate 就整页 memcpy 回来，命中计数加一。
bool f2fs_load_compressed_folio(struct f2fs_sb_info *sbi, struct folio *folio,
								block_t blkaddr)
{
	struct folio *cfolio;
	bool hitted = false;

	/* 1. 挂载时没开 COMPRESS_CACHE 直接放弃 */
	if (!test_opt(sbi, COMPRESS_CACHE))
		return false;
	/* 2. 以 blkaddr 为索引，去 COMPRESS_MAPPING 找已缓存的压缩页（不阻塞） */
	cfolio = f2fs_filemap_get_folio(COMPRESS_MAPPING(sbi),
				blkaddr, FGP_LOCK | FGP_NOWAIT, GFP_NOFS);
	if (!IS_ERR(cfolio)) {
		/* 3. 若缓存页是 uptodate，直接 memcpy 解压结果到目标 folio */
		if (folio_test_uptodate(cfolio)) {
			atomic_inc(&sbi->compress_page_hit);	/* 统计命中 */
			memcpy(folio_address(folio),
				folio_address(cfolio), folio_size(folio));
			hitted = true;
		}
		f2fs_folio_put(cfolio, true);	/* 解锁并放引用 */
	}

	return hitted;	/* 返回是否命中 */
}

// 按 inode 号批量踢出压缩缓存页
// 分批抓页→锁页→比对 inode 号→匹配就踢出缓存→循环直到扫完
void f2fs_invalidate_compress_pages(struct f2fs_sb_info *sbi, nid_t ino)
{
	struct address_space *mapping = COMPRESS_MAPPING(sbi);
	struct folio_batch fbatch;	/* 批量抓取 folio 的容器 */
	pgoff_t index = 0;	/* 从 0 开始扫描 */
	pgoff_t end = MAX_BLKADDR(sbi);	/* 最大块地址作为扫描上限 */

	if (!mapping->nrpages)	/* 1. 缓存空，直接回家 */
		return;

	folio_batch_init(&fbatch);

	do {	/* 2. 分批抓取 COMPRESS_MAPPING 里的所有 folio */
		unsigned int nr, i;

		nr = filemap_get_folios(mapping, &index, end - 1, &fbatch);
		if (!nr)
			break;
		/* 3. 逐页比对 inode 号，匹配则踢出缓存 */
		for (i = 0; i < nr; i++) {
			struct folio *folio = fbatch.folios[i];

			folio_lock(folio);
			if (folio->mapping != mapping) {	/* 已被并发移除 */
				folio_unlock(folio);
				continue;
			}
			/* inode 号不匹配，跳过 */
			if (ino != get_page_private_data(&folio->page)) {
				folio_unlock(folio);
				continue;
			}
			/* 4. 从地址空间移除并释放缓存页 */
			generic_error_remove_folio(mapping, folio);
			folio_unlock(folio);
		}
		folio_batch_release(&fbatch);	/* 释放本轮引用 */
		cond_resched();	/* 让出 CPU，避免软锁死 */
	} while (index < end);
}

// 初始化 F2FS 的压缩缓存专用 inode
// 没开缓存就跳过；开了就 iget 专用 inode→设参数→清零命中计数
int f2fs_init_compress_inode(struct f2fs_sb_info *sbi)
{
	struct inode *inode;
	/* 1. 挂载时没开 compress_cache，直接成功返回 */
	if (!test_opt(sbi, COMPRESS_CACHE))
		return 0;
	/* 2. 根据固定 inode 号 F2FS_COMPRESS_INO 拿到专用 inode */
	inode = f2fs_iget(sbi->sb, F2FS_COMPRESS_INO(sbi));
	if (IS_ERR(inode))
		return PTR_ERR(inode);	/* 失败返回错误码 */
	sbi->compress_inode = inode;	/* 保存到 sbi，后续 COMPRESS_MAPPING 用它 */
	/* 3. 初始化压缩缓存相关水位与百分比参数 */
	sbi->compress_percent = COMPRESS_PERCENT;	/* 默认压缩率阈值 */
	sbi->compress_watermark = COMPRESS_WATERMARK;	/* 内存水位线 */
	/* 4. 压缩页命中计数器归零 */
	atomic_set(&sbi->compress_page_hit, 0);

	return 0;
}

// 销毁 F2FS 的压缩缓存专用 inode:有就 iput 释放，再把指针清零
void f2fs_destroy_compress_inode(struct f2fs_sb_info *sbi)
{
	/* 1. 如果压缩缓存 inode 未初始化，直接返回 */
	if (!sbi->compress_inode)
		return;
	/* 2. 释放对压缩缓存 inode 的引用，触发回收 */
	iput(sbi->compress_inode);
	sbi->compress_inode = NULL;	/* 清空指针，防止悬垂 */
}

// 为压缩功能创建‘页指针数组’的 slab 缓存
// 没压缩就跳过；有压缩就按设备号命名、按簇大小算长度，创建 slab 缓存
int f2fs_init_page_array_cache(struct f2fs_sb_info *sbi)
{
	dev_t dev = sbi->sb->s_bdev->bd_dev;	/* 取底层设备号 */
	char slab_name[35];
	/* 1. 文件系统没开压缩特性，直接成功 */
	if (!f2fs_sb_has_compression(sbi))
		return 0;
	/* 2. 用设备号构造唯一 slab 名，避免多设备冲突 */
	sprintf(slab_name, "f2fs_page_array_entry-%u:%u", MAJOR(dev), MINOR(dev));
	/* 3. 计算 slab 对象大小：指针数 × 簇大小（压缩页数组）*/
	sbi->page_array_slab_size = sizeof(struct page *) <<
					F2FS_OPTION(sbi).compress_log_size;
	/* 4. 创建 slab，失败返回 -ENOMEM */
	sbi->page_array_slab = f2fs_kmem_cache_create(slab_name,
					sbi->page_array_slab_size);
	return sbi->page_array_slab ? 0 : -ENOMEM;
}

void f2fs_destroy_page_array_cache(struct f2fs_sb_info *sbi)
{
	kmem_cache_destroy(sbi->page_array_slab);
}

int __init f2fs_init_compress_cache(void)
{
	cic_entry_slab = f2fs_kmem_cache_create("f2fs_cic_entry",
					sizeof(struct compress_io_ctx));
	if (!cic_entry_slab)
		return -ENOMEM;
	dic_entry_slab = f2fs_kmem_cache_create("f2fs_dic_entry",
					sizeof(struct decompress_io_ctx));
	if (!dic_entry_slab)
		goto free_cic;
	return 0;
free_cic:
	kmem_cache_destroy(cic_entry_slab);
	return -ENOMEM;
}

void f2fs_destroy_compress_cache(void)
{
	kmem_cache_destroy(dic_entry_slab);
	kmem_cache_destroy(cic_entry_slab);
}
