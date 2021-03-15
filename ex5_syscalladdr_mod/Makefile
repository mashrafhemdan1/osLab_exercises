obj-m += kmodule.o

KDIR = /usr/src/linux-headers-4.19.0-13-amd64

all:	
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 

clean:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
