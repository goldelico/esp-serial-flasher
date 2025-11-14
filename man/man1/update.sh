#!/bin/bash
# run this in this directory after modifying the .groff source files

for SRC in *.groff
do
	BASE=$(basename "$SRC" .groff)
	gzip <$SRC >$BASE.1.gz
done
