#include "kfifo_test.h"

static DEFINE_KFIFO(nums, int, NUMS_SIZE);
// static DEFINE_KFIFO(class_1,struct std_info,1024);

void test_write_for_define_kfifo(int *num,struct kfifo *fifo) {
    int ret=0;
    pr_info("kfifo->in=%u\n",fifo->kfifo.in);
    pr_info("kfifo->out=%u\n",fifo->kfifo.out);
    pr_info("kfifo->mask=%u\n",fifo->kfifo.mask);
    pr_info("kfifo->esize=%u\n",fifo->kfifo.esize);
    
    ret=kfifo_in(fifo,num,sizeof(int));
    pr_info("ret=%d,sizeof_struct_std_info=%ld",ret,sizeof(int));
   
    pr_info("kfifo->in=%u\n",fifo->kfifo.in);
    pr_info("kfifo->out=%u\n",fifo->kfifo.out);
    pr_info("kfifo->make=%u\n",fifo->kfifo.mask);
    pr_info("kfifo->esize=%u\n",fifo->kfifo.esize);

}

void test_write_for_declare_kfifo(struct std_info *info,struct kfifo *fifo) {
    int ret=0;
    pr_info("kfifo->in=%u\n",fifo->kfifo.in);
    pr_info("kfifo->out=%u\n",fifo->kfifo.out);
    pr_info("kfifo->mask=%u\n",fifo->kfifo.mask);
    pr_info("kfifo->esize=%u\n",fifo->kfifo.esize);
    
    ret=kfifo_in(fifo,info,sizeof(struct std_info));
    pr_info("ret=%d,sizeof_struct_std_info=%ld",ret,sizeof(struct std_info));
   
    pr_info("kfifo->in=%u\n",fifo->kfifo.in);
    pr_info("kfifo->out=%u\n",fifo->kfifo.out);
    pr_info("kfifo->make=%u\n",fifo->kfifo.mask);
    pr_info("kfifo->esize=%u\n",fifo->kfifo.esize);

}

void test_kfifio_alloc_and_free(void) {
    struct std_class *class_1;
    int ret;
    struct std_info info= {
        .age = 10,
        .name = "qqqqq",
        .id = 1,
    };

    pr_info("test_kfifio_alloc_and_free:\n");

    class_1=kmalloc(sizeof(struct std_class),GFP_KERNEL);
    if(!class_1) {
        pr_err("Failed alloc memory!\n");
    }

    ret=kfifo_alloc(&class_1->std_info_kfifo,20*sizeof(struct std_info),GFP_KERNEL);
    if (ret) {
        pr_info("Failed to allocate FIFO: %d\n", ret);
        kfree(&class_1->std_info_kfifo);
        return;
    }   

    // 向fifo添加数据
    pr_info("Writing data to FIFO:\n");
    ret=kfifo_in(&class_1->std_info_kfifo, &info,sizeof(info));
    if(ret!=sizeof(info)) {
        pr_err("Failed to write data to FIFO\n");
    }

    struct std_info read_info;
    pr_info("Read data from FIFO:\n");
    ret=kfifo_out(&class_1->std_info_kfifo,&read_info,sizeof(read_info));
    if(ret!=sizeof(read_info)) {
        pr_err("Failed to read data from FIFO\n");
    }
    pr_info("ret_out=%u,age=%d,name=%s,id=%d\n",ret,read_info.age,read_info.name,read_info.id);

    kfifo_free(&class_1->std_info_kfifo);
    kfree(class_1);
}

void test_kfifo_init() {
    struct kfifo test_kfifo;
    struct std_info *buffer=kmalloc(10*sizeof(struct std_info),GFP_KERNEL);
    pr_info("test_kfifo_init");
    int ret=kfifo_init(&test_kfifo, buffer,10*sizeof(struct std_info));
    if(ret)
        pr_info("kfifo_init failed!\n");
    
    struct std_info info= {
        .age = 10,
        .name = "test_kfifo_init",
        .id = 1,
    };   
    
    // 向fifo添加数据
    pr_info("Writing data to FIFO:\n");
    ret=kfifo_in(&test_kfifo, &info,sizeof(info));
    if(ret!=sizeof(info)) {
        pr_err("Failed to write data to FIFO\n");
    }

    struct std_info read_info={0};
    pr_info("Read data from FIFO:\n");
    ret=kfifo_out(&test_kfifo,&read_info,sizeof(read_info));
    if(ret!=sizeof(read_info)) {
        pr_err("Failed to read data from FIFO\n");
    }
    pr_info("ret_out=%u,age=%d,name=%s,id=%d\n",ret,read_info.age,read_info.name,read_info.id);

    kfree(buffer);
}

void test_kfifo_add_and_del() {
    struct class class_1;
    struct std_info info= {
        .age = 1000,
        .name = "test_kfifo_add_and_del",
        .id = 1111,
    };   
 
    pr_info("sizeof(struct class)=%ld",sizeof(struct class));
        
    class_1.name="class_1";
    INIT_KFIFO(class_1.std_info_kfifo);

    // 向fifo添加数据
    pr_info("Writing data to FIFO:\n");
    int ret=kfifo_in(&class_1.std_info_kfifo, &info,sizeof(info));
    if(ret!=sizeof(info)) {
        pr_err("Failed to write data to FIFO\n");
    }

    struct std_info read_peek_info={0};
    pr_info("Read data from FIFO:\n");
    ret=kfifo_peek(&class_1.std_info_kfifo,&read_peek_info);
    if(!ret) {
        pr_err("Failed to read data from FIFO by kfifo_peek,ret=%d\n",ret);
    }
    pr_info("peek:ret_out=%u,age=%d,name=%s,id=%d\n",ret,read_peek_info.age,read_peek_info.name,read_peek_info.id);

    struct std_info read_info={0};
    pr_info("Read data from FIFO:\n");
    ret=kfifo_out(&class_1.std_info_kfifo,&read_info,sizeof(read_info));
    if(ret!=sizeof(read_info)) {
        pr_err("Failed to read data from FIFO\n");
    }
    pr_info("ret_out=%u,age=%d,name=%s,id=%d\n",ret,read_info.age,read_info.name,read_info.id);

}

void test_kfifo_put_and_get() {
    struct class class_1;        
    class_1.name="class_1";
    INIT_KFIFO(class_1.std_info_kfifo);

    pr_info("sizeof(struct class)=%ld",sizeof(struct class));

    // 向fifo添加数据
    struct std_info info_1= {
        .age = 20,
        .name = "test_kfifo_put_and_get",
        .id = 21,
    }; 
    pr_info("Writing data to FIFO:\n");
    int ret=kfifo_put(&class_1.std_info_kfifo, info_1);
    if(!ret) {
        pr_err("Failed to write data to FIFO\n");
    }

    struct std_info read_put_info;
    pr_info("Read data from FIFO:\n");
    ret=kfifo_get(&class_1.std_info_kfifo,&read_put_info);
    if(!ret) {
        pr_err("Failed to read data from FIFO\n");
    }
    pr_info("ret_out=%u,age=%d,name=%s,id=%d\n",ret,read_put_info.age,read_put_info.name,read_put_info.id);
}

void test_kfifo_status_check() {
    struct std_class *class_1;
    int ret;
    struct std_info info= {
        .age = 10,
        .name = "qqqqq",
        .id = 1,
    };

    pr_info("test_kfifio_alloc_and_free:\n");

    class_1=kmalloc(sizeof(struct std_class),GFP_KERNEL);
    if(!class_1) {
        pr_err("Failed alloc memory!\n");
    }

    ret=kfifo_alloc(&class_1->std_info_kfifo,20*sizeof(struct std_info),GFP_KERNEL);
    if (ret) {
        pr_info("Failed to allocate FIFO: %d\n", ret);
        kfree(&class_1->std_info_kfifo);
        return;
    }   

    // 向fifo添加数据
    pr_info("Writing data to FIFO:\n");
    ret=kfifo_in(&class_1->std_info_kfifo, &info,sizeof(info));
    if(ret!=sizeof(info)) {
        pr_err("Failed to write data to FIFO\n");
    }

    pr_info("kfifo used len = %u\n",kfifo_len(&class_1->std_info_kfifo));
    pr_info("kfifo avail len = %u\n",kfifo_avail(&class_1->std_info_kfifo));
    pr_info("kfifo total size = %u\n",kfifo_size(&class_1->std_info_kfifo));

    if(kfifo_is_empty(&class_1->std_info_kfifo))
        pr_info("class_1->std_info_kfifo is empty!\n");
    else
        pr_info("class_1->std_info_kfifo not is empty!\n");

    struct std_info read_info;
    pr_info("Read data from FIFO:\n");
    ret=kfifo_out(&class_1->std_info_kfifo,&read_info,sizeof(read_info));
    if(ret!=sizeof(read_info)) {
        pr_err("Failed to read data from FIFO\n");
    }
    pr_info("ret_out=%u,age=%d,name=%s,id=%d\n",ret,read_info.age,read_info.name,read_info.id);

    if(kfifo_is_empty(&class_1->std_info_kfifo))
        pr_info("class_1->std_info_kfifo is empty!\n");
    else
        pr_info("class_1->std_info_kfifo not is empty!\n");

    
    if(kfifo_is_full(&class_1->std_info_kfifo))
        pr_info("class_1->std_info_kfifo is full!\n");
    else
        pr_info("class_1->std_info_kfifo not is full!\n");
        

    kfifo_free(&class_1->std_info_kfifo);
    kfree(class_1);
}

