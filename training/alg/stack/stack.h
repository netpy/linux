#include <linux/init.h>    // 包含初始化宏
#include <linux/module.h>  // 包含模块相关宏
#include <linux/kernel.h>  // 包含 printk 等函数
// #include <linux/limits.h>

typedef struct ListNode {
    int val;
    struct ListNode *next;
} ListNode;

typedef struct {
    ListNode *top;
    int size;
} LinkedListStack;

LinkedListStack *newLinkedListStack(void);
void delLinkedListStack(LinkedListStack *stack);
int sizeStack(LinkedListStack *stack);
bool isEmptyStack(LinkedListStack *stack);
void pushStack(LinkedListStack *stack, int val);
int peekStack(LinkedListStack *stack);
int popStack(LinkedListStack *stack);

typedef struct {
    int *data;
    int size;
} ArrayStack;

ArrayStack *newArrayStack(void);
void delArrayStack(ArrayStack *stack);
int sizeArrayStack(ArrayStack *stack);
bool isEmptyArrayStack(ArrayStack *stack);
void pushArrayStack(ArrayStack *stack, int val);
int peekArrayStack(ArrayStack *stack);
int popArrayStack(ArrayStack *stack);

