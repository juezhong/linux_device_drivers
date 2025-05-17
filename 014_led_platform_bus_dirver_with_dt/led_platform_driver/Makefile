# 如果未指定内核源码路径，默认使用当前运行内核的源码路径
# KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
KERNEL_DIR := /home/liyunfeng/workspace/kernel/src/opendev/linux-imx
# KERNEL_DIR := /home/liyunfeng/workspace/kernel/src/linux-6.12.3

# MODULE_NAME := $(shell fd -t f -e c -x echo {.})
# MODULE_NAME := $(shell find . -type f -name "*.c" -exec basename -s .c {} \;)
# MODULE_NAME := $(notdir $(CURDIR))
MODULE_NAME := led_platform_driver_dt

# all:
	# echo $(MODULE_NAME)

# 模块目标名
obj-m := $(MODULE_NAME).o

# 编译目标
all: comp json copy2drivers

comp:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

# 清理编译生成的文件
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm ../../../drivers/$(MODULE_NAME).ko

# # 使用 insmod 加载模块
# load: all
# 	insmod ./$(MODULE_NAME).ko
# 	dmesg | tail -n 20
#
# # 使用 rmmod 卸载模块
# unload:
# 	rmmod $(MODULE_NAME)
# 	dmesg | tail -n 20

json:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) compile_commands.json

copy2drivers:
	@cp -v ./*.ko ../../../drivers/
