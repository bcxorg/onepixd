#!/bin/sh
if [ ! -f test ]
then
	make -s test || exit 0
fi
if [ $# -eq 0 ]
then
	for i in 0 1 2 3 4 5 6 7 8 9
	do
		for j in 0 1 2 3 4 5 6 7 8 9
		do
			./test -i 127.0.0.1 -p 8101 -h email_sent -P ../key.priv.pem -e123${i}${j}1 -tPST
			./test -i 127.0.0.1 -p 8101 -h email_sent -P ../key.priv.pem -e123${i}${j}2 -tCST
			./test -i 127.0.0.1 -p 8101 -h email_sent -P ../key.priv.pem -e123${i}${j}3 -tMST
			./test -i 127.0.0.1 -p 8101 -h email_sent -P ../key.priv.pem -e123${i}${j}4 -tEST
			sleep 1
		done
	done
else
	./test -i 127.0.0.1 -p 8101 -h email_sent -P ../key.priv.pem -e123987 -tGMT
fi
