#include <linux/init.h>    // 包含初始化宏
#include <linux/module.h>  // 包含模块相关宏
#include <linux/kernel.h>  // 包含 printk 等函数

// 定义模块加载函数
static int xarray_test_init(void)
{
    printk(KERN_INFO "Hello, xarray_test!\n");
    return 0;  // 返回 0 表示加载成功
}

// 定义模块卸载函数
static void xarray_test_exit(void)
{
    printk(KERN_INFO "Goodbye, xarray_test!\n");
}

// 定义模块的初始化和退出函数
module_init(xarray_test_init);
module_exit(xarray_test_exit);

// 模块元信息
MODULE_LICENSE("GPL");       // 模块许可证，必须声明
MODULE_AUTHOR("Your Name");  // 模块作者
MODULE_DESCRIPTION("A simple Hello xarray_test kernel module.");  // 模块描述
MODULE_VERSION("0.1");       // 模块版本