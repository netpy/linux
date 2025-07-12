#include "xarray_test.h"

static struct xarray std_xarray;

void test_xa_dump(const char *tag)
{
    unsigned long index;
    struct std_info *entry;

    pr_info("=== %s ===\n", tag);
    xa_for_each(&std_xarray, index, entry)
        pr_info("[%lu] = %s \n", index, entry->name);
}

int test_xa_insert_and_erase()
{
    int ret;
    struct std_info *info;

    for(unsigned long index=10; index<13; index++) {
        info =  kzalloc(sizeof(*info),GFP_KERNEL);
        if(!info)
            return -ENOMEM;
        
        info->id=index;
        snprintf(info->name, sizeof(info->name), "std_info_%lu", index);

        ret = xa_insert(&std_xarray,index, info, GFP_KERNEL);
        if(ret) {
            pr_info("Failed to insert index=%lu\n", index);
            kfree(info);
            return ret;
        }
    }

    test_xa_dump("insert after");

    info=NULL;
    info=xa_erase(&std_xarray, 12);
    if(info) {
        pr_info("erase: %s\n", info->name);
        kfree(info);
    }

    test_xa_dump("erase after");

    return 0;
}

void test_xa_destory()
{
    struct std_info *info;
    unsigned long index;

    xa_for_each(&std_xarray,index,info)
        kfree(info);
    
    xa_destroy(&std_xarray);
}


int test_xa_store_and_load()
{
    struct std_info *stu1, *stu2, *old;

    stu1 = kzalloc(sizeof(*stu1),GFP_KERNEL);
    if(!stu1)
        return -ENOMEM;
    stu1->id=42;
    snprintf(stu1->name,sizeof(stu1->name), "stu1");
    old=xa_store(&std_xarray, 42, stu1, GFP_KERNEL);
    if(old) {
        pr_err("BUG: idx 42 already had data!\n");
        kfree(old);
    }

    stu1=NULL;
    stu1=xa_load(&std_xarray, 42);
    if(stu1)
        pr_info("load 42: %s\n",stu1->name);

    test_xa_dump("store and load stu1 after:");

    stu2=kzalloc(sizeof(*stu2),GFP_KERNEL);
    if(!stu2)
        return -ENOMEM;
    
    stu2->id=42;
    snprintf(stu2->name,sizeof(stu2->name), "stu2");
    old=xa_store(&std_xarray, 42, stu2, GFP_KERNEL);
    if(old) {
        pr_info("replace 42:%s\n",((struct std_info *)old)->name);
        kfree(old);
    }

    stu2=NULL;
    stu2=xa_load(&std_xarray,42);
    if(stu2)
        pr_info("load 42 after replace: %s\n",((struct std_info *)stu2)->name);

    test_xa_dump("replace stu1 after:");

    return 0;
}

int test_xa_cmpxchg()
{
    struct std_info *stu1,*stu2,*old;
    int ret=0;

    stu1=kzalloc(sizeof(*stu1),GFP_KERNEL);
    if(!stu1)
        return -ENOMEM;
    
    stu1->id=52;
    snprintf(stu1->name,sizeof(stu1->name),"xa_cmpchg_stu1");
    
    ret=xa_insert(&std_xarray,52,stu1,GFP_KERNEL);
    if(ret) {
        kfree(stu1);
        return ret;
    }

    test_xa_dump("test_cmpchg after insert:");

    stu2=kzalloc(sizeof(*stu2),GFP_KERNEL);
    if(!stu2)
        return -ENOMEM;
    
    stu2->id=52;
    snprintf(stu2->name,sizeof(stu2->name),"xa_cmpchg_stu2");
    
    old=xa_cmpxchg(&std_xarray,52,stu1,stu2,GFP_KERNEL);
    if(old!=stu1) {
        pr_info("cmpchg failed: old=%s, new=%s\n",old->name,stu2->name);
    } else {
        pr_info("cmpchg success: old=%s, new=%s\n",old->name,stu2->name);
        kfree(old);
    }

    test_xa_dump("test_cmpchg after cmpxchg:");

    return 0;
}

