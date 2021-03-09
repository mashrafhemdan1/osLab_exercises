#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");

struct myfile{
	struct file* f;
	mm_segment_t fs;
	loff_t pos;
};

struct myfile* open_file_for_read(char* filename){
	struct myfile*  mf;
	mf = kmalloc(sizeof(struct myfile), GFP_KERNEL);
	if(!mf){
		printk(KERN_ALERT "Error Occured during memory allocation of myfile\n");
		return NULL;
	}
	mf->fs = get_fs();
	set_fs(KERNEL_DS);
	mf->f = filp_open(filename, 0, 0);
	set_fs(mf->fs);
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
			buf[i+1] = '\0';
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

static int myModule_init(void) {
	char* filename = "/proc/version";
	struct myfile* mf;
	mf = open_file_for_read(filename);
	if(mf == NULL){
		return 0;
	}
	char* buf;
	unsigned long n = 1024;
	buf = kmalloc(n * sizeof(char), GFP_KERNEL);
	int i;
	/*take the third string in the file which is the proc version*/
	for(i = 0; i < 3; i++)
		read_from_file_until(mf, buf, n, ' ');
	printk(KERN_ALERT "%s\n", buf);
	kfree(buf);
	close_file(mf);
	kfree(mf);
	return 0;
}
static void myModule_cleanup(void) {
	printk(KERN_ALERT "Bye Bye CSCE-3402 :)\n");
}

module_init(myModule_init);
module_exit(myModule_cleanup);
