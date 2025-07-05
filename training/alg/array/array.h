#include <linux/init.h>    // 包含初始化宏
#include <linux/module.h>  // 包含模块相关宏
#include <linux/kernel.h>  // 包含 printk 等函数

void array_print(int *nums, int size);
void array_insert(int *nums, int size, int num, int index);
int array_find(int *nums, int size,int target);
int *array_extend(int *nums, int size, int enlarge);
