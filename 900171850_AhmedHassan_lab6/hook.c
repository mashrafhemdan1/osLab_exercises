#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h> 
#include <linux/fs.h> 
#include <asm/uaccess.h> 
#include <linux/syscalls.h>
#include <asm/cacheflush.h>
#include <linux/semaphore.h>


struct myfile{
	struct file *f;
	mm_segment_t fs;
	loff_t pos;

};
struct myfile * open_file_for_read(char*filename){
	struct myfile *mf;
	mf = kmalloc(sizeof(struct myfile), GFP_KERNEL);
	mf->f = filp_open(filename, O_RDONLY, 0);
	
	if(mf->f != NULL)
	{
		if(IS_ERR(mf->f))
		{
			printk("<1>error %p for %s**\n", mf->f, filename);
		}
	}
	mf->pos = mf->f->f_pos;
	mf->fs = get_fs();
	
	
	return mf;
}

volatile int read_from_file_until(struct myfile * mf, char * buf, unsigned long vlen, char c){
	volatile int ret = 0;
	int i = 0;
	set_fs(KERNEL_DS);
	ret = kernel_read(mf->f,buf+i , 1, &mf->pos);
	while(buf[i] != c)
	{
		i = i + 1;
		ret = kernel_read(mf->f,buf+i , 1, &mf->pos);
		
	}
	buf[i]='\0';
	
	set_fs(mf->fs);
	return ret;
}

void close_file(struct myfile *mf){
	filp_close(mf->f, NULL);
	mf->f = NULL;
	kfree(mf);
}

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
	char * name = "/boot/System.map-4.19.0-13-amd64";
	char * buf = '\0';
	char * sys_table_add;
	unsigned long vlen = 100;
	char c = '\n';
	bool isthere = false;
	struct myfile *mf;
	unsigned long sys_table_add_int = 0;
	
	


	buf = (char*)kcalloc(vlen, sizeof(char),GFP_KERNEL);
	sys_table_add = (char*) kcalloc(vlen, sizeof(char), GFP_KERNEL);
	mf = open_file_for_read(name);
	while(isthere == false)
	{
		read_from_file_until(mf, buf, vlen, c);
		if(strstr(buf, "sys_call_table") != NULL)
		{
			isthere= true;
			strncat(sys_table_add, buf, 16);
			printk(KERN_INFO "address of system calls %s \n", sys_table_add);
			sscanf(sys_table_add, "%lx", &sys_table_add_int);
			sys_table_add_ptr = (void **)sys_table_add_int;
			
		}
	}
	
	original_call = sys_table_add_ptr[__NR_clone];
	
	
	write_cr0(read_cr0() & (~0x10000));	
	sys_table_add_ptr[__NR_clone]= our_system_fork;
	write_cr0(read_cr0() | 0x10000);

	close_file(mf);
	kfree(buf);
	kfree(sys_table_add);
	
	
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
