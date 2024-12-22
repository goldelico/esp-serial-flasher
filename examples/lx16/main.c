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
#define FIRMWARE_DIRECTORY	"/Volumes/CaseSensitive/master/Letux/lib/firmware/espressif/esp32-c6-wroom-1"
#else
#define SERIAL_DEVICE     "/dev/ttyS1"

/*

 root@letux:~# ls -l /lib/firmware/espressif/esp32-c6-wroom-1
 total 964
 -rw-r--r-- 1 502 root    422 Oct 29 12:49 NOTES
 -rw-r--r-- 1 502 root  21344 Oct 29 12:49 bootloader.bin
 -rwxr-xr-x 1 502 root   1316 Oct 29 12:49 flash-firmware.sh
 -rw-r--r-- 1 502 root 935184 Oct 29 12:49 network_adapter.bin
 -rw-r--r-- 1 502 root   8192 Oct 29 12:49 ota_data_initial.bin
 -rw-r--r-- 1 502 root   3072 Oct 29 12:49 partition-table.bin
 root@letux:~#

 To flash, run this command:
 python esptool.py -p $PORT -b 460800 --before default_reset --after hard_reset --chip esp32c6 write_flash --flash_mode dio --flash_size 4MB --flash_freq 80m 0x0 bootloader.bin 0x8000 partition-table.bin 0xd000 ota_data_initial.bin 0x1000 0 network_adapter.bin

 gleiche Syntax erlauben oder das fest einbauen?

 */

#define FIRMWARE_DIRECTORY	"/lib/firmware/espressif/esp32-c6-wroom-1"
#endif

static loader_raspberry_config_t config = {
	.device = SERIAL_DEVICE,
	.baudrate = DEFAULT_BAUD_RATE,
	.reset_trigger_pin = TARGET_RST_Pin,
	.gpio0_trigger_pin = TARGET_IO0_Pin,
};

void usage(char *arg0)
{
	fprintf(stderr, "usage: %s [-bbaud] [-d/dev/tty] [-ffirmware] [-h] [-p0-2]\n", arg0);
	fprintf(stderr, "  -b### set baud rate [%u]\n", config.baudrate);
	fprintf(stderr, "  -d### set serial device [%s]\n", config.device);
	fprintf(stderr, "  -h flash Hello World example\n");
	fprintf(stderr, "  -f### set firmware directory [%s]\n", FIRMWARE_DIRECTORY);
	fprintf(stderr, "  -p### set power [0=off 1=on 2=boot]\n");
	exit(1);
}

int helloworld=0;

int main(int argc, char *argv[])
{
	char *pmode=NULL;
	char *fdirectory=FIRMWARE_DIRECTORY;

    example_binaries_t bin;

	// try to read SERIAL_DEVICE from /proc/device-tree/ahb2/mmc@13460000/wlan@0/espressif,boot-uart

	char *arg0=argv[0];
	while(argv[1] && argv[1][0] == '-') {
		switch(argv[1][1])
			{
				case 'b': config.baudrate=atoi(argv[1]+2); argv++; break;
				case 'd': config.device=argv[1]+2; argv++; break;
				case 'f': fdirectory=argv[1]+2; argv++; break;
				case 'h': helloworld=1; argv++; break;
				case 'p': pmode=argv[1]+2; argv++; break;
				default: usage(arg0);
			}
	}

	if(argv[1])
		usage(arg0);

	if (gpioInitialise() < 0) {
		fprintf(stderr, "gpio initialization failed\n");
		return 1;
	}

   loader_port_raspberry_init(&config);

	if(pmode)
		{
		printf("Switch power mode to %s\n", pmode);
		switch(pmode[0])
			{
				case '0':
					gpioWrite(config.reset_trigger_pin, 0);
					gpioWrite(config.gpio0_trigger_pin, 1);
					exit(0);
				case '1':
					loader_port_reset_target();
					exit(0);
				case '2':
					loader_port_enter_bootloader();
					exit(0);
				default: usage(arg0);
			}
		}

	printf("Connecting through %s at %u and %u\n", config.device, config.baudrate, HIGHER_BAUD_RATE);

    if (connect_to_target(HIGHER_BAUD_RATE) == ESP_LOADER_SUCCESS) {

		if(helloworld)
			{ // Flash Hello World Example
				get_example_binaries(esp_loader_get_target(), &bin);

				printf("Loading bootloader...\n");
				flash_binary(bin.boot.data, bin.boot.size, bin.boot.addr);
				printf("Loading partition table...\n");
				flash_binary(bin.part.data, bin.part.size, bin.part.addr);
				printf("Loading app...\n");
				flash_binary(bin.app.data,  bin.app.size,  bin.app.addr);
			}
		else if(esp_loader_get_target() == ESP32C6_CHIP)
			{
			struct partition {
				char *file;
				unsigned int addr;
			} partitions[] = {
				{ "bootloader", 0x0 },
				{ "partition-table", 0x8000 },
				{ "ota_data_initial", 0xd000 },
				{ "network_adapter", 0x10000 },
			};
			int i;
			const uint8_t *data = NULL;	// buffer shared for all partitions and resized on demand
			size_t size, bufsize=0;
			for(i=0; i<sizeof(partitions)/sizeof(partitions[0]); i++)
				{
				FILE *f;
				char path[PATH_MAX];
				printf("Loading %s...\n", partitions[i].file);
				sprintf(path, "%s/%s.bin", fdirectory, partitions[i].file);
				f=fopen(path, "r");
				if(f == NULL)
					{
					fprintf(stderr, "can't open %s\n", path);
					return 1;
					}
				fseek(f, 0L, SEEK_END);
				size=ftell(f);
				if(size > bufsize)
					data=realloc((void *) data, bufsize=size);
				rewind(f);
				if(fread((void *) data, sizeof(*data), size, f) != size)
					{
					fprintf(stderr, "read error\n");
					return 1;
					}
				fclose(f);
				flash_binary(data, size, partitions[i].addr);
				}
			free((void *) data);
			}
		else
			{
			fprintf(stderr, "can not flash this device\n");
			return 1;
			}

		printf("Done!\n");
        esp_loader_reset_target();
        loader_port_deinit();

        int serial = serOpen(config.device, config.baudrate, 0);
        if (serial < 0) {
            printf("Serial port could not be opened!\n");
        }

        printf("********************************************\n");
        printf("*** Logs below are print from slave .... ***\n");
        printf("********************************************\n");

        // Delay for skipping the boot message of the targets
        // gpioDelay(500000);
        while (1) {
            int byte = serReadByte(serial);
            if (byte != PI_SER_READ_NO_DATA) {
                printf("%c", byte);
            }
        }
    }

}
