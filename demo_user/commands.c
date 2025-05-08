#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

void command_blink(int argc, char *argv[], int dev_fd)
{
    char *cmd = "switch";
    for (int i = 0; i < atoi(argv[3]); i++) {
        write(dev_fd, cmd, strlen(cmd));
        // delay 300ms
        usleep(300000);
    }
}
void command_delay(int argc, char *argv[], int dev_fd);
void command_block_read(int argc, char *argv[], int dev_fd) {
    // already opened
    #define VALID_KEY_VALUE     0xF0
    #define INVALID_KEY_VALUE   0x00
    unsigned char value = 0;
    while (1) {
        read(dev_fd, &value, 1);
        if (value == VALID_KEY_VALUE) {
            printf("Valid key value: %x\n", value);
            printf("Press key.. \n");
        }
    }
}
