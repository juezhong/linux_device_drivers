#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "devices_ioctl.h"

enum {
    DEVICE_TYPE_LED = 0, // LED 设备
    DEVICE_TYPE_GPIO,    // GPIO 设备
};

char *device_strs[] = {
    "led",  // LED 设备
    "gpio", // GPIO 设备
};

void command_blink(int argc, char *argv[], int dev_fd) {
    char *cmd = "switch";
    for (int i = 0; i < atoi(argv[3]); i++) {
        write(dev_fd, cmd, strlen(cmd));
        // delay 300ms
        usleep(300000);
    }
}
void command_delay(int argc, char *argv[], int dev_fd) {
    ;
}
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
            value = INVALID_KEY_VALUE;
        }
    }
}

static int find_device_index(char* device_type) {
    // find device index
    int index = 0;
    for (int i = 0; i < sizeof(device_strs) / sizeof(device_strs[0]); i++) {
        if (strcmp(device_strs[i], device_type) == 0) {
            index = i;
            break;
        }
    }
    return index;
}

void command_ioctl(int argc, char *argv[], int dev_fd) {
    // enter ioctl commands mode here
    printf("ioctl commands mode\n");
    // ./app /dev/xxx ioctl device_type
    // e.g. ./app /dev/xxx ioctl led
    // argv[3] decid the which devices command
    int device_index = find_device_index(argv[3]);
    switch (device_index) {
        case DEVICE_TYPE_LED:
            // printf("LED ioctl commands\n");
            led_ioctl(argc, argv, dev_fd);
            break;
        case DEVICE_TYPE_GPIO:
            // printf("GPIO ioctl commands\n");
            gpio_ioctl(argc, argv, dev_fd);
            break;
        default:
            printf("Invalid device type\n");
            break;
    }
}
