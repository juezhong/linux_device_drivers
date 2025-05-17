#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <stdlib.h>

struct operation {
    int operation_enum;
    void (*func)(int argc, char *argv[], int dev_fd);
};

struct operation_id {
    char operation_str[32];
    int operation_enum;
};

void command_blink(int argc, char *argv[], int dev_fd);
void command_delay(int argc, char *argv[], int dev_fd);
void command_block_read(int argc, char *argv[], int dev_fd);
void command_ioctl(int argc, char *argv[], int dev_fd);

// 只需要维护 枚举变量、operation_ids 和 operations 两个数组即可
// 修改操作命令时，只需要修改这两个数组即可
enum {
    APP_OPERATION_NONE = 0,
    APP_OPERATION_BLINK,
    APP_OPERATION_DELAY,
    APP_OPERATION_BLOCK_READ,   // 阻塞读
    APP_OPERATION_IOCTL,        // ioctl cmds
};

// 用来匹配操作命令索引（枚举）的数组
static const struct operation_id operation_ids[] = {
    {"none", APP_OPERATION_NONE},
    {"blink", APP_OPERATION_BLINK},
    {"delay", APP_OPERATION_DELAY},
    {"block_read", APP_OPERATION_BLOCK_READ},
    {"ioctl", APP_OPERATION_IOCTL},
};

// 用来执行操作命令的数组，通过枚举变量来快速找到对应的操作函数
static const struct operation operations[] = {
    {APP_OPERATION_NONE, NULL},
    {APP_OPERATION_BLINK, command_blink},
    {APP_OPERATION_DELAY, command_delay},
    {APP_OPERATION_BLOCK_READ, command_block_read},
    {APP_OPERATION_IOCTL, command_ioctl},
};

#endif /* __COMMANDS_H__ */
