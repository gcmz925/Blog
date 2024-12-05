## linux中ERR_PTR、PTR_ERR、IS_ERR和IS_ERR_OR_NULL



### 判断返回指针是否错误的内联函数
　　linux内核中判断返回指针是否错误的内联函数主要有：ERR_PTR、PTR_ERR、IS_ERR和IS_ERR_OR_NULL等。
　　其源代码见include/linux/err.h

```C

#include <linux/compiler.h>
#include <linux/types.h>

#include <asm/errno.h>

/*
 * Kernel pointers have redundant information, so we can use a
 * scheme where we can return either an error code or a normal
 * pointer with the same return value.
 *
 * This should be a per-architecture thing, to allow different
 * error and pointer decisions.
 */
#define MAX_ERRNO   4095

#ifndef __ASSEMBLY__

#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)-MAX_ERRNO)

static inline void * __must_check ERR_PTR(long error)
{
    return (void *) error;
}

static inline long __must_check PTR_ERR(__force const void *ptr)
{
    return (long) ptr;
}

static inline bool __must_check IS_ERR(__force const void *ptr)
{
    return IS_ERR_VALUE((unsigned long)ptr);
}

static inline bool __must_check IS_ERR_OR_NULL(__force const void *ptr)
{
    return !ptr || IS_ERR_VALUE((unsigned long)ptr);
}
```



　　理解IS_ERR()，首先理解要内核空间。所有的驱动程序都是运行在内核空间，内核空间虽然很大，但总是有限的，而在这有限的空间中，其最后一个page是专门保留的，也就是说一般人不可能用到内核空间最后一个page的指针。换句话说，你在写设备驱动程序的过程中，涉及到的任何一个指针，必然有三种情况：
1，有效指针；
2，NULL，空指针；
3，错误指针，或者说无效指针。
　　所谓的错误指针就是指其已经到达了最后一个page，即内核用最后一页捕捉错误。比如对于32bit的系统来说，内核空间最高地址0xffffffff，那么最后一个page就是指的0xfffff000~0xffffffff(假设4k一个page)，这段地址是被保留的。
　　内核空间为什么留出最后一个page？我们知道一个page可能是4k，也可能是更多，比如8k，但至少它也是4k，所以留出一个page出来就可以让我们把内核空间的指针来记录错误了。内核返回的指针一般是指向页面的边界(4k边界)，即ptr & 0xfff == 0。如果你发现你的一个指针指向这个范围中的某个地址，那么你的代码肯定出错了。
　　IS_ERR( )就是判断指针是否有错，如果指针并不是指向最后一个page，那么没有问题；如果指针指向了最后一个page，那么说明实际上这不是一个有效的指针，这个指针里保存的实际上是一种错误代码。
　　通常很常用的方法就是先用IS_ERR()来判断是否是错误，然后如果是，那么就调用PTR_ERR()来返回这个错误代码。因此，判断一个指针是不是有效的，可用如下的方式：

#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)-MAX_ERRNO)）
1
　　MAX_ERRNO是4095即0xfff，注意(unsigned long)-MAX_ERRNO并不是(unsigned long)减去MAX_ERRNO，而是对负数-MAX_ERRNO进行强制类型转化，即上面应该等价于为(unsigned long)（-MAX_ERRNO），-0xfff转换为无符号long型是0xfffff000,即：

#define IS_ERR_VALUE(x) unlikely(x >= 0xfffff000）
1
　　即判断是不是在（0xfffff000，0xffffffff)之间，在这个区间的话就是错误指针。因此，可以用IS_ERR()来判断内核函数的返回值是不是一个有效的指针。所以经常在内核中看见返回错误码的时候加个负号如 -ENOSYS。注意这里的unlikely意思是括号内的值很有可能是假，具体likely和unlikely用法见likely和unlikely。
　　ERR_PTR、PTR_ERR只是对错误进行转换。
　　IS_ERR_OR_NULL是判断指针是空指针或是错误指针，即上述2和3类型的指针。
　　注意函数前面还用到了__must_check ，__must_check 函数是指调用函数一定要处理该函数的返回值，否则编译器会给出警告。即返回的指针有可能是错误指针，一定要对返回的指针进行处理。
compiler-gcc.5中定义如下：

