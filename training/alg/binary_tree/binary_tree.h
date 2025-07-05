#include <linux/init.h>    // 包含初始化宏
#include <linux/module.h>  // 包含模块相关宏
#include <linux/kernel.h>  // 包含 printk 等函数

typedef struct TreeNode {
    int val;
    int height;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

TreeNode *newTreeNode(int val) {
    TreeNode *node;

    node=(TreeNode *)kmalloc(sizeof(TreeNode),GFP_KERNEL);
    node->val=val;
    node->height=0;
    node->left=NULL;
    node->right=NULL;
    return node;
}
