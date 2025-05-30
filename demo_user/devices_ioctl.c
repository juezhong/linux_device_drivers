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

static void oled_ioctl_help(void) {
    printf("oled ioctl cmds:\n");
    printf("1: oled turn on & init\n");
    printf("2: oled turn off\n");
    printf("3: oled set cursor position\n");
    printf("4: oled clear\n");
    printf("5: oled write str\n");
}

/* ------------------------------------------------------- */

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

void oled_ioctl(int argc, char *argv[], int dev_fd) {
    int ret = 0;
    unsigned int cmd_num;
    struct str {
        char *_str;
        int _len;
    };
    struct str str;
    str._str = malloc(32);
    str._len = 32;
    memset(str._str, 0, str._len);
    // strcpy(str._str, "abcdefghijklmnopqrstuvwxyz");

    // read user input
    oled_ioctl_help();
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
                // oled turn on & init
                ioctl(dev_fd, IOCTL_CMD_OLED_TURN_ON_INIT, 0);
                break;
            case 2:
                // oled turn off
                ioctl(dev_fd, IOCTL_CMD_OLED_TURN_OFF, 0);
                break;
            case 3:
                // oled set cursor position
                printf("Enter the cursor position (0-6): ");
                ret = scanf("%d", &cmd_num);
                if (ret != 1) {
                    printf("Invalid input!\n");
                    fflush(stdin);
                    continue;
                }
                ioctl(dev_fd, IOCTL_CMD_OLED_SETCURSOR, cmd_num);
                break;
            case 4:
                // oled clear
                ioctl(dev_fd, IOCTL_CMD_OLED_CLEAR, 0);
                break;
            case 5:
                // oled write str
                printf("Enter the string: ");
                // clear buffer
                fflush(stdin);
                // use _ replace space
                ret = scanf("%s", str._str);
                if (ret != 1) {
                    printf("Invalid input!\n");
                    fflush(stdin);
                    continue;
                }
                // recalculate the length
                str._len = strlen(str._str);
                // use _ replace space
                for (int i = 0; i < str._len; i++) {
                    if (str._str[i] == '_') {
                        str._str[i] = ' ';
                    }
                }
                // replace character "\" to '\n'
                for (int i = 0; i < str._len; i++) {
                    if (str._str[i] == '\\') {
                        str._str[i] = '\n';
                    }
                }
                // ret = scanf("%[^\n]", str._str);
                // if (ret != 1) {
                //     printf("Invalid input!\n");
                //     fflush(stdin);
                //     continue;
                // }
                ioctl(dev_fd, IOCTL_CMD_OLED_WRITE_STR, &str);
                break;
            default:
                printf("Invalid input!\n");
                break;
        }
    }
    free(str._str);
}
