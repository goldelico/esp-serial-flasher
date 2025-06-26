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

#ifdef __mips__

#define PxPINL	0x00
#define PxINT	0x10
#define PxINTS	0x14
#define PxINTC	0x18
#define PxMSK	0x20	// bit = 1 => GPIO
#define PxMSKS	0x24	// bit = 1 => GPIO
#define PxMSKC	0x28	// bit = 1 => no GPIO
#define PxPAT0	0x40
#define PxPAT0S	0x44
#define PxPAT0C	0x48
#define PxPAT1	0x30
#define PxPAT1S	0x34
#define PxPAT1C	0x38
#define PxPU0	0x80
#define PxPUS	0x84
#define PxPUC	0x88
#define PzGID2LD	0xf0

	// may better check for LX16 in /proc/device-tree/model or alike

	// copy tool to LX16 through
	//
	//  scp build/Deployment/esp-serial-flasher.bin/bin/mipsel-linux-gnu/esp-serial-flasher root@192.168.0.202:

	fd=open("/dev/mem", O_RDWR | O_SYNC);
	if(fd < 0) {
		perror("open /dev/mem");
		return -1;
	}
	// alternatively we could mmap the whole gpio register block here
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)
#define GPIO_BASE 0x10010000
#if 0
	printf("gpio base %08x\n", GPIO_BASE & ~MAP_MASK);
#endif
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO_BASE & ~MAP_MASK);

#if 0
	printf("map_base %08x\n", map_base);
	printf("register %08x\n", (uint8_t *) map_base + 0x100);
	printf("value %08x\n", *(uint32_t *) ((uint8_t *) map_base + 0x100));
#endif

#endif
	return 0;
	return -1;
}

#ifdef __mips__

int raspi2lx16(unsigned pin)
{
#define PA 0
#define PB 1
#define PC 2
#define PD 3
#define PZ 7

	switch(pin) {
			// FIXME: read this from /proc/device-tree/ahb2/mmc@13460000/wlan@0/espressif,boot-gpio

//		case TARGET_RST_Pin: return PB*32+4;	// PB4
		case TARGET_IO0_Pin: return PB*32+5;	// PB5
	}
	printf("raspi2lx16: invalid pin %u\n", pin);
	return -1;
}

uint32_t *lx16_addr(unsigned pin, unsigned reg)
{ // base address
	return (uint32_t *) ((uint8_t *) map_base + 0x100 * (pin/32) + reg);	// account for banks A..D
}

uint32_t lx16_read(unsigned pin, unsigned reg)
{ // base address
	uint32_t value;
	value = *lx16_addr(pin, reg);
#if 0
	printf("read %08x from %08x (%08x)\n", value, GPIO_BASE + 0x100 * (pin/32) + reg, lx16_addr(pin, reg));
#endif
	return value;
}

void lx16_write(unsigned pin, unsigned reg, uint32_t value)
{ // base address
#if 0
	printf("write %08x to %08x (%08x)\n", value, GPIO_BASE + 0x100 * (pin/32) + reg, lx16_addr(pin, reg));
#endif
#if 1
	*lx16_addr(pin, reg) = value;
#endif
}

uint32_t lx16_mask(unsigned pin)
{ // bit mask
	return 1<<(pin % 32);	// bit mask
}

#endif	// __mips__

void gpioTerminate(void)
{
#if 0
	printf("%s\n", __func__);
#endif
#ifdef __mips__
	close(fd);
#endif
}

int gpioRead(unsigned pin)
{
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

#ifdef __mips__
	if(raspi2lx16(pin) < 0)
		return -1;
	unsigned int lvl = lx16_read(raspi2lx16(pin), PxPINL);	// read
//	printf("%s: pin %d level raw %08x\n", __func__, lvl);
	lvl &= lx16_mask(raspi2lx16(pin));
//	printf("%s: pin %d level %d\n", __func__, pin, !!lvl);
	return !!lvl;
#else
	return 0;
#endif
}

