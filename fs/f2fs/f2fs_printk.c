#include <linux/string.h>
#include <linux/nls.h>
#include "f2fs_printk.h"

/**
 * print_f2fs_super_block - 打印 f2fs_super_block 结构体的所有成员
 * @sb: 指向 f2fs_super_block 结构体的指针
 *
 * 此函数会遍历 f2fs_super_block 结构体的所有成员，并使用 pr_info 打印其值。
 */
void print_f2fs_super_block(const struct f2fs_super_block *sb)
{
	int i, j;

	if (!sb) {
		pr_info("f2fs_super_block pointer is NULL\n");
		return;
	}

	pr_info("f2fs_super_block members:\n");
	pr_info("  magic: 0x%x\n", le32_to_cpu(sb->magic));
	pr_info("  major_ver: %u\n", le16_to_cpu(sb->major_ver));
	pr_info("  minor_ver: %u\n", le16_to_cpu(sb->minor_ver));
	pr_info("  log_sectorsize: %u\n", le32_to_cpu(sb->log_sectorsize));
	pr_info("  log_sectors_per_block: %u\n", le32_to_cpu(sb->log_sectors_per_block));
	pr_info("  log_blocksize: %u\n", le32_to_cpu(sb->log_blocksize));
	pr_info("  log_blocks_per_seg: %u\n", le32_to_cpu(sb->log_blocks_per_seg));
	pr_info("  segs_per_sec: %u\n", le32_to_cpu(sb->segs_per_sec));
	pr_info("  secs_per_zone: %u\n", le32_to_cpu(sb->secs_per_zone));
	pr_info("  checksum_offset: %u\n", le32_to_cpu(sb->checksum_offset));
	pr_info("  block_count: %llu\n", (unsigned long long)le64_to_cpu(sb->block_count));
	pr_info("  section_count: %u\n", le32_to_cpu(sb->section_count));
	pr_info("  segment_count: %u\n", le32_to_cpu(sb->segment_count));
	pr_info("  segment_count_ckpt: %u\n", le32_to_cpu(sb->segment_count_ckpt));
	pr_info("  segment_count_sit: %u\n", le32_to_cpu(sb->segment_count_sit));
	pr_info("  segment_count_nat: %u\n", le32_to_cpu(sb->segment_count_nat));
	pr_info("  segment_count_ssa: %u\n", le32_to_cpu(sb->segment_count_ssa));
	pr_info("  segment_count_main: %u\n", le32_to_cpu(sb->segment_count_main));
	pr_info("  segment0_blkaddr: %u\n", le32_to_cpu(sb->segment0_blkaddr));
	pr_info("  cp_blkaddr: %u\n", le32_to_cpu(sb->cp_blkaddr));
	pr_info("  sit_blkaddr: %u\n", le32_to_cpu(sb->sit_blkaddr));
	pr_info("  nat_blkaddr: %u\n", le32_to_cpu(sb->nat_blkaddr));
	pr_info("  ssa_blkaddr: %u\n", le32_to_cpu(sb->ssa_blkaddr));
	pr_info("  main_blkaddr: %u\n", le32_to_cpu(sb->main_blkaddr));
	pr_info("  root_ino: %u\n", le32_to_cpu(sb->root_ino));
	pr_info("  node_ino: %u\n", le32_to_cpu(sb->node_ino));
	pr_info("  meta_ino: %u\n", le32_to_cpu(sb->meta_ino));
	pr_info("  uuid: ");
	for (i = 0; i < 16; i++) {
		pr_cont("%02x", sb->uuid[i]);
		if (i < 15) {
			pr_cont("-");
		}
	}
	pr_info("\n");

    // char *vbuf;
	// vbuf = kmalloc(MAX_VOLUME_NAME, GFP_KERNEL);
	// if (!vbuf)
	// 	return;
    // int count = utf16s_to_utf8s(sb->volume_name,
	// 		ARRAY_SIZE(sb->volume_name),
	// 		UTF16_LITTLE_ENDIAN, vbuf, MAX_VOLUME_NAME);
    // if (count > 0)
    //     pr_info("Volume name: %.*s\n", count, vbuf);
    // else
    //     pr_info("Volume name: (empty)\n");

	pr_info("  extension_count: %u\n", le32_to_cpu(sb->extension_count));
	pr_info("  extension_list:\n");
	for (i = 0; i < F2FS_MAX_EXTENSION; i++) {
		pr_info("    [%d]: ", i);
		for (j = 0; j < F2FS_EXTENSION_LEN; j++) {
			if (sb->extension_list[i][j] == 0) {
				break;
			}
			pr_cont("%c", sb->extension_list[i][j]);
		}
		pr_info("\n");
	}

	pr_info("  cp_payload: %u\n", le32_to_cpu(sb->cp_payload));
	pr_info("  version: %s\n", sb->version);
	pr_info("  init_version: %s\n", sb->init_version);
	pr_info("  feature: 0x%x\n", le32_to_cpu(sb->feature));
	pr_info("  encryption_level: %u\n", sb->encryption_level);
	pr_info("  encrypt_pw_salt: ");
	for (i = 0; i < 16; i++) {
		pr_cont("%02x", sb->encrypt_pw_salt[i]);
		if (i < 15) {
			pr_cont("-");
		}
	}
	pr_info("\n");

	pr_info("  devs:\n");
	for (i = 0; i < MAX_DEVICES; i++) {
		pr_info("    [%d]:\n", i);
		pr_info("      path: %s\n", sb->devs[i].path);
		pr_info("      total_segments: %u\n", le32_to_cpu(sb->devs[i].total_segments));
	}

	pr_info("  qf_ino:\n");
	// for (i = 0; i < F2FS_MAX_QUOTAS; i++) {
	// 	pr_info("    [%d]: %u\n", i, le32_to_cpu(sb->qf_ino[i]));
	// }

	pr_info("  hot_ext_count: %u\n", sb->hot_ext_count);
	pr_info("  s_encoding: %u\n", le16_to_cpu(sb->s_encoding));
	pr_info("  s_encoding_flags: %u\n", le16_to_cpu(sb->s_encoding_flags));
	pr_info("  s_stop_reason: ");
	for (i = 0; i < MAX_STOP_REASON; i++) {
		pr_cont("%02x ", sb->s_stop_reason[i]);
	}
	pr_info("\n");

	pr_info("  s_errors: ");
	for (i = 0; i < MAX_F2FS_ERRORS; i++) {
		pr_cont("%02x ", sb->s_errors[i]);
	}
	pr_info("\n");

	pr_info("  crc: 0x%x\n", le32_to_cpu(sb->crc));
    // kfree(vbuf);
}

/**
 * print_filename_from_inode - 通过 inode 打印文件名
 * @inode: 指向 struct inode 结构体的指针
 *
 * 此函数会遍历与 inode 关联的所有 dentry，打印出每个 dentry 对应的文件名。
 * 由于一个 inode 可能有多个硬链接，所以可能会打印出多个文件名。
 */
void print_filename_from_inode(struct inode *inode)
{
    struct dentry *dentry;

    // 遍历 inode 的 dentry 列表
    hlist_for_each_entry(dentry, &inode->i_dentry, d_u.d_alias)
    {
        // 打印文件名
		if (dentry->d_name.name) {
            pr_info("Filename: %.*s\n", (int)dentry->d_name.len, dentry->d_name.name);
        }
    }
}
