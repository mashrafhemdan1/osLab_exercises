#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <asm/page_types.h>
#include <linux/kallsyms.h>

MODULE_LICENSE("GPL");

/*typedef asmlinkage long (*sys_call_ptr_t) (const struct pt_regs*);*/
typedef asmlinkage int (*clone_syscalltype) (int (*fn) (void *), void *stack, int flags, pid_t *parent_tid, void *tls, pid_t *child_tid);
clone_syscalltype old_fork; /*the old fork entry*/
static sys_call_ptr_t* syscall_table; 
int counter = 0;
bool sysmap_read = false;
struct myfile{
	struct file* f;
	mm_segment_t fs;
	loff_t pos;
};

struct myfile* open_file_for_read(char* filename){
	struct myfile*  mf;
	int err = 0;
	mf = kmalloc(sizeof(struct myfile), GFP_KERNEL);
	if(!mf){
		printk(KERN_ALERT "Error Occured during memory allocation of myfile\n");
		return NULL;
	}
	mf->fs = get_fs();
	set_fs(get_ds());
	mf->f = filp_open(filename, 0, 0);
	set_fs(mf->fs);
	if(IS_ERR(mf->f)){
		err = PTR_ERR(mf->f);
		return NULL;
	}
	return mf;
}

volatile int read_from_file_until(struct myfile* mf, char* buf, unsigned long vlen, char c){
	mf->fs = get_fs();
	set_fs(KERNEL_DS);
	int i = 0;
	while( i < vlen){
		char* ptr = buf + i;
		vfs_read(mf->f, ptr, 1, &mf->pos);
		/*check if the character is at this position. If so, break*/
		if(*ptr == c){
			buf[i] = '\0';
			break;
		}
		i++;
	}
	set_fs(mf->fs);
	return i;
}

void close_file(struct myfile* mf){
	struct file* f = mf->f;
	filp_close(f, NULL);
}

char* string_concatenate(char* first, char* second){
	int new_size;
        new_size = strlen(first) + strlen(second) + 1;
	char* result;
	result = kmalloc(new_size * sizeof(char), GFP_KERNEL);

	strcpy(result, first);
	strcat(result, second);
	return result;
}
static asmlinkage int my_fork(int (*fn) (void *), void *stack, int flags, pid_t *parent_tid, void *tls, pid_t *child_tid )
{
	counter = counter +1;
	if(counter%10 == 0)
		printk(KERN_ALERT "Counter Now: %d\n", counter);
	return old_fork(fn, stack, flags, parent_tid, tls, child_tid);
}


static int myModule_init(void) {
	
	/* retrieving fork system call address with kaslr disabled */
	unsigned long address;
	address = kallsyms_lookup_name("sys_call_table");
	if(address == NULL){
		sysmap_read = false;
		return 0;
	}
	else	sysmap_read = true;

	printk(KERN_ALERT "System Call Table Address:%px\n", address);
	
	/* Retrieve address of the system call clone with kaslr enabled*/
	
	unsigned long syscalladdr;
	syscalladdr = address;
       	sscanf(address, "%lx", &syscalladdr);
	syscall_table = (sys_call_ptr_t *)syscalladdr;
	old_fork = (clone_syscalltype)syscall_table[__NR_clone];
	const unsigned long fork_addr = syscall_table[__NR_clone];
	printk(KERN_ALERT "Fork Address: %px\n", fork_addr);
	

	/*changing pte's write permission*/
	unsigned int lvl;
	pte_t* pte = lookup_address(syscalladdr, &lvl);
	if(pte->pte &~ _PAGE_RW)
		pte->pte |= _PAGE_RW;
	
	/*adding my function address instead of the original fork*/
	syscall_table[__NR_clone] = (sys_call_ptr_t)my_fork;
	
	/*returning to the read only mode*/
	pte->pte &= ~_PAGE_RW;
	/*kfree(address);*/
	return 0;
}
static void myModule_cleanup(void) {
	/*changing pte's write permission*/
	if(sysmap_read){
		unsigned int lvl;
		pte_t* pte = lookup_address((unsigned long)syscall_table, &lvl);
		if(pte->pte &~ _PAGE_RW)
		pte->pte |= _PAGE_RW;

		syscall_table[__NR_clone] = (sys_call_ptr_t)old_fork;
		/*returning to the read-only mode again*/
		pte->pte &= ~_PAGE_RW;
	}
	printk(KERN_ALERT "Bye Bye CSCE-3402 :)\n");

}

module_init(myModule_init);
module_exit(myModule_cleanup);
