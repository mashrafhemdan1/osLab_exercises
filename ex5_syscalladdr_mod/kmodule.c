#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/syscalls.h>

MODULE_LICENSE("GPL");

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

static int myModule_init(void) {
	char* filename = "/proc/version";
	/*char* filename = "/boot/System.map-4.19.0-13-amd64";*/
	struct myfile* mf;
	mf = open_file_for_read(filename);
	if(mf == NULL){
		return 0;
	}
	char* version;
	unsigned long n = 1024;
	version = kmalloc(n * sizeof(char), GFP_KERNEL);
	int i;
	/*take the third string in the file which is the proc version*/
	for(i = 0; i < 3; i++)
		read_from_file_until(mf, version, n, ' ');
	printk(KERN_ALERT "%s\n", version);
	
	filename = "/boot/System.map-";
	char* newfilename;
	newfilename = string_concatenate(filename, version);
	printk(KERN_ALERT "%s\n", newfilename);
	kfree(version);
	close_file(mf);
	kfree(mf);
	
	struct myfile* nmf;
	/*newfilename = "/boot/System.map-4.19.0-13-amd64";*/
	nmf = open_file_for_read(newfilename);
	if(nmf == NULL){
		printk(KERN_ALERT "Cannot open the file\n");
		return 0;
	}
	char* address;
	char* name;
	address = kmalloc(n * sizeof(char), GFP_KERNEL);
	name = kmalloc(n * sizeof(char), GFP_KERNEL);
	char* systable = "sys_call_table";
	int count;
	while(true){ 
		count = read_from_file_until(nmf, address, n, ' '); if(count == 0) break;
		count = read_from_file_until(nmf, name, n, ' '); if(count == 0) break;
		count = read_from_file_until(nmf, name, n, '\n'); if(count == 0) break;
		if(strcmp(name, systable) == 0){
			break;
		}
	}
	
	printk(KERN_ALERT "Address:%s\n", address);
	printk(KERN_ALERT "Name:%s\n", name);
	
	kfree(name);
	kfree(newfilename);
	close_file(nmf);
	kfree(nmf);
	
	unsigned long syscalladdr;
       	sscanf(address, "%lx", &syscalladdr);
	sys_call_ptr_t* sys_call_table = (sys_call_ptr_t *)syscalladdr;
	printk(KERN_ALERT "Fork Address: %px\n", sys_call_table[__NR_fork]);
	kfree(address);
	return 0;
}
static void myModule_cleanup(void) {
	printk(KERN_ALERT "Bye Bye CSCE-3402 :)\n");
}

module_init(myModule_init);
module_exit(myModule_cleanup);