int gpioSetMode(unsigned pin, unsigned mode)
{ // set gpio mode
#if 1
	printf("%s(%u, %u)\n", __func__, pin, mode);
#endif
	return 0;	// ignore
#ifdef __mips__
	if(raspi2lx16(pin) < 0)
		return -1;

	/*
	 gpout 0: INT=0 MASK=1 PAT1=0 PAT0=0
	 gpout 1: INT=0 MASK=1 PAT1=0 PAT0=1
	 gpin:    INT=0 MASK=1 PAT1=1 PAT0=x
	 */
//	lx16_read(raspi2lx16(pin), PxINT);	// read
//	lx16_read(raspi2lx16(pin), PxMSK);	// read
//	lx16_read(raspi2lx16(pin), PxPAT1);	// read
//	lx16_read(raspi2lx16(pin), PxPAT0);	// read

	lx16_write(PZ*32, PxINTC, lx16_mask(raspi2lx16(pin)));	// disable INT on this pin
	lx16_write(PZ*32, PxMSKS, lx16_mask(raspi2lx16(pin)));	// set MSK to make it a GPIO

	switch(mode)
		{
			case PI_OUTPUT:
				{
				int lvl=gpioRead(pin);
				lx16_write(PZ*32, PxPAT1C, lx16_mask(raspi2lx16(pin)));	// set MSK to make it an output GPIO
				lx16_write(PZ*32, lvl ? PxPAT0S : PxPAT0C, lx16_mask(raspi2lx16(pin)));	// set PAT0 to mirror previous level
				break;
				}
			case PI_INPUT:
				lx16_write(PZ*32, PxPAT1S, lx16_mask(raspi2lx16(pin)));	// set MSK to make it an input GPIO
				break;
			default:
				return -1;
		}

//	lx16_write(PZ*32, PxPAT0C, lx16_mask(raspi2lx16(TARGET_RST_Pin)));	// TARGET_RST_Pin = 0 as default (active high)
	lx16_write(PZ*32, PxPAT0S, lx16_mask(raspi2lx16(TARGET_IO0_Pin)));	// TARGET_IO0_Pin = 1 as default (active low)

	lx16_write(PZ*32, PzGID2LD, PB);	// commit changes for PB

//	lx16_read(raspi2lx16(pin), PxINT);	// read back
//	lx16_read(raspi2lx16(pin), PxMSK);	// read back
//	lx16_read(raspi2lx16(pin), PxPAT1);	// read back
//	lx16_read(raspi2lx16(pin), PxPAT0);	// read back

	// disable pull-ups PI_PUD_OFF?
//	lx16_read(raspi2lx16(pin), PxPU0);	// current value
	lx16_write(raspi2lx16(pin), PxPUC, lx16_mask(raspi2lx16(2)) | lx16_mask(raspi2lx16(3))); // disable internal pull-ups on PB4 and PB5
//	lx16_read(raspi2lx16(pin), PxPU0);	// read back
#endif
	return 0;
}

void gpioWrite(unsigned pin, unsigned value)
{
#if 1
	printf("%s(%u, %u)\n", __func__, pin, value);
#endif
#ifdef __mips__

	/*
	 * Controlling power of the ESP32 on a running Linux system is quite tricky
	 * if we directly control the EN GPIO, the mmc core will detect that the
	 * module is not responding and actively enables it through a vmmc-supply
	 * or mmc-pwrseq-simple which usually controls the EN GPIO.
	 * So we can turn off the ESP32 only for a moment by directly accessing
	 * the GPIO registers.
	 * Therefore,we unbind/bind the esp32_sdio driver, which indirectly controls
	 * the EN GPIO.
	 * Next issue is to bring the ESP32 into flashing mode. While that GPIO
	 * is not controlled by Linux at all, we can directly activate/deactivate
	 * it through the /dev/mem + mmap() interface.
	 * So we could switch it to low level and activate the EN GPIO by binding the
	 * esp32_sdio driver.
	 * But as soon as the ESP32 enters ROM download mode the mmc core again detects
	 * that the ESP32 is not responding and does a reset.
	 * Since here in this library we just know about these two GPIOs but not what
	 * the caller intends to do we need another hack:
	 * - use unbind/bind method for the EN GPIO if the flashing GPIO is high
	 * - use /dev/mem + mmap() for the EN GPIO of the flashing GPIO is low
	 * - there is a special case when we do both
	 */

	// 	printf("before: "); gpioRead(pin);	// current pin level

	char *file;
	FILE *f;
	switch(pin) {
		case TARGET_RST_Pin: {
			if(!value || gpioRead(TARGET_IO0_Pin))
				{ // control gpio through mmc-pwrseq-simple or some regulator - unless we are controlling flashing mode
				  //		printf("%s\n", value? "bind" : "unbind");
#if 1	// more reliable version
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
#else	// is not as reliable
					if(value)
						file="/sys/bus/sdio/drivers/esp32_sdio/bind";
					else
						file="/sys/bus/sdio/drivers/esp32_sdio/unbind";
					f=fopen(file, "w");
					if(!f) {
						perror(file);
						exit(1);
					}
					fprintf(f, "%s\n", "mmc1:0001:1");	// mmc1 controls the GPIO
#endif
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

//	lx16_read(raspi2lx16(pin), PxPAT0);	// read

	lx16_write(PZ*32, value ? PxPAT0S : PxPAT0C, lx16_mask(raspi2lx16(pin)));	// set PAT0 to make it an GPIO = 1
	lx16_write(PZ*32, PzGID2LD, PB);	// commit changes for PB
	sleep(1);

//	lx16_read(raspi2lx16(pin), PxPAT0);	// read back
#endif
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
