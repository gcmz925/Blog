## 关于 arm 系统 HOOK 系统调用表

参考文章：

https://blog.csdn.net/weixin_45030965/article/details/129203081

https://juejin.cn/post/6990646217399074853

Makefile

```
# Makefile for a Linux Kernel Module
obj-m := my_module.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

```



my_module.c

```c
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kallsyms.h> 
#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/ptrace.h> 

void (*update_mapping_prot)(phys_addr_t phys, unsigned long virt, phys_addr_t size, pgprot_t prot);

// .rodata segment 区间
unsigned long start_rodata;
unsigned long init_begin;

#define section_size init_begin - start_rodata

static unsigned long *__sys_call_table;

typedef long (*syscall_fn_t)(const struct pt_regs *regs);

#ifndef __NR_mkdirat
#define __NR_mkdirat 34
#endif

//用于保存原始的 mkdir 系统调用
static syscall_fn_t orig_mkdir;

asmlinkage long mkdir_hook(const struct pt_regs *regs)
{
    int ret;

    char filename[256] = {0};
    
    //获取第二个参数：const char __user *, pathname
    char __user *pathname = (char*)regs->regs[1];
    
    //返回原始系统调用
    ret = orig_mkdir(regs);
 
    printk("hook mkdir sys_call\n");
 
 	//从用户空间复制数据到内核空间
    if(copy_from_user(filename, pathname, 256))
    return -1;
   
    //打印创建的文件名
    printk("file name = %s\n", filename);

    return ret;
}

static inline void protect_memory(void)
{
	update_mapping_prot(__pa_symbol(start_rodata), (unsigned long)start_rodata,
			section_size, PAGE_KERNEL_RO);
}

static inline void unprotect_memory(void)
{
	update_mapping_prot(__pa_symbol(start_rodata), (unsigned long)start_rodata,
			section_size, PAGE_KERNEL);
}
	
static int __init lkm_init(void)
{
    update_mapping_prot = (void *)kallsyms_lookup_name("update_mapping_prot");

	start_rodata = (unsigned long)kallsyms_lookup_name("__start_rodata");
	init_begin = (unsigned long)kallsyms_lookup_name("__init_begin");

    __sys_call_table = (unsigned long *)kallsyms_lookup_name("sys_call_table");
    if (!__sys_call_table)
		return -1;

    printk("__sys_call_table = %lx\n", __sys_call_table);
    
    //保存原始的系统调用：mkdir
	orig_mkdir = (syscall_fn_t)__sys_call_table[__NR_mkdirat];

	//hook 系统调用表表项：sys_call_table[__NR_mkdirat]
    unprotect_memory();
    __sys_call_table[__NR_mkdirat] = (unsigned long)mkdir_hook;
    protect_memory();

    printk("lkm_init\n");

	return 0;
}

static void __exit lkm_exit(void)
{
	//模块卸载时恢复原来的mkdir系统调用
	unprotect_memory();
    __sys_call_table[__NR_mkdirat] = (unsigned long)orig_mkdir;
    protect_memory();

    printk("lkm_exit\n");
}

module_init(lkm_init);
module_exit(lkm_exit);

MODULE_LICENSE("GPL");

```





# 问题:

#### 1.框架驱动安装后直接卡死

问题原因:
	定义了宏 SC_ARM64_REGS_TO_ARGS 用来区分hook函数的入参是 

```c
asmlinkage long(*real_sys_unlinkat)(const struct pt_regs* regs) = NULL;	// 参数未分开
```

```c
asmlinkage long(*real_sys_unlinkat)(int dfd, const char __user * pathname, int flag) = NULL; // 参数分开
```

关于该宏的总结:

* SC_ARM64_REGS_TO_ARGS  $(K_K_PATH)/../arch/arm64/include/asm/syscall_wrapper.h 中
* SC_X86_64_REGS_TO_ARGS   ????????????  这个宏在哪里  ??????













