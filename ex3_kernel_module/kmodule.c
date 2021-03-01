#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

int n = 0;
char* word;
module_param(n, int, S_IRUSR | S_IWUSR);
module_param(word, charp, 0000);

static int myModule_init(void) {
	/*printk(KERN_ALERT "Hello World CSCE-3402 :) + %d%s\n", n, word);*/
	int i;
	for(i = 0; i < n; i++){
		printk(KERN_ALERT "%s\n", word);
	}
	return 0;
}
static void myModule_cleanup(void) {
	printk(KERN_ALERT "Bye Bye CSCE-3402 :)\n");
}

module_init(myModule_init);
module_exit(myModule_cleanup);
