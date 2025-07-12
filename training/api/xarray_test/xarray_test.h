#include <linux/init.h>    // 包含初始化宏
#include <linux/module.h>  // 包含模块相关宏
#include <linux/kernel.h>  // 包含 printk 等函数
#include <linux/xarray.h>  // 包含 xarray 相关函数
#include <linux/slab.h>  // 包含 kmalloc 等函数

#define MARK_USER0 0
#define MARK_USER1 1

struct std_info {
    int id;
    char name[20];
};

void test_xa_dump(const char *tag);
int test_xa_insert_and_erase(void);
void test_xa_destory(void);
int test_xa_store_and_load(void);
int test_xa_cmpxchg(void);
void test_xa_for_each(void);
void test_xa_for_each(void);
void test_xa_set_and_clear_mark(void);
void test_xa_alloc(void);
void test_xas_set_order(void);
void test_xa_reserve_and_release(void);
void test_xa_store_int(void);
