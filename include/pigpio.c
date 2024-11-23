/* hardeware/gpio.h */

#include "pigpio.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/select.h>
#include <termios.h>

int gpioInitialise(void)
{
	printf("%s\n", __func__);
	return 0;
	return -1;
}

int gpioSetMode(unsigned pin, unsigned mode)
{
	printf("%s(%u, %u)\n", __func__, pin, mode);
	return 0;
	return -1;
}

void gpioTerminate(void)
{
	printf("%s\n", __func__);
}

void gpioWrite(unsigned pin, unsigned level)
{
	printf("%s(%u, %u)\n", __func__, pin, level);
}

uint32_t gpioDelay(uint32_t micros)
{
	usleep(micros);
	return micros;
}

int serOpen(const char *device, int baud, int unused)
{
	struct termios tc;
	int fd = open(device, O_RDWR);
	if (fd >= 0) {
		fd=open(device, O_RDWR);
		if(tcgetattr(fd, &tc) < 0) {
			perror("tcgetattr");
			return 1;
		}
		tc.c_cflag &= ~(CSIZE | PARENB | CSIZE);
		tc.c_cflag |= CS8 | CSTOPB;
		tc.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | INPCK | ISIG);
		tc.c_iflag |= IGNBRK | IGNPAR | ICRNL | INLCR | IXANY;
		tc.c_oflag &= ~OPOST;
		tc.c_cc[VMIN]	= 1;
		tc.c_cc[VTIME]	= 0;
		cfsetspeed(&tc, baud);
		if(tcsetattr(fd, TCSANOW, &tc) < 0) { /* failed to modify */
#ifdef __APPLE__
#define IOSSIOSPEED _IOW('T', 2, speed_t)
			cfsetspeed(&tc, B9600);
			if(tcsetattr(fd, TCSANOW, &tc) < 0 || ioctl(fd, IOSSIOSPEED, &baud))
#endif
				{
				perror("tcsetattr");
				return 1;
				}
		}
	}
	return fd;
}

int serReadByte(int fd)
{
	char buf[1];
	int n = read(fd, buf, sizeof(buf));
	if (n != 1)
		return PI_SER_READ_NO_DATA;
	return buf[0];
}
