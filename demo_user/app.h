#ifndef __APP_H__
#define __APP_H__

static void print_cli(int argc, char* argv[]);
static int open_device(char* device);
static int close_device(int fd);
static int write_device(int fd, char* buf, int len);
static int read_device(int fd, char* buf, int len);
static int execute_command_no_args(int argc, char* argv[]);
int find_command_index(char* command);
static int execute_command_with_one_args(int argc, char* argv[]);
static int execute_command_with_multi_args(int argc, char* argv[]);

#endif /* __APP_H__ */
