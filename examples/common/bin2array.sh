# Creates C resources file from files in given directory recursively

DIR=$(dirname "$0")
cd $DIR/../binaries

find . -name '*.bin' | while read FILE
do
	(
		echo "#include <stdint.h>"
		xxd -i $FILE | sed 's/_.*\(ESP.*\)/\1/' | sed 's/unsigned char/const uint8_t/' | sed 's/bin_len/bin_size/'
	) >$(dirname "$FILE")/$(basename "$FILE" .bin).c
done
