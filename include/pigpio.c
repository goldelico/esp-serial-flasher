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
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>

#define TARGET_RST_Pin 2
#define TARGET_IO0_Pin 3

int fd;
void *map_base;

int gpioInitialise(void)
{
#if 0
	printf("%s\n", __func__);
#endif
	return 0;
}

void gpioTerminate(void)
{
#if 0
	printf("%s\n", __func__);
#endif
}

int gpioRead(unsigned pin)
{
#ifdef __APPLE__
	return -EINVAL;
#else
	char *file;
	FILE *f;
	int level = -EINVAL;
	switch(pin)
		{
			case TARGET_RST_Pin: file="/sys/devices/platform/wlan_pwrseq/enable_power"; break;
			case TARGET_IO0_Pin: file="/sys/devices/platform/wlan_pwrseq/enable_flashing"; break;
			default:
				fprintf(stderr, "unknown pin %u\n", pin);
				return -EINVAL;
		}
	f=fopen(file, "r");
	if(!f)
		{
		perror(file);
		return -EINVAL;
		}
	fscanf(f, "%d", &level);	// directly control the wlan_pwrseq-gpio
	fclose(f);
	if (pin == TARGET_IO0_Pin)
		level=!level;	// inverted logic - returning 1 means "disable flashing"
	return level;
#endif
}

int gpioSetMode(unsigned pin, unsigned mode)
{ // set gpio mode
#if 1
	printf("%s(%u, %u)\n", __func__, pin, mode);
#endif
	return 0;	// ignore (should have been et by pwrseq_esp32 driver in kernel)
}

void gpioWrite(unsigned pin, unsigned value)
{
#if 1
	printf("%s(%u, %u)\n", __func__, pin, value);
#endif
#ifdef __APPLE__
	return;	// can't do Linux tricks - power must be controlled externally
#endif
	/*
	 * Controlling power of the ESP32 on a running Linux system is quite tricky
	 * if we directly control the EN GPIO, the mmc core will detect that the
	 * module is not responding and actively enables it through a vmmc-supply
	 * or mmc-pwrseq-simple which usually controls the EN GPIO.
	 * So we can turn off the ESP32 only for a moment by directly manipulating
	 * the GPIO registers.
	 * Even worse, if the GPIO is not accessible directly (i2c-gpio expander)
	 * doing that from user-space is almost impossible.
	 * Therefore, we rely on a special pwrseq_esp32 driver in the kernel which
	 * adds two new properties:
	 *   enable_power
	 *   enable_flashing
	 * To switch the ESP32 into flashing mode do:
	 *   echo "13460000.mmc" >/sys/bus/platform/drivers/jz4740-mmc/unbind
	 *   echo "0" >/sys/devices/platform/wlan_pwrseq/enable_power
	 *   echo "1" >/sys/devices/platform/wlan_pwrseq/enable_flashing
	 *   echo "1" >/sys/devices/platform/wlan_pwrseq/enable_power
	 *   echo "0" >/sys/devices/platform/wlan_pwrseq/enable_flashing
	 *   sleep 2

	 */

	// 	printf("before: "); gpioRead(pin);	// current pin level

	char *file;
	FILE *f;
	switch(pin) {
		case TARGET_RST_Pin: {
			if(!value || gpioRead(TARGET_IO0_Pin))
				{ // control gpio through mmc-pwrseq-simple or some regulator - unless we are controlling flashing mode
				  //		printf("%s\n", value? "bind" : "unbind");
					if(value)
						file="/sys/bus/platform/drivers/jz4740-mmc/bind";
					else
						file="/sys/bus/platform/drivers/jz4740-mmc/unbind";
					f=fopen(file, "w");
					if(!f) {
						perror(file);
						exit(1);
					}
					fprintf(f, "%s\n", "13460000.mmc");	// mmc1 controls the GPIO
					fclose(f);
					sleep(1);
					//			printf("after: "); gpioRead(pin);	// current power level
					if (value)
						return;	// no need for manual control
				}

			file="/sys/devices/platform/wlan_pwrseq/enable_power";
			f=fopen(file, "w");
			if(!f) {
				perror(file);
				exit(1);
			}
			fprintf(f, "%d\n", value);	// directly control the wlan_pwrseq-gpio
			fclose(f);
			return;
		}
		case TARGET_IO0_Pin: {
			file="/sys/devices/platform/wlan_pwrseq/enable_flashing";
			f=fopen(file, "w");
			if(!f) {
				perror(file);
				exit(1);
			}
			// value = 0 means "active", i.e. set to 1
			fprintf(f, "%d\n", !value);	// directly control the wlan_pwrseq-gpio
			fclose(f);
			return;
		}
	}
	fprintf(stderr, "unknown pin %u\n", pin);
	return;
}

uint32_t gpioDelay(uint32_t micros)
{
	usleep(micros);
	return micros;
}

int serOpen(const char *device, speed_t baud, int unused)
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
