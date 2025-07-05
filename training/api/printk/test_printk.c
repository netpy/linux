#define pr_fmt(fmt) KBUILD_MODNAME ": %s : %d :" fmt,__func__,__LINE__

#include <linux/init.h>    // 包含初始化宏
#include <linux/module.h>  // 包含模块相关宏
#include <linux/kernel.h>  // 包含 printk 等函数

// 定义模块加载函数
static int test_printk_init(void)
{
    int ret=0;
    void * ret_ptr=ERR_PTR(-1);
    pr_emerg("Hello, test_printk emerg!\n");
    pr_alert("Hello, test_printk alert!\n");
    pr_crit("Hello, test_printk crit!\n");
    pr_err("Hello, test_printk err!\n");
    pr_warn("Hello, test_printk warn!\n");
    pr_notice("Hello, test_printk notice!\n");
    pr_info("Hello, test_printk info!\n");
    pr_debug("Hello, test_printk debug!\n");
    pr_cont("Hello, test_printk cont!\n");
    // WARN(1,"This is WARN:");
    // WARN_ON(1);
    // BUG_ON(1);
    // BUG();
    for(int i=1;i<10;i++) {
        pr_info("i=%d\n",i);
        WARN_ON_ONCE(1<2);
    }
    
    // if(IS_ERR(ret)) {
    //     pr_info("IS_ERR:ret=%d",ret);
    // }

    if(IS_ERR(ret_ptr)) {
        pr_info("IS_ERR:ret_ptr=%p\n",ret_ptr);
    }

    ret=PTR_ERR(ret_ptr);
    pr_info("IS_ERR:ret=%d\n",ret);

    return 0;  // 返回 0 表示加载成功
}

// 定义模块卸载函数
static void test_printk_exit(void)
{
    pr_info("Goodbye, test_printk!\n");
}

// 定义模块的初始化和退出函数
module_init(test_printk_init);
module_exit(test_printk_exit);

// 模块元信息
MODULE_LICENSE("GPL");       // 模块许可证，必须声明
MODULE_AUTHOR("Your Name");  // 模块作者
MODULE_DESCRIPTION("A simple test_printk kernel module.");  // 模块描述
MODULE_VERSION("0.1");       // 模块版本