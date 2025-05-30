#ifndef __CMD_LED_H__
#define __CMD_LED_H__

#define IOCTL_CMD_LED_TIMER_OPEN (_IO(0xAA, 0x01))   // 打开定时器
#define IOCTL_CMD_LED_TIMER_CLOSE (_IO(0xAA, 0x02))  // 关闭定时器
#define IOCTL_CMD_LED_TIMER_SET (_IO(0xAA, 0x03))    // 设置定时器

#endif /* __CMD_LED_H__ */