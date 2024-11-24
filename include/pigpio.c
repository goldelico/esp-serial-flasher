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

int fd;
void *map_base;

int gpioInitialise(void)
{
	printf("%s\n", __func__);
#ifdef __mips__

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
	printf("gpio base %08x\n", GPIO_BASE & ~MAP_MASK);
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO_BASE & ~MAP_MASK);

	printf("map_base %08x\n", map_base);
	printf("register %08x\n", (uint8_t *) map_base + 0x100);
	printf("value %08x\n", *(uint32_t *) ((uint8_t *) map_base + 0x100));

#endif
	return 0;
	return -1;
}

#ifdef __mips__

unsigned raspi2lx16(unsigned pin)
{
#define TARGET_RST_Pin 2
#define TARGET_IO0_Pin 3

	switch(pin) {
		case 2: return 32+4;	// PB4
		case 3: return 32+5;	// PB5
	}
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
	printf("read %08x from %08x\n", value, lx16_addr(pin, reg));
	return value;
}

void lx16_write(unsigned pin, unsigned reg, uint32_t value)
{ // base address
	printf("write %08x to %08x\n", value, lx16_addr(pin, reg));
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
	printf("%s\n", __func__);
#ifdef __mips__
	close(fd);
#endif
}

int gpioSetMode(unsigned pin, unsigned mode)
{ // set gpio mode
	printf("%s(%u, %u)\n", __func__, pin, mode);
#ifdef __mips__
#define PxPINL	0x00
#define PxINT	0x10
#define PxINTS	0x14
#define PxINTC	0x18
#define PxMSK	0x20	// bit = 1 => GPIO
#define PxMSKS	0x24	// bit = 1 => GPIO
#define PxMSKC	0x28	// bit = 1 => no GPIO
#define PxPAT0	0x40
#define PxPAT1	0x30
#define PxPU0	0x80
#define PxPUS	0x84
#define PxPUC	0x88

	uint32_t value;
	/*
	 gpout 0: INT=0 MASK=1 PAT1=0 PAT0=0
	 gpout 1: INT=0 MASK=1 PAT1=0 PAT0=1
	 gpin:    INT=0 MASK=1 PAT1=1 PAT0=x
	 */
	lx16_read(raspi2lx16(pin), PxINT);	// read back
	lx16_write(raspi2lx16(pin), PxINTC, lx16_mask(raspi2lx16(pin)));	// disable INT on this pin
	lx16_read(raspi2lx16(pin), PxINT);	// read back

	lx16_read(raspi2lx16(pin), PxMSK);	// read back
	lx16_write(raspi2lx16(pin), PxMSKS, lx16_mask(raspi2lx16(pin)));	// set MSK to make it a GPIO
	lx16_read(raspi2lx16(pin), PxMSK);	// read back
	value = lx16_read(raspi2lx16(pin), PxPAT1);
	switch(mode)
		{
			case PI_OUTPUT:
				value &= ~lx16_mask(raspi2lx16(pin));	// set to output
				break;
			case PI_INPUT:
				value |= lx16_mask(raspi2lx16(pin));	// set to input
				break;
			default:
				return -1;
		}
	lx16_write(raspi2lx16(pin), PxPAT1, value);
	lx16_read(raspi2lx16(pin), PxPAT1);	// read back
	// disable pull-ups PI_PUD_OFF?
	lx16_read(raspi2lx16(pin), PxPU0);	// current value
	lx16_write(raspi2lx16(pin), PxPUC, lx16_mask(raspi2lx16(2)) | lx16_mask(raspi2lx16(3))); // disable internal pull-ups on PB4 and PB5
	lx16_read(raspi2lx16(pin), PxPU0);	// read back
#endif
	return 0;
}

void gpioWrite(unsigned pin, unsigned level)
{
	printf("%s(%u, %u)\n", __func__, pin, level);
#ifdef __mips__

	/*
	 gpout 0: INT=0 MASK=1 PAT1=0 PAT0=0
	 gpout 1: INT=0 MASK=1 PAT1=0 PAT0=1
	 gpin:    INT=0 MASK=1 PAT1=1 PAT0=x
	 */
	uint32_t value;

	switch(pin) {
		case 2: // PB4: 1 = enable, 0 = power down
		case 3:	// PB5: 1/input = normal boot, 0 = flashing
			value = lx16_read(raspi2lx16(pin), PxPAT0);
			if(level)
				value |= lx16_mask(raspi2lx16(pin));	// set high
			else
				value &= ~lx16_mask(raspi2lx16(pin));	// set low
			lx16_write(raspi2lx16(pin), PxPAT0, value);	// set gpio value (assumin PI_OUTPUT)
			lx16_read(raspi2lx16(pin), PxPAT0);	// read back
			break;
	}
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
