#include "string_test.h"

void test_strcpy(char *buf,const char *num_str) {
    strcpy(buf,num_str);

    pr_info("buf=%s,len=%ld\n",buf,strnlen(buf,BUF_SIZE));     
}

void test_strncpy(char *buf,const char *num_str) {
    strncpy(buf,num_str,BUF_SIZE);

    pr_info("buf=%s,len=%ld\n",buf,strnlen(buf,BUF_SIZE));     
}

void test_strcat(char *buf,const char *num_str) {
    strcat(buf,num_str);
    pr_info("buf=%s,len=%ld\n",buf,strnlen(buf,BUF_SIZE));    
}

void test_strncat(char *buf,const char *num_str) {
    buf[11]='z';
    strncat(buf,num_str,BUF_SIZE-strnlen(buf,BUF_SIZE)-1);
    for(int i=0;i<BUF_SIZE+2;i++)
        pr_info("buf[%d]=%c\n",i,buf[i]);
    pr_info("buf=%s,len=%ld\n",buf,strlen(buf));    
}

void test_strsep_1() {
    char str[] = "apple,banana,orange";
    char *token, *rest = str;

    // 循环分割字符串
    while ((token = strsep(&rest, ",")) != NULL) {
        printk(KERN_INFO "Token: %s\n", token);
    }
    pr_info("str=%s\n",str);
}

void test_strsep_2() {
    char str[] = "a,,c";
    char *token, *rest = str;

    while ((token = strsep(&rest, ",")) != NULL) {
        printk(KERN_INFO "Token: '%s'\n", token);
    }
}

void test_strsep_3() {
    char str[] = "foo:bar;baz";
    char *token, *rest = str;

    while ((token = strsep(&rest, ":;")) != NULL) {
        printk(KERN_INFO "Token: %s\n", token);
    }
}

void test_kstrtoint() {
    const char *str="123";
    const char *str_hex="0x10";
    const char *str_1="123qwe";

    int num=0;
    int ret=0;
    
    ret=kstrtoint(str,10,&num);
    if(!ret) {
        pr_info("num=%d\n",num);
    } else {
        pr_info("ret=%d\n",ret);
    }

    ret=kstrtoint(str_hex,0,&num);
    if(!ret) {
        pr_info("num=%d\n",num);
    } else {
        pr_info("ret=%d\n",ret);
    }

    ret=kstrtoint(str_1,0,&num);
    if(!ret) {
        pr_info("num=%d\n",num);
    } else {
        pr_info("ret=%d\n",ret);
    }  
}

void test_kstrtoul() {
    const char *str="123";
    const char *str_hex="0x10";
    const char *str_1="123qwe";

    unsigned long num=0;
    int ret=0;
    
    ret=kstrtoul(str,10,&num);
    if(!ret) {
        pr_info("num=%lu\n",num);
    } else {
        pr_info("ret=%d\n",ret);
    }

    ret=kstrtoul(str_hex,0,&num);
    if(!ret) {
        pr_info("num=%lu\n",num);
    } else {
        pr_info("ret=%d\n",ret);
    }

    ret=kstrtoul(str_1,0,&num);
    if(!ret) {
        pr_info("num=%lu\n",num);
    } else {
        pr_info("ret=%d\n",ret);
    }  
}

void test_kstrtoull() {
    const char *str="123";
    const char *str_hex="0x10";
    const char *str_1="123qwe";

    unsigned long long num=0;
    int ret=0;
    
    ret=kstrtoull(str,10,&num);
    if(!ret) {
        pr_info("num=%llu\n",num);
    } else {
        pr_info("ret=%d\n",ret);
    }

    ret=kstrtoull(str_hex,0,&num);
    if(!ret) {
        pr_info("num=%llu\n",num);
    } else {
        pr_info("ret=%d\n",ret);
    }

    ret=kstrtoull(str_1,0,&num);
    if(!ret) {
        pr_info("num=%llu\n",num);
    } else {
        pr_info("ret=%d\n",ret);
    }  
}

void test_sprintf() {
    char *buf=kmalloc(128,GFP_KERNEL);
    int a=10;

    if(buf) {
        sprintf(buf,"a=%d",a);
        pr_info("%s\n",buf);
    }

    kfree(buf);
}

void test_snprintf() {
    char buf[10];

    int ret=snprintf(buf,sizeof(buf),"%s", "test_snprintf");
    pr_info("buf_len=%ld,buf=%s,ret=%d\n",strlen(buf),buf,ret);
}

void test_scnprintf() {
    char buf[10];

    int ret=scnprintf(buf,5,"%s","test_scnprintf");
    pr_info("buf_len=%ld,buf=%s,ret=%d\n",strlen(buf),buf,ret);
}

void test_memset() {
    char *buf=kmalloc(12,GFP_KERNEL);

    memset(buf,'c',5);
    pr_info("buf=%s\n",buf);

    memset(memset(buf, 'A', 5)+5, 'B', 5);
    pr_info("buf=%s\n",buf);
}

void test_memcpy() {
    char buf1[20] = "hello";
    char buf2[20], buf3[20];

    memcpy(memcpy(buf2, buf1, sizeof(buf1)), buf3, sizeof(buf3));  // 连续复制  
    pr_info("buf1=%s,buf2=%s\n",buf1,buf2);
}

