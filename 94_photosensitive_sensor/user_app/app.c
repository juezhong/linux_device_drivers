#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/photosensitive_sensor"

int main(void)
{
	int fd;
	int value;
	int ret;
	int time = 10;

	fd = open(DEVICE_PATH, O_RDONLY);
	if (fd < 0) {
		perror("打开设备失败");
		return 1;
	}

	while (time--) {
		ret = read(fd, &value, sizeof(value));
		if (ret < 0) {
			perror("读取设备失败");
			break;
		}
		printf("Read value is: %d\n", value);
		sleep(1);
	}

	close(fd);
	return 0;
}
