#include "binary_tree.h"

// 定义模块加载函数
static int binary_tree_init(void)
{
    printk(KERN_INFO "Hello, binary_tree!\n");
    return 0;  // 返回 0 表示加载成功
}

// 定义模块卸载函数
static void binary_tree_exit(void)
{
    printk(KERN_INFO "Goodbye, binary_tree!\n");
}

// 定义模块的初始化和退出函数
module_init(binary_tree_init);
module_exit(binary_tree_exit);

// 模块元信息
MODULE_LICENSE("GPL");       // 模块许可证，必须声明
MODULE_AUTHOR("Your Name");  // 模块作者
MODULE_DESCRIPTION("A simple binary_tree kernel module.");  // 模块描述
MODULE_VERSION("0.1");       // 模块版本