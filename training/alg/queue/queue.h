#include <linux/init.h>    // 包含初始化宏
#include <linux/module.h>  // 包含模块相关宏
#include <linux/kernel.h>  // 包含 printk 等函数

typedef struct ListNode {
    int val;
    struct ListNode *next;
} ListNode;

typedef struct LinkedListQueue {
    ListNode *first,*rear;
    int size; 
} LinkedListQueue;

LinkedListQueue *newLinkedListQueue(void);
void delLinkedListQueue(LinkedListQueue *queue);
int sizeLinkedListQueue(LinkedListQueue *queue);
bool isEmptyLinkedListQueue(LinkedListQueue *queue);
void pushLinkedListQueue(LinkedListQueue *queue, int val);
int peekLinkedListQueue(LinkedListQueue *queue);
int popLinkedListQueue(LinkedListQueue *queue);
void printLinkedListQueue(LinkedListQueue *queue);

// ----------------------------------------------------------
#define MAX_QUEUE (128);
typedef struct ArrayQueue {
    int *nums;
    int first;
    int size;
    int capacity;
} ArrayQueue;

ArrayQueue *newArrayQueue(int capacity);
void delArrayQueue(ArrayQueue *queue);
int sizeArrayQueue(ArrayQueue *queue);
bool isEmptyArrayQueue(ArrayQueue *queue);
void pushArrayQueue(ArrayQueue *queue, int val);
int peekArrayQueue(ArrayQueue *queue);
int popArrayQueue(ArrayQueue *queue);
void printArrayQueue(ArrayQueue *queue);

