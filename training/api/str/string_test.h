#include <linux/init.h>    // 包含初始化宏
#include <linux/module.h>  // 包含模块相关宏
#include <linux/kernel.h>  // 包含 printk 等函数

# define BUF_SIZE 12
void test_strcpy(char *buf,const char *num_str);
void test_strncpy(char *buf,const char *num_str);
void test_strcat(char *buf,const char *num_str);
void test_strncat(char *buf,const char *num_str);
void test_strsep_1(void);
void test_strsep_2(void);
void test_strsep_3(void);
void test_kstrtoint(void);
void test_kstrtoul(void);
void test_kstrtoull(void);
void test_sprintf(void);
void test_snprintf(void);
void test_scnprintf(void);
void test_memset(void);
void test_memcpy(void);
void test_memmove(void);
void test_memcmp(void);
void test_memchr(void);
void test_strscpy(void);
void test_strlcat(void);
void test_ptrtostr(void);
