/* Flash multiple partitions example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pigpio.h>
#include <sys/param.h>
#include "raspberry_port.h"
#include "esp_loader.h"
#include "example_common.h"

#define TARGET_RST_Pin 2
#define TARGET_IO0_Pin 3

#define DEFAULT_BAUD_RATE 115200
// #define HIGHER_BAUD_RATE  460800
#define HIGHER_BAUD_RATE  DEFAULT_BAUD_RATE
#ifdef __APPLE__
// #define SERIAL_DEVICE     "/dev/cu.usbserial-FTH9L0T7"
#define SERIAL_DEVICE     "/dev/cu.usbserial-FT76GT23"
#else
#define SERIAL_DEVICE     "/dev/ttyS1"
#endif

void usage(char *arg0)
{
	fprintf(stderr, "usage: %s [-b###] [-d/dev/tty]\n", arg0);
	fprintf(stderr, "  -b### set baud rate\n");
	fprintf(stderr, "  -d### set serial device\n");
	exit(1);
}

int main(int argc, char *argv[])
{
    example_binaries_t bin;

    loader_raspberry_config_t config = {
        .device = SERIAL_DEVICE,
        .baudrate = DEFAULT_BAUD_RATE,
        .reset_trigger_pin = TARGET_RST_Pin,
        .gpio0_trigger_pin = TARGET_IO0_Pin,
    };

	char *arg0=argv[0];
	while(argv[1] && argv[1][0] == '-') {
		switch(argv[1][1])
			{
				case 'b': config.baudrate=atoi(argv[1]+2); argv++; break;
				case 'd': config.device=argv[1]+2; argv++; break;
				default: usage(arg0);
			}
	}

	if(argv[1])
		usage(arg0);

    loader_port_raspberry_init(&config);

	printf("Connecting through %s at %u and %u\n", config.device, config.baudrate, HIGHER_BAUD_RATE);

    if (connect_to_target(HIGHER_BAUD_RATE) == ESP_LOADER_SUCCESS) {

		get_example_binaries(esp_loader_get_target(), &bin);

        printf("Loading bootloader...\n");
        flash_binary(bin.boot.data, bin.boot.size, bin.boot.addr);
        printf("Loading partition table...\n");
        flash_binary(bin.part.data, bin.part.size, bin.part.addr);
        printf("Loading app...\n");
        flash_binary(bin.app.data,  bin.app.size,  bin.app.addr);
        printf("Done!\n");
        esp_loader_reset_target();
        loader_port_deinit();


        if (gpioInitialise() < 0) {
            fprintf(stderr, "pigpio initialization failed\n");
            return 1;
        }

        int serial = serOpen(config.device, config.baudrate, 0);
        if (serial < 0) {
            printf("Serial port could not be opened!\n");
        }

        printf("********************************************\n");
        printf("*** Logs below are print from slave .... ***\n");
        printf("********************************************\n");

        // Delay for skipping the boot message of the targets
        gpioDelay(500000);
        while (1) {
            int byte = serReadByte(serial);
            if (byte != PI_SER_READ_NO_DATA) {
                printf("%c", byte);
            }
        }
    }

}
