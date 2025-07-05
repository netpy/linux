#include "array.h"

static int my_array[]={1,2,3,4,5};
static int my_array_size=ARRAY_SIZE(my_array);
module_param_array(my_array,int,&my_array_size,0644);

void array_print(int *nums, int size)
{
    for(int i=0;i<size;++i)
        printk(KERN_INFO "nums[%d] = %d", i, nums[i]);

    return;
}

void array_insert(int *nums, int size, int num, int index)
{
    for(int i=size-1;i>index;--i)
        nums[i]=nums[i-1];

    nums[index] = num;
}

int array_find(int *nums, int size,int target)
{
    for(int i=0;i<size;++i) {
        if(nums[i] ==  target)
            return i;
    }

    return -1;
}

int *array_extend(int *nums, int size, int enlarge)
{
    int *res=(int *)kmalloc(sizeof(int)*(size+enlarge),GFP_KERNEL);

    for(int i=0;i<size;++i)
        res[i]=nums[i];

    for(int i=size;i<size+enlarge;++i)
        res[i]=0;

    return res;
}

// 定义模块加载函数
static int array_init(void)
{
    int nums_init[5] = {1,2,3,4,5};
    int nums[5] ={ 0 };
    int *new_nums=NULL;

    printk(KERN_INFO "module array start:\n");

    array_insert(nums_init,ARRAY_SIZE(nums_init),333,0);
    array_print(nums_init,ARRAY_SIZE(nums_init));

    array_print(nums, ARRAY_SIZE(nums));

    new_nums=array_extend(nums_init,ARRAY_SIZE(nums_init),3);
    array_print(new_nums,ARRAY_SIZE(nums_init)+3);
    
    if(new_nums!=NULL)
        kfree(new_nums);


    array_print(my_array,my_array_size);

    return 0;  // 返回 0 表示加载成功
}

// 定义模块卸载函数
static void array_exit(void)
{
    printk(KERN_INFO "module array exit!\n");
}

// 定义模块的初始化和退出函数
module_init(array_init);
module_exit(array_exit);

// 模块元信息
MODULE_LICENSE("GPL2.0");       // 模块许可证，必须声明
MODULE_AUTHOR("Priv@te");  // 模块作者
MODULE_DESCRIPTION("array test module");  // 模块描述
MODULE_VERSION("0.1");       // 模块版本
__MODULE_INFO(author,author_info,"zhangjiqing");
__MODULE_INFO(license,license_info,"sdafsidfhasf");
