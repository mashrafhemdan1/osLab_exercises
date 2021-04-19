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
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define BUFSIZE 4096

MODULE_LICENSE("GPL");

int MAJOR_Number = 150;

/* RC4 */
void rc4(unsigned char* p, unsigned char* k, unsigned char* c, int l){
	unsigned char s[256];
	unsigned char t[256];
	unsigned char temp;
	unsigned char kk;
	int i, j, x;
	for(i = 0; i < 256; i++)
	{
		s[i] = i;
		t[i] = k[i % 4];
	}
	j = 0;
	for(i = 0; i < 256; i++)
	{
		j = (j+s[i]+t[i])%256;
		temp = s[i];
		s[i] = s[j];
		s[j] = temp;
	}
	i = j = -1;
	for(x = 0; x < l; x++)
	{
		i = (i+1) % 256;
		j = (j+s[i])%256;
		temp = s[i];
		s[i] = s[j];
		s[j] = temp;
		kk = (s[i] +s[j]) % 256;
		c[x] = p[x] ^ s[kk];
	}
}

/*Character Devices*/
bool is_cdev_registered = false;
struct cdev cipher_dv;
struct cdev cipher_key_dv;

bool is_procfile_created = false;
static ssize_t counter_procread(struct file* file, char __user *ubuf, size_t count, loff_t* ppos);
char buffer[BUFSIZE];
char message[BUFSIZE];
char key[128];
int counter = 0;
/*procfs related files*/
/*a structure used to hold information about the proc file*/
static struct proc_dir_entry* cipher_procfile; 
static struct proc_dir_entry* cipherkey_procfile;

/*Read and Write for the Cipher Character Device*/
static ssize_t cipherdv_read(struct file* file, char __user* ubuf, size_t count, loff_t* ppos){
	ssize_t ret;
	ssize_t length = 0;
	printk(KERN_INFO "cipherdv_read (/dev/%s) called\n", "cipher");

	if(*ppos > 0 || count < BUFSIZE){
		ret = 0;
	} else {
		length += sprintf(buffer, "%s", message);
		if(copy_to_user(ubuf, message, length)){
			return -EFAULT;
		}
		*ppos = length;
		ret = length;
	}

	return ret;
}

static ssize_t cipherdv_write(struct file* file, const char __user* ubuf, size_t count, loff_t* ppos){
	int i, c;
	if(*ppos > 0 || count > BUFSIZE){
		return -EFAULT;
	}
	if(copy_from_user(buffer, ubuf, count)) {
		return -EFAULT;
	}
	c = strlen(buffer);
	*ppos = c;
	rc4(buffer, key, message, c);
	return c;
}

/*Read and Write for the Cipher Key Character Device*/
static ssize_t cipherkeydv_read(struct file* file, char __user* ubuf, size_t count, loff_t* ppos){
	printk(KERN_ALERT "Go away silly one, you cannot see my key >-:\n");
	return 0;
}

static ssize_t cipherkeydv_write(struct file* file, const char __user* ubuf, size_t count, loff_t* ppos){
	int i, c;
	if(*ppos > 0 || count > 128){
		return -EFAULT;
	}
	if(copy_from_user(key, ubuf, count)) {
		return -EFAULT;
	}
	c = strlen(key);
	*ppos = c;
	return c;
}
static int cipher_open(struct inode* inode, struct file* file){
	printk(KERN_ALERT "Character Device is opened\n");
	return 0;
}

static const struct file_operations cipherdv_fops = {
	.owner = THIS_MODULE,
	.open = cipher_open,
	.read = cipherdv_read,
	.write = cipherdv_write,
};
static const struct file_operations cipherkeydv_fops = {
	.owner = THIS_MODULE,
	.open = cipher_open,
	.read = cipherkeydv_read,
	.write = cipherkeydv_write,
};

/*Read and Write for the Cipher Character Device*/
static ssize_t cipherdv_procread(struct file* file, char __user* ubuf, size_t count, loff_t* ppos){
	ssize_t ret;
	ssize_t length = 0;
	printk(KERN_INFO "cipherdv_read (/dev/%s) called\n", "cipher");

	if(*ppos > 0 || count < BUFSIZE){
		ret = 0;
	} else {
		length = strlen(message);
		rc4(message, key, buffer, length);
			/*+= sprintf(buffer, "%s\n", message);*/
		if(copy_to_user(ubuf, buffer, length)){
			return -EFAULT;
		}
		*ppos = length;
		ret = length;
	}
	
	return ret;
}

