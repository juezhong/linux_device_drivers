#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "app.h"
#include "commands.h"

static void print_cli(int argc, char* argv[])
{
    printf("===> [APP DEBUG] Device: %s <===\n", argv[1]);
    printf("===> [APP DEBUG] Command: %s <===\n", argv[2]);
    // 如果传入了 arg
    if (argv[3]) {
        printf("===> [APP DEBUG] Args %d: %s <===\n", 1, argv[3]);
    }
    // 如果传入了多个 arg
    for (int i = 4; i < argc; i++) {
        printf("===> [APP DEBUG] Args %d: %s <===\n", i - 3, argv[i]);
    }
}

static int open_device(char* device)
{
    int dev_fd = open(device, O_RDWR);
    printf("===> [APP DEBUG] Open device: \"%s\" fd: %d <===\n", device, dev_fd);
    return dev_fd;
}

static int close_device(int fd)
{
    printf("===> [APP DEBUG] Close device fd: %d <===\n", fd);
    close(fd);
    return 0;
}

static int write_device(int fd, char* buf, int len)
{
    int ret = write(fd, buf, len);
    printf("===> [APP DEBUG] Write \"%s\" to device: %d ret: %d <===\n", buf, fd, ret);
    return ret;
}

static int read_device(int fd, char* buf, int len)
{
    int ret = read(fd, buf, len);
    printf("===> [APP DEBUG] Read \"%s\" from device: %d ret: %d <===\n", buf, fd, ret);
    return ret;
}

static int execute_command_no_args(int argc, char* argv[])
{
    // printf("Usage: ./app <device> <command>\n");
    print_cli(argc, argv);
    int dev_fd = open_device(argv[1]);
    char *read_buf = (char *)malloc(32);
    char *write_buf = (char *)malloc(32);

    memset(read_buf, '\0', 32);
    memset(write_buf, '\0', 32);

    read_device(dev_fd, read_buf, 3); // 读取设备中的数据
    write_device(dev_fd, argv[2], strlen(argv[2])); // 写入命令到设备

    close_device(dev_fd);
    return 0;
}

// 找出命令的下标
int find_command_index(char* command)
{
    for (int i = 0; i < sizeof(operation_ids) / sizeof(operation_ids[0]); i++) {
        if (strcmp(operation_ids[i].operation_str, command) == 0) {
            return i;
        }
    }
    return 0;
}

static int execute_command_with_one_args(int argc, char* argv[])
{
    // printf("Usage: ./app <device> <command> <args>\n");
    print_cli(argc, argv);
    int dev_fd = open_device(argv[1]);
    char *read_buf = (char *)malloc(32);
    char *write_buf = (char *)malloc(32);

    memset(read_buf, '\0', 32);
    memset(write_buf, '\0', 32);

    read_device(dev_fd, read_buf, 3); // 读取设备中的数据

    // 根据不同的命令做不同的处理
    // 通过字符串数组，直接取出来下标
    int command_enum = find_command_index(argv[2]);
    operations[command_enum].func(argc, argv, dev_fd);

    close_device(dev_fd);
    return 0;
}

static int execute_command_with_multi_args(int argc, char* argv[])
{
    // printf("Usage: ./app <device> <command> <args> <args> <args> ...\n");
    print_cli(argc, argv);
    return 0;
}

int main(int argc, char *argv[])
{

    if (argc < 3) {
        printf("Usage1: ./app <device> <command>\n");
        printf("Usage2: ./app <device> <command> <args>\n");
        printf("Usage3: ./app <device> <command> <args> <args> <args> ...\n");
        return 1;
    }

    // Usage1: ./app <device> <command>
    if (argc == 3)
        return execute_command_no_args(argc, argv);

    // Usage2: ./app <device> <command> <args>
    if (argc == 4)
        return execute_command_with_one_args(argc, argv);

    // Usage3: ./app <device> <command> <args> <args> <args> ...
    if (argc > 4)
        return execute_command_with_multi_args(argc, argv);

    return 0;
}
