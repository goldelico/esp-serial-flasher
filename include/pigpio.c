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

int gpioInitialise(void)
{
	return -1;
}

int gpioSetMode(int pin, int mode)
{
	return -1;
}

int gpioTerminate(void)
{
	return -1;
}

int gpioWrite(int pin, int value)
{
	return -1;
}

int gpioDelay(int us)
{
	usleep(us);
}
