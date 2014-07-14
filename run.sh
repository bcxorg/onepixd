#!/bin/sh
while `/bin/true`
do
	./onepixd -f -c onepixd.conf
	logger -pmail.info "onepixd died and restarted."
	sleep 2
done
