#include "heap.h"

ListMaxHeap *newmaxHeap(int *nums,int size) {
    ListMaxHeap *maxHeap=kmalloc(sizeof(ListMaxHeap),GFP_KERNEL);
    maxHeap->size=size;
    memcpy(maxHeap->data, nums, maxHeap->size*sizeof(int));
    for(int i = parent(maxHeap,maxHeap->size-1);i>=0;i--) {
        printk("i=%d\n",i);
        siftDown(maxHeap, i);
    }
    return maxHeap;
}

void destroy(ListMaxHeap* maxHeap) {
    kfree(maxHeap);
}

int left(ListMaxHeap *maxHeap, int i) {
    return maxHeap->data[2*i+1];
}

int right(ListMaxHeap *maxHeap, int i) {
    return maxHeap->data[2*i+2];
}

int parent(ListMaxHeap *maxHeap, int i) {
    return maxHeap->data[(i-1)/2];
}

int peek(ListMaxHeap *maxHeap) {
    return maxHeap->data[0];
}

void push(ListMaxHeap *maxHeap, int val) {
    if(maxHeap->size==COUNT_NODE) {
        printk("Head is full!\n");
        return;
    }

    maxHeap->data[maxHeap->size]=val;
    maxHeap->size++;

    siftUp(maxHeap,maxHeap->size-1);
}

int pop(ListMaxHeap *maxHeap) {
    if(maxHeap->size==0) {
        printk("Heap is empty");
        return INT_MAX;
    }

    int val=maxHeap->data[0];
    maxHeap->data[0]=maxHeap->data[maxHeap->size-1];
    maxHeap->data[maxHeap->size-1]=val;
    maxHeap->size--;

    siftDown(maxHeap,0);

    return val;
}

void siftUp(ListMaxHeap *maxHeap, int i) {
    while(true) {
        int p=parent(maxHeap,i);
        if(p<0 || maxHeap->data[i] <= maxHeap->data[p])
            break;
        int tmp=maxHeap->data[i];
        maxHeap->data[i]=maxHeap->data[p];
        maxHeap->data[p]=tmp;
        i=p;
    }
}

void siftDown(ListMaxHeap *maxHeap, int i) {
    while(true) {
        int l=left(maxHeap,i);
        int r=right(maxHeap,i);
        int max=i;
        if(l<maxHeap->size && maxHeap->data[l] > maxHeap->data[max]) {
            max=l;
        }
        if(r<maxHeap->size && maxHeap->data[r] > maxHeap->data[max]) {
            max=r;
        }

        if(max==i)
            break;

        int val=maxHeap->data[max];
        maxHeap->data[max]=maxHeap->data[i];
        maxHeap->data[i]=val;

        i=max;
    }
}

void printListMaxHeap(ListMaxHeap *maxHeap) {
    printk("List ListMaxHeap:");
    for(int i=0;i<maxHeap->size;i++) {
        printk("%d ",maxHeap->data[i]);
    }
    printk("\n");
}

ListMaxHeap* maxHeap=NULL;
// 定义模块加载函数
static int heap_init(void)
{
    printk(KERN_INFO "Hello, heap!\n");
    int nums[20] ={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};

    maxHeap=newmaxHeap(nums,ARRAY_SIZE(nums));
    printListMaxHeap(maxHeap);




    return 0;  // 返回 0 表示加载成功
}

// 定义模块卸载函数
static void heap_exit(void)
{
    destroy(maxHeap);
    printk(KERN_INFO "Goodbye, heap!\n");
}

// 定义模块的初始化和退出函数
module_init(heap_init);
module_exit(heap_exit);

// 模块元信息
MODULE_LICENSE("GPL");       // 模块许可证，必须声明
MODULE_AUTHOR("Private");  // 模块作者
MODULE_DESCRIPTION("A simple Hello Heap kernel module.");  // 模块描述
MODULE_VERSION("0.1");       // 模块版本
