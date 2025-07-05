#include <linux/init.h>    // 包含初始化宏
#include <linux/module.h>  // 包含模块相关宏
#include <linux/kernel.h>  // 包含 printk 等函数
#include <linux/kfifo.h>

#define NUMS_SIZE 2048

struct std_info {
    int age;
    const char *name;
    int id;
};

struct class {
    const char *name;
    DECLARE_KFIFO(std_info_kfifo,struct std_info,32);
};


struct std_class {
    struct kfifo std_info_kfifo; 
    spinlock_t fifo_lock;
};

void test_write_for_define_kfifo(int *num,struct kfifo *fifo);
void test_write_for_declare_kfifo(struct std_info *info,struct kfifo *fifo);
void test_kfifio_alloc_and_free(void);
void test_kfifo_init(void);
void test_kfifo_add_and_del(void);
void test_kfifo_put_and_get(void);
void test_kfifo_status_check(void);
void test_kfifo_status_check(void);
void test_kfifo_reset(void);
void test_kfifo_in_and_out_spinlocked(void);
