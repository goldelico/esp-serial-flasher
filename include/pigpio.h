/* hardeware/gpio.h */

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

#define PI_OUTPUT 1

int gpioInitialise(void);
int gpioSetMode(int pin, int mode);
int gpioTerminate(void);
int gpioWrite(int pin, int value);
