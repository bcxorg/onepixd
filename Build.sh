#!/bin/sh

OURPWD=`pwd`
PROJECT=onepixd

perform()
{
	$*
	status=$?
	if [ $status != 0 ]
	then
		echo Fatal ERROR: "\"$*\" failed."
		exit 1
	fi
}

case $1 in
clean)
	echo "Cleaning ${PROJECT}"
	if [ -f Makefile ]
	then
		perform "make -s clean"
	fi
	rm -f *.gz
	rm -f gmilt.pid
	rm -f gmilt.stats.cache
	rm -f gmilt.pdf
	exit 0
	;;
dist)
	if [ -f docs/gmilt.pdf ]
	then
		cp docs/gmilt.pdf gmilt.pdf
	else
		echo "docs/gmilt.pdf: File missing."
	fi
	if [ -f Makefile ]
	then
		make dist
	fi
	exit 0
	;;
*)
	;;
esac

echo "Building ${PROJECT}"

if [ ! -f config.h.in -a -f config.h.in~ ]; then
    echo cp config.h.in~ config.h.in
    perform "cp config.h.in~ config.h.in"
fi

ctags *.c
autoreconf -v -i

CONF="./configure --with-wall  -q"
echo ${CONF}
perform "$CONF"
#perform "make -s"
exit 0
