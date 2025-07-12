KVERS = $(shell uname -r)

# Kernel modules
obj-m += globalmem.o
obj-m += multi_globalmem.o

# Specify flags for the module compilation.
#EXTRA_CFLAGS=-g -O0

build: kernel_modules json

kernel_modules:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules

json:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) compile_commands.json

clean:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean
