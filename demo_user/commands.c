#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
