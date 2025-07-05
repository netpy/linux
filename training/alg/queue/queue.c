#include "queue.h"

LinkedListQueue *newLinkedListQueue(void)
{
    LinkedListQueue *queue=kmalloc(sizeof(LinkedListQueue),GFP_KERNEL);
    queue->first=NULL;
    queue->rear=NULL;
    queue->size=0;

    return queue;
}

void delLinkedListQueue(LinkedListQueue *queue)
{
    while(queue->first!=NULL) {
        ListNode *tmp=queue->first;
        queue->first=queue->first->next;
        kfree(tmp);
    }

    kfree(queue);
}

int sizeLinkedListQueue(LinkedListQueue *queue)
{
    return queue->size;
}

bool isEmptyLinkedListQueue(LinkedListQueue *queue)
{
    return sizeLinkedListQueue(queue)==0;
}

void pushLinkedListQueue(LinkedListQueue *queue, int val)
{
    ListNode *node=kmalloc(sizeof(ListNode),GFP_KERNEL);
    node->val=val;
    node->next=NULL;

    if(queue->first == NULL) {
        queue->first=node;
        queue->rear=node;
    } else {
        queue->rear->next=node;
        queue->rear=node;
    }

    ++queue->size;
}

int peekLinkedListQueue(LinkedListQueue *queue)
{
    if(!isEmptyLinkedListQueue(queue))
        return queue->first->val;
    
    return INT_MAX;
}

int popLinkedListQueue(LinkedListQueue *queue)
{
    int val=peekLinkedListQueue(queue);
    if(val!=INT_MAX) {
        ListNode *tmp=queue->first;
        queue->first=queue->first->next;
        kfree(tmp);
        --queue->size;
        if(!sizeLinkedListQueue(queue)) {
            queue->rear=NULL;
        }
    }
    return val;
}

void printLinkedListQueue(LinkedListQueue *queue)
{
    ListNode *head=queue->first;
    while(head!=NULL) {
        printk(KERN_INFO "val=%d\n", head->val);
        head=head->next;
    }

    return ;
}

// -------------------------------------------------------------------

ArrayQueue *newArrayQueue(int capacity) {
    ArrayQueue *queue=kmalloc(sizeof(ArrayQueue),GFP_KERNEL);
    queue->nums=kmalloc(sizeof(int)*capacity,GFP_KERNEL);
    queue->first=0;
    queue->size=0;
    queue->capacity=capacity;

    return queue;
}

void delArrayQueue(ArrayQueue *queue)
{
    kfree(queue->nums);
    kfree(queue);
}

int sizeArrayQueue(ArrayQueue *queue)
{
    return queue->size;
}

bool isEmptyArrayQueue(ArrayQueue *queue)
{
    return sizeArrayQueue(queue)==0;
}
void pushArrayQueue(ArrayQueue *queue, int val)
{
    if(queue->size==queue->capacity){
        printk(KERN_ERR "ArrayQueue is enought!");
        return;
    }

    int rear=(queue->first+queue->size) % queue->capacity;
    queue->nums[rear]=val;
    ++queue->size;
}
int peekArrayQueue(ArrayQueue *queue)
{
    if(queue->size!=0)
        return queue->nums[queue->first];

    return INT_MAX;
}

int popArrayQueue(ArrayQueue *queue){
    int num=peekArrayQueue(queue);
    if(num!=INT_MAX) {
        queue->first=(queue->first+1)%queue->capacity;
        --queue->size;
    }

    return num;
}

void printArrayQueue(ArrayQueue *queue)
{
    if(isEmptyArrayQueue(queue))
        printk(KERN_INFO "queue is NULL, not need printk!");
    
    for(int i=0;i<queue->size;++i) {
        printk(KERN_INFO "val=%d\n", queue->nums[(queue->first+i)%queue->capacity]);
    }

    return;
}

LinkedListQueue *queue=NULL;
ArrayQueue *arrayqueue=NULL;

// 定义模块加载函数
static int queue_init(void)
{
    printk(KERN_INFO "Hello, queue!\n");

    queue=newLinkedListQueue();
    if(isEmptyLinkedListQueue(queue)) {
        printk(KERN_INFO "queue is empty\n");
    }

    pushLinkedListQueue(queue,1);
    pushLinkedListQueue(queue,2);
    pushLinkedListQueue(queue,3);

    printk(KERN_INFO "print queue:\n");
    printLinkedListQueue(queue);

    printk(KERN_INFO "queue first val=%d\n", peekLinkedListQueue(queue));

    printk(KERN_INFO "pop queue first val=%d\n", popLinkedListQueue(queue));
    printk(KERN_INFO "queue first val=%d\n", peekLinkedListQueue(queue));

    // ---------------------------------------------------------------

    arrayqueue=newArrayQueue(3);
    if(isEmptyArrayQueue(arrayqueue)){
        printk(KERN_INFO "arrayqueue is empty!\n");
    }
    pushArrayQueue(arrayqueue,1);
    pushArrayQueue(arrayqueue,2);
    pushArrayQueue(arrayqueue,3);

    printk(KERN_INFO "print arrayqueue:\n");
    printArrayQueue(arrayqueue);

    printk(KERN_INFO "arrayqueue first val=%d\n", peekArrayQueue(arrayqueue));

    printk(KERN_INFO "pop arrayqueue first val=%d\n", popArrayQueue(arrayqueue));
    printk(KERN_INFO "arrayqueue first val=%d\n", peekArrayQueue(arrayqueue));

    pushArrayQueue(arrayqueue,4);
    printk(KERN_INFO "print arrayqueue:\n");
    printArrayQueue(arrayqueue); 

    return 0;  // 返回 0 表示加载成功
}

// 定义模块卸载函数
static void queue_exit(void)
{
    printk(KERN_INFO "Goodbye, queue!\n");
    delLinkedListQueue(queue);
    delArrayQueue(arrayqueue);
}

// 定义模块的初始化和退出函数
module_init(queue_init);
module_exit(queue_exit);

// 模块元信息
MODULE_LICENSE("GPL");       // 模块许可证，必须声明
MODULE_AUTHOR("Your Name");  // 模块作者
MODULE_DESCRIPTION("A simple queue kernel module.");  // 模块描述
MODULE_VERSION("0.1");       // 模块版本