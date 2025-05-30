#ifndef __DEVICES_IOCTL_H__
#define __DEVICES_IOCTL_H__

#include "ioctl_cmds/cmd_led.h"
#include "ioctl_cmds/cmd_iic_oled.h"

void led_ioctl(int argc, char *argv[], int dev_fd);
void gpio_ioctl(int argc, char *argv[], int dev_fd);
void oled_ioctl(int argc, char *argv[], int dev_fd);

#endif /* __DEVICES_IOCTL_H__ */