#define __must_check            __attribute__((warn_unused_result))
//警告结果未使用，也即是返回值未被处理
1
2
常见错误源定义
　　Linux内核中,出错有多种可能.关于Linux内核中的错误,参考include/asm-generic/errno-base.h文件:

   1#ifndef _ASM_GENERIC_ERRNO_BASE_H
   2#define _ASM_GENERIC_ERRNO_BASE_H
   3
   4#define EPERM            1      /* Operation not permitted */操作禁止
   5#define ENOENT           2      /* No such file or directory */文件或者目录不存在
   6#define ESRCH            3      /* No such process */相应的进程不存在
   7#define EINTR            4      /* Interrupted system call */中断系统调用
   8#define EIO              5      /* I/O error */I/O错误
   9#define ENXIO            6      /* No such device or address */相应设备或者地址不存在
  10#define E2BIG            7      /* Argument list too long */参数列表太长
  11#define ENOEXEC          8      /* Exec format error */Exec格式错误
  12#define EBADF            9      /* Bad file number */错误的文档编号
  13#define ECHILD          10      /* No child processes */没有子进程
  14#define EAGAIN          11      /* Try again */ 重试
  15#define ENOMEM          12      /* Out of memory */溢出内存
  16#define EACCES          13      /* Permission denied */没有权限
  17#define EFAULT          14      /* Bad address */错误的地址
  18#define ENOTBLK         15      /* Block device required */块设备需求
  19#define EBUSY           16      /* Device or resource busy */设备或者资源繁忙
  20#define EEXIST          17      /* File exists */文件已经存在
  21#define EXDEV           18      /* Cross-device link */交叉链接设备
  22#define ENODEV          19      /* No such device */没有对应的设备
  23#define ENOTDIR         20      /* Not a directory */不是路径
  24#define EISDIR          21      /* Is a directory */是路径，与上面意思相反
  25#define EINVAL          22      /* Invalid argument */无效参数
  26#define ENFILE          23      /* File table overflow */文件表溢出
  27#define EMFILE          24      /* Too many open files */打开的文件太多了
  28#define ENOTTY          25      /* Not a typewriter */非打印机
  29#define ETXTBSY         26      /* Text file busy */文本文件繁忙
  30#define EFBIG           27      /* File too large */文件太大
  31#define ENOSPC          28      /* No space left on device */没有足够的空间留给设备使用
  32#define ESPIPE          29      /* Illegal seek */非法定位
  33#define EROFS           30      /* Read-only file system */只读文件系统
  34#define EMLINK          31      /* Too many links */太多的链接
  35#define EPIPE           32      /* Broken pipe */管道中断
  36#define EDOM            33      /* Math argument out of domain of func */??
  37#define ERANGE          34      /* Math result not representable */非数字结果
  38
  39#endif 

　　最常见的几个是-EBUSY,-EINVAL,-ENODEV,-EPIPE,-EAGAIN,-ENOMEM.这些是每个体系结构里都有的,另外各个体系结构也都定义了自己的一些错误代码.这些东西当然也都是宏,实际上对应的是一些数字,这个数字就叫做错误号.
　　对于Linux内核来说,不管任何体系结构,最大的错误号不会超过4095.而4095又正好是比4k小1,即4096减1.而我们知道一个page可能是4k,也可能是更多,比如8k,但至少它也是4k,所以留出一个page出来就可以让我们把内核空间的指针来记录错误了.







# Linux 64位进程地址空间分布概况

对于Linux 64位系统，理论上，64bit内存地址可用空间为0x0000000000000000 ~ 0xFFFFFFFFFFFFFFFF（16位十六进制数），这是个相当庞大的空间，Linux实际上只用了其中一小部分（256T）。

Linux64位操作系统仅使用低47位，高17位做扩展（只能是全0或全1）。所以，实际用到的地址为空间为0x0000000000000000 ~ 0x00007FFFFFFFFFFF（user space）和0xFFFF800000000000 ~ 0xFFFFFFFFFFFFFFFF（kernel space）,其余的都是unused space。

user space 也就是用户区由以下几部分组成：代码段，数据段，BSS段，heap，stack

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/9256b843eca81f1953426b542de12a8d.png#pic_center)


在精细一些：一个进程在内存中的分配如下

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/7610bcf587f48987181ffb35e109303c.png#pic_center)







# [kmalloc、vmalloc、__get_free_pages()的区别]



## 一、分布位置上的区别：

