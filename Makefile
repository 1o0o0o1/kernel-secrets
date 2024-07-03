obj-m += secret.o
PWD := $(CURDIR)

all:
        make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
        echo "" > secret.c
        make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean