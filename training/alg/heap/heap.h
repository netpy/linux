#include <linux/init.h>    // 包含初始化宏
#include <linux/module.h>  // 包含模块相关宏
#include <linux/kernel.h>  // 包含 printk 等函数
#include <linux/array_size.h>
#include <linux/string.h>

#define COUNT_NODE 256

typedef struct MaxHeap {
    int data[COUNT_NODE];
    int size;
} ListMaxHeap;

ListMaxHeap *newmaxHeap(int *nums,int size);
void destroy(ListMaxHeap* maxHeap);
int left(ListMaxHeap *maxHeap, int i);
int right(ListMaxHeap *maxHeap, int i);
int parent(ListMaxHeap *maxHeap, int i);
int peek(ListMaxHeap *maxHeap);
void push(ListMaxHeap *maxHeap, int val);
int pop(ListMaxHeap *maxHeap);
void siftUp(ListMaxHeap *maxHeap, int i);
void siftDown(ListMaxHeap *maxHeap, int i);
void printListMaxHeap(ListMaxHeap *maxHeap);