kmalloc()和__get_free_pages()函数申请的内存位于物理内存的映射区域，而且在物理上也是连续的，它们与真实的物理地址只有一个固定的偏移，因此存在简单的线性关系；（3G+896M）（低端内存）；

vmalloc函数申请的虚拟内存与物理内存之间也没有简单的换算关系；（高端内存）（3G+896M以上的内存）；

 

[回到顶部](https://www.cnblogs.com/linhaostudy/p/7477370.html#_labelTop)

## 二、特性上的区别：

1、kmalloc()

void *kmalloc(size_t size, int flags);

kmalloc第一个参数是要分配块的大小，第二个参数为分配标志，用于控制kmalloc的行为；

最常用的分配标志是GFP_KERNEL，其含义是内核空间的进程中申请内存。kmalloc()的底层依赖__get_free_page()实现，分配标志的前缀GFP正好是底层函数的缩写。

kmalloc申请的是较小的连续的物理内存，内存物理地址上连续，虚拟地址上也是连续的，使用的是内存分配器slab的一小片。申请的内存位于物理内存的映射区域。其真正的物理地址只相差一个固定的偏移。可以用两个宏来简单转换__pa(address) { virt_to_phys()} 和__va(address) {phys_to_virt()}

使用kmalloc函数之后使用kfree函数；

 

2、__get_free_pages()

get_free_page()申请的内存是一整页，一页的大小一般是128K。

 从本质上讲，kmalloc和get_free_page最终调用实现是相同的，只不过在调用最终函数时所传的flag不同而已。

 

3、vmalloc()

vmalloc()一般用在只存在于软件中的较大顺序缓冲区分配内存，vmalloc()远大于__get_free_pages()的开销，为了完成vmalloc()，新的页表需要被建立。所以效率没有kmalloc和__get_free_page效率高；

 

[回到顶部](https://www.cnblogs.com/linhaostudy/p/7477370.html#_labelTop)

## 三、另外的一些东西：

**kmalloc()**

**用于申请较小的、连续的物理内存
****1. 以字节为单位进行分配，在<linux/slab.h>中
\2. void \*kmalloc(size_t size, int flags) 分配的内存物理地址上连续，虚拟地址上自然连续
\3. gfp_mask标志**：什么时候使用哪种标志？如下：
———————————————————————————————-
情形 相应标志
———————————————————————————————-
进程上下文，可以睡眠 GFP_KERNEL
进程上下文，不可以睡眠 GFP_ATOMIC
中断处理程序 GFP_ATOMIC
软中断 GFP_ATOMIC
Tasklet GFP_ATOMIC
用于DMA的内存，可以睡眠 GFP_DMA | GFP_KERNEL
用于DMA的内存，不可以睡眠 GFP_DMA | GFP_ATOMIC
———————————————————————————————-

 

kzalloc函数

用kzalloc申请内存的时候， 效果等同于先是用 kmalloc() 申请空间 , 然后用 memset() 来初始化 ,所有申请的元素都被初始化为 0.

 对应的释放函数也是kfree函数；





### vmalloc基本知识

- vmalloc用于分配虚拟地址连续(物理地址不连续)的内存空间，vzmalloc相对于vmalloc多了个0初始化;
- vmalloc/vzmalloc分配的虚拟地址范围在VMALLOC_START/VMALLOC_END之间，属于堆内存;
- linux管理vmalloc分别有两个数据结构: [vm_struct](https://pzh2386034.github.io/Black-Jack/linux-memory/2019/09/05/ARM64内存管理十二-vmalloc内存机制/#vm_struct), [vm_area_struct](https://pzh2386034.github.io/Black-Jack/linux-memory/2019/09/05/ARM64内存管理十二-vmalloc内存机制/#vm_area_struct)；前者是内核虚拟地址空间的映射，后者是应用进程虚拟地址空间映射;
- 内核vmalloc区具体地址空间的管理是通过[vmap_area](https://pzh2386034.github.io/Black-Jack/linux-memory/2019/09/05/ARM64内存管理十二-vmalloc内存机制/#vmap_area)管理的，该结构体记录整个区间的起始和结束;
- vmalloc是内核出于自身的目的使用高端内存页的少数情形之一
- vmalloc在申请内存时逐页分配，确保在物理内存有严重碎片的情况下，vmalloc仍然可以工作







````C
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "vmalloc_demo"
#define DEVICE_FILE "/dev/vmalloc_demo"

static char *vmalloc_buffer = NULL;
static unsigned int mlen = 1024;  // 分配 1024 字节内存

#ifdef __aarch64__
#define PTR_IS_ERROR(ptr) IS_ERR_OR_NULL(ptr)
#else
#define PTR_IS_ERROR(ptr) ((unsigned long)(ptr) < PAGE_OFFSET || IS_ERR_OR_NULL(ptr))
#endif

// 驱动初始化函数
static int __init vmalloc_demo_init(void)
{
    printk(KERN_INFO "%s: 初始化驱动\n", DRIVER_NAME);
    
    printk(KERN_INFO "%s: VMALLOC_START: %llx, VMALLOC_END: %llx", DRIVER_NAME, VMALLOC_START, VMALLOC_END);

    // 使用 vmalloc 分配内存
    vmalloc_buffer = vmalloc(mlen);
    if (!vmalloc_buffer) 
    {
        printk(KERN_ERR "%s: 内存分配失败\n", DRIVER_NAME);
        return -ENOMEM;
    }
    printk(KERN_INFO "%s: 成功分配内存\n", DRIVER_NAME);
    
    printk(KERN_INFO "%s: vmalloc_buffer:%llx, PAGE_OFFSET:%llx\n", DRIVER_NAME, vmalloc_buffer, PAGE_OFFSET);
	
	if ((unsigned long)vmalloc_buffer < PAGE_OFFSET)
	{
		printk(KERN_INFO "%s: vmalloc_buffer < PAGE_OFFSET\n", DRIVER_NAME);
	}
    
    return 0;
}

// 驱动退出函数
static void __exit vmalloc_demo_exit(void)
{
    if (vmalloc_buffer) {
        vfree(vmalloc_buffer);
        printk(KERN_INFO "%s: 释放内存\n", DRIVER_NAME);
    }
    printk(KERN_INFO "%s: 驱动卸载\n", DRIVER_NAME);
}

module_init(vmalloc_demo_init);
module_exit(vmalloc_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("一个简单的 vmalloc 驱动示例");

````





在arm中需要使用新增的 PTR_IS_ERROR 进行判断，在原有的判断中 分配的内存 vmalloc_buffer 的地址是在  VMALLOC_START, VMALLOC_END 内但是小于 PAGE_OFFSET 的。







1. kmalloc分配内存的大小
本文基于linux-5.15分析。

kmalloc会根据申请的内存大小来决定来决定使用块分配器(slab/slub/slob)或页分配器进行内存分配。控制kmalloc分配行为的主要有如下三个宏。

macro	desc
KMALLOC_MAX_SIZE	kmalloc可以分配的最大内存，超过此大小时返回NULL
KMALLOC_MAX_CACHE_SIZE	kmalloc使用slab分配器分配的最大内存，超过此大小后会通过伙伴系统分配页
KMALLOC_MIN_SIZE	kmalloc可以分配的最小内存，小于此大小时，kmalloc内部会按此大小分配

1.1. KMALLOC_MAX_SIZE：kmalloc可以分配的最大内存
KMALLOC_MAX_SIZE与块分配器类型(slab/slub/slob)和页面大小以及MAX_ORDER有关，相关定义在include/linux/slab.h中。一般最大为2 ^ (MAX_ORDER + PAGE_SHIFT - 1)，也就是2 ^ (MAX_ORDER - 1)个页面，就是伙伴系统所管理的最大内存块。通常MAX_ORDER为11，页面大小为4K，相应的，kmalloc最大可以分配1024个页面，也就是4M。

分配器类型	KMALLOC_MAX_SIZE
slab	2 ^ (MAX_ORDER + PAGE_SHIFT - 1)，但不得超过32M(2^25)
slub	2 ^ (MAX_ORDER + PAGE_SHIFT - 1)
slob	2 ^ (MAX_ORDER + PAGE_SHIFT - 1)



![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/1c378b598b2efac9ee0d64b745c960d3.png#pic_center)

另外，早期的slab是可以支持分配64M的，在5.13版本时，改为了32M，具体可以参考commit 588c7fa022d7b2361500ead5660d9a1a2ecd9b7d








参考链接:

https://blog.csdn.net/u014001096/article/details/125902768

https://www.cnblogs.com/linhaostudy/p/7477370.html