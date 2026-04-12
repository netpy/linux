# SPDX-License-Identifier: GPL-2.0
# 根据内核配置选项决定是否将 F2FS 文件系统编译进内核
# 如果 CONFIG_F2FS_FS 被启用（设为 'y' 或 'm'），则将 f2fs.o 添加到构建目标
obj-$(CONFIG_F2FS_FS) += f2fs.o

# 定义 F2FS 文件系统的基本组件对象文件
# 这些是 F2FS 文件系统的核心功能模块：
# dir.o - 目录操作相关功能
# file.o - 文件操作相关功能  
# inode.o - inode 管理相关功能
# namei.o - 名称解析相关功能
# hash.o - 哈希表相关功能
# super.o - 超级块管理相关功能
# inline.o - 内联数据相关功能
# checkpoint.o - 检查点管理相关功能
# gc.o - 垃圾回收相关功能
# data.o - 数据管理相关功能
# node.o - 节点管理相关功能
# segment.o - 段管理相关功能
# recovery.o - 恢复相关功能
# shrinker.o - 内存收缩器相关功能
# extent_cache.o - 区段缓存相关功能
# sysfs.o - 系统文件系统接口相关功能
f2fs-y		:= dir.o file.o inode.o namei.o hash.o super.o inline.o
f2fs-y		+= checkpoint.o gc.o data.o node.o segment.o recovery.o
f2fs-y		+= shrinker.o extent_cache.o sysfs.o
# 如果启用了 F2FS 统计功能，则添加 debug.o
f2fs-$(CONFIG_F2FS_STAT_FS) += debug.o
# 如果启用了扩展属性功能，则添加 xattr.o
f2fs-$(CONFIG_F2FS_FS_XATTR) += xattr.o
# 如果启用了 POSIX 访问控制列表功能，则添加 acl.o
f2fs-$(CONFIG_F2FS_FS_POSIX_ACL) += acl.o
# 如果启用了文件系统完整性验证功能，则添加 verity.o
f2fs-$(CONFIG_FS_VERITY) += verity.o
# 如果启用了 F2FS 压缩功能，则添加 compress.o
f2fs-$(CONFIG_F2FS_FS_COMPRESSION) += compress.o
# 如果启用了 F2FS I/O 统计功能，则添加 iostat.o
f2fs-$(CONFIG_F2FS_IOSTAT) += iostat.o
