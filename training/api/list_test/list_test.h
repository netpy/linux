#include <linux/init.h>    // 包含初始化宏
#include <linux/module.h>  // 包含模块相关宏
#include <linux/kernel.h>  // 包含 printk 等函数

static LIST_HEAD(list_test_head);
static LIST_HEAD(list1);

struct std_info {
    struct list_head val_list;
    int val;
};

static int my_array_param[] = {1,2,3,4,5};
static int my_array_param_size = ARRAY_SIZE(my_array_param);
module_param_array(my_array_param, int, &my_array_param_size, 0644);

static int my_array_param1[] = {11,22,33,44,55};
static int my_array_param_size1 = ARRAY_SIZE(my_array_param1);
module_param_array(my_array_param1, int, &my_array_param_size1, 0644);

void init_my_list(int *nums,struct list_head *head);
void printk_list(struct list_head *head);
void destroy_list(struct list_head *head);