void test_xa_for_each()
{
    struct std_info *entry;
    unsigned long index=0;

    pr_info("=== xa_for_each_start:20->end ===\n");
    xa_for_each_start(&std_xarray,index,entry,20)
        pr_info("[%lu] = %s \n", index, entry->name);
    
    pr_info("=== xa_for_each_range:20->50 ===\n");
    xa_for_each_range(&std_xarray,index,entry,20,50)
        pr_info("[%lu] = %s \n", index, entry->name);
    
}

void test_xa_set_and_clear_mark()
{
    struct std_info *stu,*entry;
    unsigned long index;

    for(int i=60;i<70;i++) {
        stu=kzalloc(sizeof(*stu),GFP_KERNEL);
        if(!stu)
            return;
        stu->id=i;
        snprintf(stu->name,sizeof(stu->name),"stu_%d",i);
        if(xa_insert(&std_xarray,i,stu,GFP_KERNEL)) {
            return;
        }
    }

    test_xa_dump("test_xa_set_and_clear_mark after insert:\n");

    xa_for_each_range(&std_xarray,index,entry,60,70) {
        if(index%2==0)
            xa_set_mark(&std_xarray,index,MARK_USER0);
        else
            xa_set_mark(&std_xarray,index,MARK_USER1);
    }

    pr_info("=== check mark after set: ===\n");
    for(int i=60;i<70;i++) {
        if(xa_get_mark(&std_xarray,i,MARK_USER0))
            pr_info("%d is MARK_USER0\n",i);
        else if(xa_get_mark(&std_xarray,i,MARK_USER1))
            pr_info("%d is MARK_USER1\n",i);
        else
            pr_info("%d is not MARK_USER0\n",i);
    }   

    for(int i=60;i<70;i++) {
        if(i%3==0)
            xa_clear_mark(&std_xarray,i,MARK_USER0);
    }    

    pr_info("=== check mark after clear: ===\n");   
    for(int i=60;i<70;i++) {
        if(xa_get_mark(&std_xarray,i,MARK_USER0))
            pr_info("%d is MARK_USER0\n",i);
        else if(xa_get_mark(&std_xarray,i,MARK_USER1))
            pr_info("%d is MARK_USER1\n",i);
        else
            pr_info("%d is not MARK_USER0\n",i);
    }

    stu=NULL;
    unsigned long start=60;
    pr_info("=== xa_find for MARK_USER0: === \n");
    stu=xa_find(&std_xarray,&start,70,MARK_USER0);
    if(!stu) {
        pr_info("Don't find entry for set MARK_USER0!\n");
    } else {
        pr_info("find entry for set MARK_USER0: %s,index=%lu\n",stu->name,start);
    }
}

void test_xa_alloc()
{
    struct xarray tmp_xa;
    xa_init_flags(&tmp_xa,XA_FLAGS_ALLOC);
    struct std_info *std,*entry;
    unsigned int id;
    unsigned long index;
    int ret;

    for(int i=0;i<5;i++) {
        std=kzalloc(sizeof(*std),GFP_KERNEL);
        if(!std)
            return;
        
        ret=xa_alloc(&tmp_xa,&id,std,xa_limit_31b,GFP_KERNEL);
        if(ret) {
            pr_info("xa_alloc failed!\n");
            return;
        }
        
        std->id=id;
        snprintf(std->name,sizeof(std->name),"stu_%d",(int)id);
    }

    pr_info("=== xa_alloc after: === \n");
    xa_for_each(&tmp_xa,index,entry) {
        pr_info("id=%d, name=%s\n",entry->id,entry->name);
    }
    
}

