#!/bin/bash
# flash firmware

if [ "$(uname)" == "Darwin" ]
then
	SRC=/Volumes/Retrode3/esp-hosted
	PORT=/dev/cu.usbserial-FTH9L0T7	# update to local setup
else
	SRC=/usr/local/src/esp-hosted
	PORT=/dev/tty???	# update to local setup
fi

CHIP=esp32c6
Flashing_Baud_Rate=115200	# 115200, 460800, 921600 etc.
HCI_Baud_Rate=92100		# 'UART baud rate' is for 'Bluetooth over UART' and set to 921600 in this binary
ESPTOOL="python3 $SRC/esp_hosted_ng/esp/esp_driver/esp-idf/components/esptool_py/esptool/esptool.py"

# print info
if true
then
	$ESPTOOL -p $PORT -b $Flashing_Baud_Rate --before default_reset --after hard_reset --chip $CHIP read_mac
	$ESPTOOL -p $PORT -b $Flashing_Baud_Rate --before default_reset --after hard_reset --chip $CHIP flash_id
fi

# make a backup
if false
then
$ESPTOOL -p $PORT -b $Flashing_Baud_Rate --before default_reset --after hard_reset --chip $CHIP \
		read_flash \
		0 ALL flash_contents.bin	# 4MB Flash
fi

# flash new software
if true
then
# FIXME: adjust this to what is written by firmware compile
$ESPTOOL -p $PORT -b $Flashing_Baud_Rate --before default_reset --after hard_reset --chip $CHIP \
		write_flash --flash_mode dio --flash_size 4MB --flash_freq 80m \
		0x0 bootloader.bin \
		0x8000 partition-table.bin \
		0xd000 ota_data_initial.bin \
		0x10000 network_adapter.bin
fi