void test_memmove() {
    char arr[11] = "abcdefghij";  // 索引 0-9
    pr_info("arr=%s\n",arr);
    //  将前 5 个字符复制到索引 2 开始的位置（src=arr, dest=arr+2, 重叠）
    memmove(arr + 2, arr, 5);
    // 结果：arr = "ababcdefgh"（原 arr[0-4] 复制到 arr[2-6]）
    pr_info("arr=%s\n",arr);
}

struct data {
    int a;
    char b[20];
};
void test_memcmp() {
    struct data d1 = {42, "test"};
    struct data d2 = {42, "test1"};
    int result = memcmp(&d1, &d2, sizeof(struct data));  // 返回 0（内容相同）

    pr_info("result=%d\n",result);
}

struct packet {
    unsigned char header[4];
    unsigned char payload[100];
};

void test_memchr() {
    struct packet p = {{0xAA, 0xBB, 0xCC, 0xDD}, {0}};
    unsigned char *header_end = memchr(p.header, 0xCC, sizeof(p.header));  // 查找 0xCC 的位置
    pr_info("header_end=%s\n",header_end);
}

void test_strscpy() {
    char dest[4];
    const char *src = "long string";

    strncpy(dest,src,sizeof(dest));
    pr_info("dest=%s\n",dest);

    strscpy(dest, src, sizeof(dest));  // dest = "lo\0"（截断为 3 字符 + '\0'）
    pr_info("dest=%s\n",dest);
}

void test_strlcat() {
    char dest[8] = "hello";
    const char *src = " world";

    // strncat(dest, src, 2);
    // pr_info("len=%ld,dest=%s\n",strlen(dest),dest);

    size_t len = strlcat(dest, src, sizeof(dest));
    // dest = "hello w\0"（截断为 7 字节 + '\0'），len = 11（表示完整长度）
    pr_info("len=%ld,dest=%s\n",len,dest);
}

void test_ptrtostr() {
    char *ptr=kmalloc(4096,GFP_KERNEL);
    pr_info("ptr=%p\n",ptr);
    char buf[64];

    int ret=snprintf(buf,sizeof(buf),"%p", ptr);
    pr_info("buf_len=%ld,buf=%s,ret=%d\n",strlen(buf),buf,ret);  
    kfree(ptr);
}

// 定义模块加载函数
static int string_test_init(void)
{
    printk(KERN_INFO "Hello, string_test!\n");
    char buf[BUF_SIZE]={'a',':','a',':','a',':','\0'};
    const char *str="hello, string_test!";
    const char *num_str="123456789abcdef";
    const char *str_1="basdafas";
    const char *str_2="zAsdB";
    int ret=0;

    pr_info("sizeof(buf)=%ld",sizeof(buf));
    pr_info("buf=%s,len=%ld\n",buf,strnlen(buf,BUF_SIZE));
    pr_info("str=%s,len=%ld\n",str,strlen(str));
    pr_info("num_str=%s,len=%ld\n",num_str,strlen(num_str));

    // test_strcpy(buf,num_str);
    // test_strncpy(buf,num_str);
    // test_strcat(buf,num_str);
    test_strncat(buf,num_str);

    ret=strcmp(str_1,str_2);
    if(ret)
        pr_info("str_1(%s)!=str_2(%s),ret=%d\n",str_1,str_2,ret);
    
    ret=strncmp(str_1,str_2,3);
    if(ret)
        pr_info("str_1(%s)!=str_2(%s),ret=%d\n",str_1,str_2,ret);
    else
        pr_info("str_1(%s)==str_2(%s),ret=%d\n",str_1,str_2,ret);

    ret=strcasecmp(str_1,str_2);
    if(ret)
        pr_info("str_1(%s)!=str_2(%s),ret=%d\n",str_1,str_2,ret);
    else
        pr_info("str_1(%s)==str_2(%s),ret=%d\n",str_1,str_2,ret);

    ret=strncasecmp(str_1,str_2,3);
    if(ret)
        pr_info("str_1(%s)!=str_2(%s),ret=%d\n",str_1,str_2,ret);
    else
        pr_info("str_1(%s)==str_2(%s),ret=%d\n",str_1,str_2,ret);


    char *dot=strchr(str_1,'a');
    pr_info("dot=%s\n",dot);

    dot=strrchr(str_1,'a');
    pr_info("dot=%s\n",dot);

    dot=strstr(str_1,"aab");
    pr_info("dot=%s\n",dot);

    dot=strpbrk(str_1,str_2);
    pr_info("dot=%s\n",dot);
    
    test_strsep_1();
    test_strsep_2();
    test_strsep_3();
    
    test_kstrtoint();

    test_kstrtoul();

    test_kstrtoull();

    test_sprintf();

    test_snprintf();

    test_scnprintf();

    test_memset();

    test_memcpy();

    test_memmove();

    test_memcmp();

    test_memchr();

    test_strscpy();

    test_strlcat();

    test_ptrtostr();

    return 0;  // 返回 0 表示加载成功
}

// 定义模块卸载函数
static void string_test_exit(void)
{
    printk(KERN_INFO "Goodbye, string_test!\n");
}

// 定义模块的初始化和退出函数
module_init(string_test_init);
module_exit(string_test_exit);

// 模块元信息
MODULE_LICENSE("GPL");       // 模块许可证，必须声明
MODULE_AUTHOR("Your Name");  // 模块作者
MODULE_DESCRIPTION("A simple Hello string_test kernel module.");  // 模块描述
MODULE_VERSION("0.1");       // 模块版本