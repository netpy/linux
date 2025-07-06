#include <linux/module.h>
#include <linux/f2fs_fs.h>

#include "f2fs.h"

#define FUN_START() pr_info("(%-20s:%-6d) %s start: \n", __FILE__, __LINE__, __func__)
#define FUN_END() pr_info("(%-20s:%-6d) %s  end!\n", __FILE__, __LINE__, __func__)

void print_f2fs_super_block(const struct f2fs_super_block *sb);