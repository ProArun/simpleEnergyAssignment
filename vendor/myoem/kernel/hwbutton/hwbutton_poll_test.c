#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "hwbutton.h"

int main(void)
{
	int fd = open("/dev/hwbutton", O_RDWR);
	if (fd < 0) {
		perror("open /dev/hwbutton");
		return 1;
	}

	__u32 value = 0;
	if (ioctl(fd, HWBUTTON_IOC_GET_VALUE, &value) < 0) {
		perror("ioctl GET_VALUE");
		return 1;
	}
	printf("initial value: %u\n", value);

	struct pollfd pfd = { .fd = fd, .events = POLLIN };
	printf("blocking in poll(), trigger the button from another shell...\n");

	int ret = poll(&pfd, 1, -1);
	if (ret < 0) {
		perror("poll");
		return 1;
	}

	if (pfd.revents & POLLIN)
		printf("poll() woke up: button was clicked\n");
	else
		printf("poll() returned with unexpected revents=0x%x\n", pfd.revents);

	close(fd);
	return 0;
}
