#include "data_type_switch.h"

void test_kstrtobool() {
    const char *bool_strings[] = {
        "y", "yes", "true", "on", "1",
        "n", "no", "false", "off", "0"
    };
    const int count = ARRAY_SIZE(bool_strings);
    bool val;

    for(int i=0;i<count;i++) {
        if(kstrtobool(bool_strings[i],&val)== 0) {
            if(val)
                pr_info("str=%s,bool=true;\n",bool_strings[i]);
            else
                pr_info("str=%s,bool=false;\n",bool_strings[i]);
        }
    }
}

void test_isspace() {
    char str[]={ ' ', '\t', '\n', '\f', '\r', '\v', 'a', 'b'};
    int count =ARRAY_SIZE(str);

    for(int i=0;i<count;i++) {
        if(isspace(str[i]))
            pr_info("This is space!\n");
        else
            pr_info("This is not space,str=%c!\n",str[i]);
    }
}

void test_isdigit() {
    char str[]={'0','1','2','3','4','5','6','7','8','9','a','b'};
    int count =ARRAY_SIZE(str);

    for(int i=0;i<count;i++) {
        if(isdigit(str[i]))
            pr_info("This is digit,str=%c!\n",str[i]);
        else
            pr_info("This is not digit,str=%c!\n",str[i]);
    }    
}

void test_isalpha() {
    char str[]={'0','1','2','3','4','5','6','7','8','9','a','A'};
    int count =ARRAY_SIZE(str);

    for(int i=0;i<count;i++) {
        if(isalpha(str[i]))
            pr_info("This is alpha,str=%c!\n",str[i]);
        else
            pr_info("This is not alpha,str=%c!\n",str[i]);
    }    
}

void test_isalnum() {
    char str[]={'0','1','2','3','4','5','6','7','8','9','a','A',' ', '.'};
    int count =ARRAY_SIZE(str);

    for(int i=0;i<count;i++) {
        if(isalnum(str[i]))
            pr_info("This is alnum,str=%c!\n",str[i]);
        else
            pr_info("This is not alnum,str=%c!\n",str[i]);
    }    
}

void test_tolower() {
     char str[]={'0','1','2','3','4','5','6','7','8','9','a','A',' ', '.'};
    int count =ARRAY_SIZE(str);

    for(int i=0;i<count;i++) {
        pr_info("str=%c!\n",tolower(str[i]));

    }     
}

void test_toupper() {
     char str[]={'0','1','2','3','4','5','6','7','8','9','a','A',' ', '.'};
    int count =ARRAY_SIZE(str);

    for(int i=0;i<count;i++) {
        pr_info("str=%c!\n",toupper(str[i]));

    }     
}

// 定义模块加载函数
static int data_type_switch_init(void)
{
    printk(KERN_INFO "Hello, data_type_switch!\n");

    test_kstrtobool();

    test_isspace();

    test_isdigit();

    test_isalpha();

    test_isalnum();

    test_tolower();

    test_toupper();

    return 0;  // 返回 0 表示加载成功
}

// 定义模块卸载函数
static void data_type_switch_exit(void)
{
    printk(KERN_INFO "Goodbye, data_type_switch!\n");
}

// 定义模块的初始化和退出函数
module_init(data_type_switch_init);
module_exit(data_type_switch_exit);

// 模块元信息
MODULE_LICENSE("GPL");       // 模块许可证，必须声明
MODULE_AUTHOR("Your Name");  // 模块作者
MODULE_DESCRIPTION("A simple Hello data_type_switch kernel module.");  // 模块描述
MODULE_VERSION("0.1");       // 模块版本