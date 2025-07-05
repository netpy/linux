#include <linux/init.h>    // 包含初始化宏
#include <linux/module.h>  // 包含模块相关宏
#include <linux/kernel.h>  // 包含 printk 等函数

typedef struct ListNode {
    int val;
    struct ListNode *next;
} ListNode;

ListNode *newListNode(int val);
void listTraversal(ListNode *head);
void listInsert(ListNode *prev,ListNode *next);
void listRemove(ListNode *prev,ListNode *target);
ListNode *listAccess(ListNode *head, int index);
ListNode *listFind(ListNode *head,int target);
