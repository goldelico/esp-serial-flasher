/* hardeware/gpio.h */

#include <stdint.h>

#ifdef __APPLE__

/* speed constants not directly supported by macOS */
#define B460800 460800
#define B500000 500000
#define B576000 576000
#define B921600 921600
#define B1000000 1000000
#define B1152000 1152000
#define B1500000 1500000
#define B2000000 2000000
#define B2500000 2500000
#define B3000000 3000000
#define B3500000 3500000
#define B4000000 4000000

#endif

#define PI_OFF   0
#define PI_ON    1

#define PI_CLEAR 0
#define PI_SET   1

#define PI_LOW   0
#define PI_HIGH  1

#define PI_INPUT  0
#define PI_OUTPUT 1

#define PI_PUD_OFF  0
#define PI_PUD_DOWN 1
#define PI_PUD_UP   2

int gpioInitialise(void);
int gpioSetMode(unsigned pin, unsigned mode);
void gpioTerminate(void);
void gpioWrite(unsigned pin, unsigned level);
uint32_t gpioDelay(uint32_t micros);

int serOpen(const char *device, int baudrate, int unused);

#define PI_SER_READ_NO_DATA -1
int serReadByte(int fd);
