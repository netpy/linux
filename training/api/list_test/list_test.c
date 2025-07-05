#include "list_test.h"

void init_my_list(int *nums,struct list_head *head) {
    for(int i=0;i<my_array_param_size;i++) {
        struct std_info *tmp=kmalloc(sizeof(struct std_info), GFP_KERNEL);
        INIT_LIST_HEAD(&tmp->val_list);
        tmp->val=nums[i];
        list_add_tail(&tmp->val_list,head);
    }
}


void printk_list(struct list_head *head) {
    struct std_info *pos=NULL;
    list_for_each_entry(pos,head,val_list) {
        pr_info("val=%d\n",pos->val);
    }
}

void destroy_list(struct list_head *head) {
    struct std_info *pos=NULL;
    struct std_info *tmp=NULL;

    if(list_empty(head))
        return;

    list_for_each_entry_safe(pos,tmp,head,val_list) {
        list_del_init(&pos->val_list);
        kfree(pos);
    }
}


// 定义模块加载函数
static int list_test_init(void)
{
    printk(KERN_INFO "Hello, list_test!\n");
    struct std_info *new_std=kmalloc(sizeof(struct std_info),GFP_KERNEL);
    INIT_LIST_HEAD(&new_std->val_list);
    new_std->val=11111;


    init_my_list(my_array_param,&list_test_head);
    init_my_list(my_array_param1,&list1);

    list_move(list1.next,&list_test_head);
    list_move_tail(list1.prev,&list_test_head);

    // struct list_head *
    list_splice_tail_init(&list1,&list_test_head);
    // INIT_LIST_HEAD(&list1);

    list_replace(list_test_head.next,&new_std->val_list);

    list_swap(list_test_head.prev,list_test_head.next);

    if(!list_empty(&list1))
        pr_info("list1 not empty!");
    else
        pr_info("list1 is empty!");
    
    if(!list_is_singular(&list1))
        pr_info("list1 not singular!");
    else
        pr_info("list1 is singular!");

    printk_list(&list_test_head);
    // printk_list(&list1);
    return 0;  // 返回 0 表示加载成功
}

// 定义模块卸载函数
static void list_test_exit(void)
{
    printk(KERN_INFO "Goodbye, list_test!\n");
    destroy_list(&list_test_head);
    destroy_list(&list1);
}

// 定义模块的初始化和退出函数
module_init(list_test_init);
module_exit(list_test_exit);

// 模块元信息
MODULE_LICENSE("GPL");       // 模块许可证，必须声明
MODULE_AUTHOR("Your Name");  // 模块作者
MODULE_DESCRIPTION("A simple Hello list_test kernel module.");  // 模块描述
MODULE_VERSION("0.1");       // 模块版本