#include "stack.h"

LinkedListStack *liststack=NULL;
ArrayStack *arraystack=NULL;


LinkedListStack *newLinkedListStack(void) {
    LinkedListStack *new=kmalloc(sizeof(LinkedListStack), GFP_KERNEL);
    new->top=NULL;
    new->size=0;
    return new;
}

void delLinkedListStack(LinkedListStack *stack) {
    while(stack->top) {
        ListNode *tmp=stack->top->next;
        kfree(stack->top);
        --stack->size;
        stack->top=tmp;
    }

    kfree(stack);
}

int sizeStack(LinkedListStack *stack) {
    return stack->size;
}

bool isEmptyStack(LinkedListStack *stack) {
    return sizeStack(stack)==0;
}

void pushStack(LinkedListStack *stack, int val) {
    ListNode *newNode=kmalloc(sizeof(ListNode),GFP_KERNEL);
    newNode->next=stack->top;
    newNode->val=val;
    stack->top=newNode;
    ++stack->size;
}

int peekStack(LinkedListStack *stack) {
    if(isEmptyStack(stack)) {
        printk(KERN_INFO "Stack is ");
        return INT_MAX;
    }

    return stack->top->val;
}

int popStack(LinkedListStack *stack) {
    int val=peekStack(stack);
    ListNode *tmp=NULL;

    if(val != INT_MAX) {
        tmp=stack->top;
        stack->top=stack->top->next;
        kfree(tmp);
        --stack->size;
    }
    
    return val;
}
// -------------------------------------------------------------
#define STACK_SIZE (128)

ArrayStack *newArrayStack(void) {
    ArrayStack *new=kmalloc(sizeof(ArrayStack),GFP_KERNEL);
    new->data=kmalloc(sizeof(int)*STACK_SIZE,GFP_KERNEL);
    new->size=0;
    return new;
}

void delArrayStack(ArrayStack *stack) {
    kfree(stack->data);
    kfree(stack);
}

int sizeArrayStack(ArrayStack *stack) {
    return stack->size;
}

bool isEmptyArrayStack(ArrayStack *stack) {
    return sizeArrayStack(stack)==0;
}

void pushArrayStack(ArrayStack *stack, int val) {
    if(stack->size==STACK_SIZE) {
        printk(KERN_INFO "ArrayStack is enought!\n");
        return ;
    }

    stack->data[stack->size]=val;
    ++stack->size;
}

int peekArrayStack(ArrayStack *stack) {
    if(isEmptyArrayStack(stack)) {
        printk(KERN_INFO "ArrayStack is null!\n");
        return INT_MAX;
    }

    return stack->data[stack->size-1];
}

int popArrayStack(ArrayStack *stack) {
    int val=peekArrayStack(stack);
    --stack->size;

    return val;
}


// -------------------------------------------------------------
// 定义模块加载函数
static int stack_init(void)
{
    printk(KERN_INFO "Hello, Stack!\n");
    liststack=newLinkedListStack();

    pushStack(liststack,1);
    pushStack(liststack,2);
    pushStack(liststack,3);

    printk(KERN_INFO "stack size:%d\n",sizeStack(liststack));
    printk(KERN_INFO "stack top:%d\n",peekStack(liststack));

    printk(KERN_INFO "stack pop:%d\n",popStack(liststack));
    printk(KERN_INFO "stack top:%d\n",peekStack(liststack));
    
    // -------------------------------------------------------

    arraystack=newArrayStack();
    pushArrayStack(arraystack,1);
    pushArrayStack(arraystack,2);
    pushArrayStack(arraystack,3);

    printk(KERN_INFO "arraystack size:%d\n",sizeArrayStack(arraystack));
    printk(KERN_INFO "arraystack top:%d\n",peekArrayStack(arraystack));

    printk(KERN_INFO "arraystack pop:%d\n",popArrayStack(arraystack));
    printk(KERN_INFO "arraystack top:%d\n",peekArrayStack(arraystack));

    return 0;  // 返回 0 表示加载成功
}

// 定义模块卸载函数
static void stack_exit(void)
{
    delLinkedListStack(liststack);
    delArrayStack(arraystack);
    printk(KERN_INFO "Goodbye, Stack!\n");
}

// 定义模块的初始化和退出函数
module_init(stack_init);
module_exit(stack_exit);

// 模块元信息
MODULE_LICENSE("GPL");       // 模块许可证，必须声明
MODULE_AUTHOR("Your Name");  // 模块作者
MODULE_DESCRIPTION("A simple stack kernel module.");  // 模块描述
MODULE_VERSION("0.1");       // 模块版本