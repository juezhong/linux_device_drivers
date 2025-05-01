#!/bin/bash

# 编译工具链和架构设置
export CROSS_COMPILE=arm-linux-gnueabihf-
export ARCH=arm

# 检查传入的参数
if [ -z "$1" ]; then
  echo "Error: Module name (MODULE_NAME) is required!"
  echo "Usage example: ./compile_module.sh <MODULE_NAME> <KERNEL_DIR>"
  exit 1
fi

if [ -z "$2" ]; then
  echo "Error: Kernel source directory (KERNEL_DIR) is required!"
  echo "Usage example: ./compile_module.sh <MODULE_NAME> /path/to/kernel/source"
  exit 1
fi

# 必须提供的参数
export MODULE_NAME=$1
export KERNEL_DIR=$2

# 编译命令
# 运行 make 准备编译
make
if [ $? -ne 0 ]; then
    echo "Error: Make failed!" >> $LOG_FILE
    exit 1
fi

# 生成 JSON 输出（如果有这个步骤）
make json
if [ $? -ne 0 ]; then
    echo "Error: Make json failed!" >> $LOG_FILE
    exit 1
fi

# 将编译生成的 .ko 文件复制到目标目录
cp -v ./*.ko ../../drivers/
if [ $? -ne 0 ]; then
    echo "Error: Failed to copy .ko files!" >> $LOG_FILE
    exit 1
fi
echo "Module ./*.ko successfully compiled and copied to ../../drivers/"

# # 如果命令是 'clean'，则清理构建文件
# if [ "$MODULE_NAME" == "clean" ]; then
#   make clean
#   echo "Clean completed."
# fi

