#ifndef __COMMANDS_H__
#define __COMMANDS_H__

enum {
    APP_OPERATION_NONE = 0,
    APP_OPERATION_BLINK,
    APP_OPERATION_DELAY,
    APP_OPERATION_BLOCK_READ, // 阻塞读
};

char *app_operation_strs[] = {
    "none",
    "blink", // 闪烁
    "delay", // 延时
    "block_read", // 阻塞读
};

void command_blink(int argc, char *argv[], int dev_fd);
void command_delay(int argc, char *argv[], int dev_fd);
void command_block_read(int argc, char *argv[], int dev_fd);

#endif /* __COMMANDS_H__ */
