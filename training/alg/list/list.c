#include "list.h"

ListNode *newListNode(int val)
{
    ListNode *node=NULL;

    node=kmalloc(sizeof(ListNode),GFP_KERNEL);
    node->val=val;
    node->next=NULL;
    return node;
}

void listTraversal(ListNode *head)
{
    while(head) {
        printk(KERN_INFO "val=%d\n",head->val);
        head=head->next;
    }

    return;
}

void listInsert(ListNode *prev,ListNode *next)
{
    next->next=prev->next;
    prev->next=next;
}

void listRemove(ListNode *prev,ListNode *target)
{
    prev->next=target->next;
    kfree(target);
}

ListNode *listAccess(ListNode *head, int index)
{
    for(int i=0;i<index;++i) {
        if(head==NULL)
            return NULL;
        head=head->next;
    }

    return head;
}

ListNode *listFind(ListNode *head,int target)
{
    while(head) {
        if(head->val==target)
            return head;
        head=head->next;
    }

    return NULL;
}

static int list_init(void)
{
    ListNode *head=NULL;
    
    ListNode *node1=newListNode(1);
    ListNode *node2=newListNode(2);
    ListNode *node3=newListNode(3);
    ListNode *node4=newListNode(4);
    ListNode *node5=newListNode(5);

    printk(KERN_INFO "module list start:\n");

    head=node1;
    node1->next=node2;
    node2->next=node3;
    node3->next=node4;
    node4->next=node5;

    listTraversal(head);


    ListNode *list_insert=newListNode(3333);
    listInsert(node3,list_insert);

    printk(KERN_INFO "Inserted:\n");
    listTraversal(head);

    printk(KERN_INFO "Remove:\n");
    listRemove(node2,node3);
    listTraversal(head);

    printk(KERN_INFO "Access:\n");
    ListNode *node333=listAccess(head,3);
    printk(KERN_INFO "node333->val=%d\n",node333->val);

    printk(KERN_INFO "Find:\n");
    ListNode *target_node=listFind(head,2);
    printk(KERN_INFO "target_node->val=%d\n",target_node->val);

    return 0;  // 返回 0 表示加载成功
}

// 定义模块卸载函数
static void list_exit(void)
{
    printk(KERN_INFO "module list exit!\n");
}

// 定义模块的初始化和退出函数
module_init(list_init);
module_exit(list_exit);

// 模块元信息
MODULE_LICENSE("GPL2.0");       // 模块许可证，必须声明
MODULE_AUTHOR("Priv@te");  // 模块作者
MODULE_DESCRIPTION("list test module");  // 模块描述
MODULE_VERSION("0.1");       // 模块版本
__MODULE_INFO(author,author_info,"zhangjiqing");
__MODULE_INFO(license,license_info,"sdafsidfhasf");