static ssize_t cipherdv_procwrite(struct file* file, const char __user* ubuf, size_t count, loff_t* ppos){
	printk(KERN_ALERT "You are not allowed to write in the cipher device proc file. File is for reading only\n");
	return 0;
}

/*Read and Write for the Cipher Key Character Device*/
static ssize_t cipherkeydv_procread(struct file* file, char __user* ubuf, size_t count, loff_t* ppos){
	printk(KERN_ALERT "Go away silly one, you cannot see my key >-:\n");
	return 0;
}

static ssize_t cipherkeydv_procwrite(struct file* file, const char __user* ubuf, size_t count, loff_t* ppos){
	int i, c;
	if(*ppos > 0 || count > 128){
		return -EFAULT;
	}
	if(copy_from_user(key, ubuf, count)) {
		return -EFAULT;
	}
	c = strlen(buffer);
	*ppos = c;
	return c;
}

static const struct file_operations cipherdv_procfops = {
	.owner = THIS_MODULE,
	.open = cipher_open,
	.read = cipherdv_procread,
};
static const struct file_operations cipherkeydv_procfops = {
	.owner = THIS_MODULE,
	.open = cipher_open,
	.read = cipherkeydv_procread,
	.write = cipherkeydv_procwrite,
};

static int myModule_init(void) {
	
	/*Registering The Character Device*/
	int error;
	dev_t dev;
	error = alloc_chrdev_region(&dev, 0, 2, "cipher");
	MAJOR_Number = MAJOR(dev);
	if(error < 0) {
		printk(KERN_ALERT "Error in registering character device\n");
		return error;
	}
	printk(KERN_ALERT "Major Number: %d\n", MAJOR_Number);
	cdev_init(&cipher_dv, &cipherdv_fops);
	cdev_add(&cipher_dv, MKDEV(MAJOR_Number, 0), 1);
	cdev_init(&cipher_key_dv, &cipherkeydv_fops);
	cdev_add(&cipher_key_dv, MKDEV(MAJOR_Number, 1), 1);
	is_cdev_registered = true;
	printk(KERN_ALERT "Character Device Registered\n");

	/*Registering the proc file system*/
	cipher_procfile = proc_create("cipher", 0660, NULL, &cipherdv_procfops);
	if(cipher_procfile == NULL) {
		proc_remove(cipher_procfile);
		printk(KERN_ALERT "Error: Couldn't initialize /proc/%s\n", "cipher");
		is_procfile_created = false;
		return -ENOMEM;
	}	
	cipherkey_procfile = proc_create("cipher_key", 0660, NULL, &cipherkeydv_procfops);
	if(cipherkey_procfile == NULL) {
		proc_remove(cipherkey_procfile);
		printk(KERN_ALERT "Error: Couldn't initialize /proc/%s\n", "cipher_key");
		is_procfile_created = false;
		return -ENOMEM;
	}
	is_procfile_created = true;
	printk(KERN_INFO "/proc/%s created\n", "cipher");
	printk(KERN_INFO "/proc/%s created\n", "cipher_key");

	return 0;
}
static void myModule_cleanup(void) {
	/*Removing Proc files*/
	if(is_procfile_created){
		proc_remove(cipher_procfile);
		proc_remove(cipherkey_procfile);
		printk(KERN_INFO "/proc/%s removed\n", "cipher");
		printk(KERN_INFO "/proc/%s removed\n", "cipher_key");
	}
	/*Unregister the character devices*/
	if(is_cdev_registered){
		cdev_del(&cipher_dv);
		cdev_del(&cipher_key_dv);
		unregister_chrdev_region(MKDEV(MAJOR_Number, 0), 2);
		printk(KERN_ALERT "Character Device unregistered\n");
	}
	printk(KERN_ALERT "Bye Bye CSCE-3402 :)\n");
}

module_init(myModule_init);
module_exit(myModule_cleanup);
