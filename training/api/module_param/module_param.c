#include <linux/init.h>    // 包含初始化宏
#include <linux/module.h>  // 包含模块相关宏
#include <linux/kernel.h>  // 包含 printk 等函数

static int my_init_param = 10;
// module_param(my_init_param, int, 0644);
module_param_named(my_init_param_out,my_init_param,int,0644);

static char *my_string_param = "Test module_param!";
module_param(my_string_param, charp, 0644);

static long my_long_param = 123456;
module_param(my_long_param, long ,0644);

static bool my_bool_param = false;
module_param(my_bool_param, bool ,0644);

static int my_array_param[] = {1,2,3};
static int my_array_param_size = ARRAY_SIZE(my_array_param);
module_param_array(my_array_param, int, &my_array_param_size, 0644);


static int my_debug=0;
static int custom_callback_function(const char *val, const struct kernel_param *kp) {
    int res=param_set_int(val,kp);
    if(res==0) {
        printk(KERN_INFO "Call back function called...\n");
        printk(KERN_INFO "New value of debug=%d\n",my_debug);
        return 0;
    }
    return -1;
}

const struct kernel_param_ops my_param_oops= {
    .set = &custom_callback_function,
    .get = &param_get_int,
};

module_param_cb(my_debug,&my_param_oops, &my_debug, S_IRUGO | S_IWUSR);

// 定义模块加载函数
static int module_param_init(void)
{
    printk(KERN_INFO "Hello, module_param!\n");

    printk(KERN_INFO "Interger maram:%d\n", my_init_param);
    printk(KERN_INFO "String param:%s\n", my_string_param);

    printk(KERN_INFO "long param:%ld\n", my_long_param);
    printk(KERN_INFO "bool param:%d\n", my_bool_param);

    printk(KERN_INFO "Array parameters:");
    for(int i=0;i<my_array_param_size;i++) {
        printk(KERN_CONT "%d ", my_array_param[i]);
    }
    printk(KERN_CONT "\n");

    printk(KERN_INFO "interesting param debug: %d\n", my_debug);

    return 0;  // 返回 0 表示加载成功
}

// 定义模块卸载函数
static void module_param_exit(void)
{
    printk(KERN_INFO "Goodbye, module_param!\n");
    printk(KERN_INFO "bye... debug=%d\n", my_debug);
}

// 定义模块的初始化和退出函数
module_init(module_param_init);
module_exit(module_param_exit);

// 模块元信息
MODULE_LICENSE("GPL");       // 模块许可证，必须声明
MODULE_AUTHOR("zhangjiqing");  // 模块作者
MODULE_DESCRIPTION("A simple module_param kernel module.");  // 模块描述
MODULE_VERSION("0.1");       // 模块版本