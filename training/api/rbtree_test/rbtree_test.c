#include "rbtree_test.h"

static struct rb_root my_rb_tree = RB_ROOT;
static struct rb_root_cached my_rb_tree_cached = RB_ROOT_CACHED;

struct std_info *test_rbtree_insert(struct rb_root *root, int id)
{
    struct rb_node **p=&root->rb_node;
    struct rb_node *parent = NULL;
    struct std_info *tmp=NULL;
    struct std_info *std = kmalloc(sizeof(struct std_info), GFP_KERNEL);
    
    std->id=id;
    RB_CLEAR_NODE(&std->rb_node);

    while(*p) {
        parent = *p;
        tmp = rb_entry(parent,struct std_info, rb_node);

        if(tmp->id > id)
            p=&parent->rb_left;
        else
            p=&parent->rb_right;
    }

    rb_link_node(&std->rb_node, parent, p);
    rb_insert_color(&std->rb_node, root);
    return std;
}

struct std_info *test_rbtree_cached_insert(struct rb_root_cached *root, int id)
{
    struct rb_node **pos=&root->rb_root.rb_node;
    struct rb_node *parent=NULL;
    struct std_info *tmp=NULL;
    struct std_info *std=kmalloc(sizeof(struct std_info), GFP_KERNEL);
    bool leftmost = true;
    std->id=id;
    RB_CLEAR_NODE(&std->rb_node);

    while(*pos) {
        parent=*pos;
        tmp=rb_entry(parent,struct std_info, rb_node);
        if(tmp->id>id)
            pos=&parent->rb_left;
        else{
            pos=&parent->rb_right;
            leftmost=false;
        }
    }

    rb_link_node(&std->rb_node,parent,pos);
    rb_insert_color_cached(&std->rb_node,root,leftmost);

    return std;
}

void reversal_rbtree(struct rb_root *root)
{
    struct rb_node *pos;

    /* 1. 取最左节点 */
    for(pos=rb_first(root);pos;pos=rb_next(pos)) {
        struct std_info *tmp=rb_entry(pos,struct std_info, rb_node);
        pr_info("std->id: %d\n", tmp->id);
    }
}

void reversal_rbtree_cached(struct rb_root_cached *root)
{
    struct rb_node *pos;
    for(pos=rb_first_cached(root);pos;pos=rb_next(pos)){
        struct std_info *tmp=rb_entry(pos,struct std_info, rb_node);
        pr_info("std->id: %d\n", tmp->id);
    }
}


void test_rb_replace(struct rb_root *root, int id)
{
    struct rb_node **pos=&root->rb_node;
    struct rb_node *parent=NULL;
    struct std_info *tmp=NULL;
    struct std_info *std=kmalloc(sizeof(struct std_info), GFP_KERNEL);
    std->id=id+10;
    RB_CLEAR_NODE(&std->rb_node);


    while(*pos) {
        parent = *pos;
        tmp=rb_entry(parent,struct std_info, rb_node);

        if(tmp->id > id)
            pos=&parent->rb_left;
        else
            pos=&parent->rb_right;
        
        if(tmp->id == id) {
            rb_replace_node(parent, &std->rb_node, root);
            kfree(tmp);
            return;
        }
    }
    return;
}

void rbtree_destroy(struct rb_root *root)
{
    struct rb_node *pos=NULL;
    struct std_info *tmp=NULL;

    while((pos=rb_first(root))) {
        tmp=rb_entry(pos, struct std_info, rb_node);
        pr_info("destroy std->id: %d\n", tmp->id);
        rb_erase(pos,root);
        if(tmp)
            kfree(tmp);
    }
    return;
}

void rbtree_cached_destroy(struct rb_root_cached *root)
{
    struct rb_node *pos=NULL;
    struct std_info *tmp=NULL;

    while((pos=rb_first_cached(root))) {
        tmp=rb_entry(pos, struct std_info, rb_node);
        pr_info("destroy std->id: %d\n", tmp->id);
        rb_erase_cached(pos,root);
        if(tmp)
            kfree(tmp);
    }
    return;
}

// 定义模块加载函数
static int rbtree_test_init(void)
{
    printk(KERN_INFO "Hello, rbtree_test!\n");
    test_rbtree_insert(&my_rb_tree,1);
    for(int i=2;i<10;i++)
        test_rbtree_insert(&my_rb_tree,i);
    reversal_rbtree(&my_rb_tree);
    test_rb_replace(&my_rb_tree,5);
    reversal_rbtree(&my_rb_tree);


    pr_info("Start test rb_node_cached!\n");
    for(int i=100;i<110;i++)
        test_rbtree_cached_insert(&my_rb_tree_cached,i);
    
    reversal_rbtree_cached(&my_rb_tree_cached);

    return 0;  // 返回 0 表示加载成功
}

// 定义模块卸载函数
static void rbtree_test_exit(void)
{
    printk(KERN_INFO "Goodbye, rbtree_test!\n");
    rbtree_destroy(&my_rb_tree);
    rbtree_cached_destroy(&my_rb_tree_cached);
}

// 定义模块的初始化和退出函数
module_init(rbtree_test_init);
module_exit(rbtree_test_exit);

// 模块元信息
MODULE_LICENSE("GPL");       // 模块许可证，必须声明
MODULE_AUTHOR("Your Name");  // 模块作者
MODULE_DESCRIPTION("A simple Hello rbtree_test kernel module.");  // 模块描述
MODULE_VERSION("0.1");       // 模块版本