void test_xas_set_order()
{
    struct std_info *std,*entry;
    int index=74;
    unsigned int order=2;
    
    std=kzalloc(sizeof(*std),GFP_KERNEL);
    if(!std)
        return;
    
    std->id=index;
    snprintf(std->name,sizeof(std->name),"stu_%d",index);

    XA_STATE(xas, &std_xarray,index);

    xas_set_order(&xas,index,order);

    xas_store(&xas, std);

    pr_info("=== order=%u, 2^order=%u slots merged ===\n",
		order, 1u << order);
    for (unsigned long i = 72; i < 72 + (1u << order); i++) {
        entry = xa_load(&std_xarray, i);
        if (entry == std)
            pr_info("slot[%lu] -> %s\n", i, entry->name);
        else
            pr_info("slot[%lu] -> NULL (unexpected!)\n", i);
    }
}

void test_xa_reserve_and_release()
{
    struct xarray tmp_xa;
    xa_init_flags(&tmp_xa,XA_FLAGS_ALLOC);
    struct std_info *std,*entry;
    unsigned int id;
    unsigned long index;
    int ret;

    ret=xa_reserve(&tmp_xa,2,GFP_KERNEL);
    if(ret) {
        pr_err("xa_reserve failed: %d\n", ret);
        return;
    }

    for(int i=0;i<5;i++) {
        std=kzalloc(sizeof(*std),GFP_KERNEL);
        if(!std)
            return;
        
        ret=xa_alloc(&tmp_xa,&id,std,xa_limit_31b,GFP_KERNEL);
        if(ret) {
            pr_info("xa_alloc failed!\n");
            return;
        }
        
        std->id=id;
        snprintf(std->name,sizeof(std->name),"stu_%d",(int)id);
    }

    pr_info("=== xa_reserve after: === \n");
    xa_for_each(&tmp_xa,index,entry) {
        pr_info("id=%d, name=%s\n",entry->id,entry->name);
    }

    xa_release(&tmp_xa,2);
    for(int i=5;i<10;i++) {
        std=kzalloc(sizeof(*std),GFP_KERNEL);
        if(!std)
            return;
        
        ret=xa_alloc(&tmp_xa,&id,std,xa_limit_31b,GFP_KERNEL);
        if(ret) {
            pr_info("xa_alloc failed!\n");
            return;
        }
        
        std->id=id;
        snprintf(std->name,sizeof(std->name),"stu_%d",(int)id);
    }

    pr_info("=== xa_relase after: === \n");
    xa_for_each(&tmp_xa,index,entry) {
        pr_info("id=%d, name=%s\n",entry->id,entry->name);
    }
}

void test_xa_store_int()
{
    int val=100;
    int ret;
    void *e;
    
    ret=xa_insert(&std_xarray,100,xa_mk_value(val),GFP_KERNEL);
    if(ret) {
        pr_err("xa_store failed: %d\n", ret);
        return;
    }

    e=xa_load(&std_xarray,100);
    if(xa_is_value(e))
        pr_info("=== stored value = %lu ===\n", xa_to_value(e));
    else
        pr_err("e not int!\n");
    
    xa_erase(&std_xarray,100);

    return;
}

// 定义模块加载函数
static int xarray_test_init(void)
{
    printk(KERN_INFO "Hello, xarray_test!\n");

    xa_init(&std_xarray);

    test_xa_insert_and_erase();

    test_xa_store_and_load();

    test_xa_cmpxchg();

    test_xa_for_each();

    test_xa_set_and_clear_mark();
    
    test_xa_alloc();

    test_xas_set_order();

    test_xa_reserve_and_release();

    test_xa_store_int();

    return 0;  // 返回 0 表示加载成功
}

// 定义模块卸载函数
static void xarray_test_exit(void)
{
    test_xa_destory();
    printk(KERN_INFO "Goodbye, xarray_test!\n");
}

// 定义模块的初始化和退出函数
module_init(xarray_test_init);
module_exit(xarray_test_exit);

// 模块元信息
MODULE_LICENSE("GPL");       // 模块许可证，必须声明
MODULE_AUTHOR("Your Name");  // 模块作者
MODULE_DESCRIPTION("A simple Hello xarray_test kernel module.");  // 模块描述
MODULE_VERSION("0.1");       // 模块版本