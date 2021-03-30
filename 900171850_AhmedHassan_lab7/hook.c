#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h> 
#include <linux/fs.h> 
#include <asm/uaccess.h> 
#include <linux/syscalls.h>
#include <asm/cacheflush.h>
#include <linux/semaphore.h>
MODULE_LICENSE("GPL");


void **sys_table_add_ptr = NULL;
int counters = 0;
asmlinkage long (*original_call)(long a1, long a2, long a3, long a4, long a5, long a6) = NULL;
asmlinkage long our_system_fork(long a1, long a2, long a3, long a4, long a5, long a6)
	{
		if( counters%10 == 0)
			{
				printk(KERN_INFO "number of fork calls since system installed %d \n", counters);
			}
		counters = counters + 1;
		return original_call(a1, a2, a3, a4, a5, a6);
	}

static int hello_init(void){
	sys_table_add_ptr = (void **) kallsyms_lookup_name("sys_call_table");
	original_call = sys_table_add_ptr[__NR_clone];
	
	
	write_cr0(read_cr0() & (~0x10000));	
	sys_table_add_ptr[__NR_clone]= our_system_fork;
	write_cr0(read_cr0() | 0x10000);
	
	return 0;

}

static void hello_exit(void){
	write_cr0(read_cr0() & (~0x10000));
	sys_table_add_ptr[__NR_clone]= original_call;
	write_cr0(read_cr0() | 0x10000);
	printk(KERN_ALERT "CSCE: Goodbye\n");
}

module_init(hello_init);
module_exit(hello_exit);