void test_kfifo_reset() {
    struct std_class *class;
    int ret;
    struct std_info info= {
        .age = 10,
        .name = "test_kfifo_reset",
        .id = 20,
    };

    pr_info("test_kfifo_reset:\n");

    class=kmalloc(sizeof(struct std_class),GFP_KERNEL);
    if(!class) {
        pr_err("Failed alloc memory!\n");
    }

    ret=kfifo_alloc(&class->std_info_kfifo, 20*sizeof(struct std_info),GFP_KERNEL);
    if(ret) {
        pr_info("Failed to allocate FIFO: %d\n", ret);
        kfree(&class->std_info_kfifo);
        kfree(class);
        return;        
    }

    ret=kfifo_in(&class->std_info_kfifo,&info,sizeof(info));
    if(!ret) {
        pr_err("Failed to write data to FIFO\n");
    }

    pr_info("class->std_info_kfifo used:%u\n",kfifo_len(&class->std_info_kfifo));
    // kfifo_reset_out(&class->std_info_kfifo);
    kfifo_reset_out(&class->std_info_kfifo);
    pr_info("class->std_info_kfifo used:%u\n",kfifo_len(&class->std_info_kfifo));

    kfifo_free(&class->std_info_kfifo);
    kfree(class);
}

void test_kfifo_in_and_out_spinlocked() {
    struct std_class *class;
    int ret;
    struct std_info info= {
        .age = 10,
        .name = "test_kfifo_in_and_out_spinlocked",
        .id = 20,
    };

    pr_info("test_kfifo_in_and_out_spinlocked:\n");

    class=kmalloc(sizeof(struct std_class),GFP_KERNEL);
    if(!class) {
        pr_err("Failed alloc memory!\n");
    }

    spin_lock_init(&class->fifo_lock);
    ret=kfifo_alloc(&class->std_info_kfifo, 20*sizeof(struct std_info),GFP_KERNEL);
    if(ret) {
        pr_info("Failed to allocate FIFO: %d\n", ret);
        kfree(&class->std_info_kfifo);
        kfree(class);
        return;        
    }

    ret=kfifo_in_spinlocked(&class->std_info_kfifo,&info,sizeof(info),&class->fifo_lock);
    if(!ret) {
        pr_err("Failed to write data to FIFO\n");
    }

    struct std_info read_info={0};
    pr_info("Read data from FIFO:\n");
    ret=kfifo_out_spinlocked(&class->std_info_kfifo,&read_info,sizeof(read_info),&class->fifo_lock);
    if(ret!=sizeof(read_info)) {
        pr_err("Failed to read data from FIFO\n");
    }
    pr_info("ret_out=%u,age=%d,name=%s,id=%d\n",ret,read_info.age,read_info.name,read_info.id);

    kfifo_free(&class->std_info_kfifo);
    kfree(class);
}



// 定义模块加载函数
static int kfifo_test_init(void)
{
    printk(KERN_INFO "Hello, kfifo_test!\n");

    int num=1;
    unsigned int ret_out=0;
    struct std_info tmp;
    struct class class_1;
    struct std_info info= {
        .age = 10,
        .name = "qqqqq",
        .id = 1,
    };

    pr_info("sizeof(struct class)=%ld",sizeof(struct class));
        
    class_1.name="class_1";
    INIT_KFIFO(class_1.std_info_kfifo);

    test_write_for_define_kfifo(&num,(struct kfifo *)&nums);

    test_write_for_declare_kfifo(&info,(struct kfifo *)&class_1.std_info_kfifo);
    ret_out=kfifo_out(&class_1.std_info_kfifo, &tmp,sizeof(struct std_info));
    if (ret_out == sizeof(struct std_info)) {
        pr_info("ret_out=%u,age=%d,name=%s,id=%d\n",ret_out,tmp.age,tmp.name,tmp.id);
    } else {
        pr_info("Failed to read from kfifo, ret_out=%u\n", ret_out);
    }

    test_kfifio_alloc_and_free();
    test_kfifo_init();
    test_kfifo_add_and_del();
    test_kfifo_put_and_get();
    test_kfifo_status_check();
    test_kfifo_reset();
    test_kfifo_in_and_out_spinlocked();
    return 0;  // 返回 0 表示加载成功
}

// 定义模块卸载函数
static void kfifo_test_exit(void)
{
    printk(KERN_INFO "Goodbye, kfifo_test!\n");
}

// 定义模块的初始化和退出函数
module_init(kfifo_test_init);
module_exit(kfifo_test_exit);

// 模块元信息
MODULE_LICENSE("GPL");       // 模块许可证，必须声明
MODULE_AUTHOR("Your Name");  // 模块作者
MODULE_DESCRIPTION("A simple Hello kfifo_test kernel module.");  // 模块描述
MODULE_VERSION("0.1");       // 模块版本