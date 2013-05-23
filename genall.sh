#!/bin/bash

rm -f configure.scan
mv configure.ac configure.ac_bak
autoscan
mv configure.ac_bak configure.ac

if [ $# -ge 1 ]; then
	vimdiff configure.ac configure.scan
fi

aclocal -I m4
libtoolize --force --copy
autoheader
autoconf
automake --add-missing
chmod +x configure
