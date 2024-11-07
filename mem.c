#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>

static int mem_size_m = 1; // Default size in megabytes (MB)
module_param(mem_size_m, int, 0444);
MODULE_PARM_DESC(mem_size_m, "Memory size to allocate in MB");

#define PAGE_ORDER 2  // 16K = 4 pages, as each page is 4K
#define PAGE_SIZE_KB (1 << (PAGE_ORDER + 2))  // 4 pages * 4K = 16K

static unsigned long *my_pages = NULL;
void * mem_pages = NULL;
static int num_allocations = 0;

static int __init my_module_init(void)
{
    int i;
    int total_pages = (mem_size_m * 1024) / PAGE_SIZE_KB;

    mem_pages = kmalloc(total_pages * sizeof(unsigned long), GFP_KERNEL);
	my_pages = (unsigned long *)mem_pages;
    if (!my_pages) 
	{
        printk(KERN_ERR "Failed to allocate memory for pointers.\n");
        return -ENOMEM;
    }

    for (i = 0; i < total_pages; i++) 
	{
        my_pages[i] = __get_free_pages(GFP_KERNEL, PAGE_ORDER);
        if (!my_pages[i]) 
		{
            printk(KERN_ERR "Failed to allocate memory chunk %d.\n", i);
            break;
        }
        printk(KERN_INFO "Allocated 16K chunk %d at 0x%lx.\n", i, my_pages[i]);
        num_allocations++;
    }

    printk(KERN_INFO "Requested %d MB, allocated %d chunks of 16K.\n", mem_size_m, num_allocations);
    return 0;
}

static void __exit my_module_exit(void)
{
    int i;
	
	my_pages = (unsigned long *)mem_pages;
    if (my_pages) 
	{
        for (i = 0; i < num_allocations; i++) 
		{
            free_pages(my_pages[i], PAGE_ORDER);
            //printk(KERN_INFO "Freed 16K chunk %d at 0x%lx.\n", i, my_pages[i]);
        }

        kfree(my_pages);
        printk(KERN_INFO "Freed all allocated memory.\n");
    }
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("allocate memory in 16K");
