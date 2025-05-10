#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "devices_ioctl.h"

static void led_ioctl_help(void) {
    printf("led ioctl cmds:\n");
    printf("1: led timer open\n");
    printf("2: led timer close\n");
    printf("3: led timer set\n");
}

static void gpio_ioctl_help(void) {
    printf("gpio ioctl cmds:\n");
}

void led_ioctl(int argc, char *argv[], int dev_fd) {
    int ret = 0;
    unsigned int cmd_num;
    unsigned int tmp_time_period;

    // read user input
    led_ioctl_help();
    while (1) {
        printf("Enter the ioctl cmds (0-7) or 'q' to quit: ");
        ret = scanf("%d", &cmd_num);
        if (ret != 1) {
            printf("Invalid input!\n");
            fflush(stdin);
            // continue;
            // may be user want to quit
            break;
        }
        switch (cmd_num) {
            case 1:
                // led timer open
                ioctl(dev_fd, IOCTL_CMD_LED_TIMER_OPEN, 0);
                break;
            case 2:
                // led timer close
                ioctl(dev_fd, IOCTL_CMD_LED_TIMER_CLOSE, 0);
                break;
            case 3:
                // led timer set, units is ms(millisecond)
                printf("Enter the timer value (ms): ");
                ret = scanf("%d", &tmp_time_period);
                if (ret != 1) {
                    printf("Invalid input!\n");
                    fflush(stdin);
                    continue;
                }
                ioctl(dev_fd, IOCTL_CMD_LED_TIMER_SET, tmp_time_period);
                break;
            default:
                printf("Invalid input!\n");
                break;
        }
    }
}
void gpio_ioctl(int argc, char *argv[], int dev_fd) {
    gpio_ioctl_help();
}
