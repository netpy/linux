#include <linux/init.h>    // 包含初始化宏
#include <linux/module.h>  // 包含模块相关宏
#include <linux/kernel.h>  // 包含 printk 等函数

struct std_info {
    int id;
    struct rb_node rb_node;
};

struct std_info *test_rbtree_insert(struct rb_root *root, int id);
void reversal_rbtree(struct rb_root *root);
void rbtree_destroy(struct rb_root *root);
void test_rb_replace(struct rb_root *root, int id);

struct std_info *test_rbtree_cached_insert(struct rb_root_cached *root, int id);
void reversal_rbtree_cached(struct rb_root_cached *root);
void rbtree_cached_destroy(struct rb_root_cached *root);
