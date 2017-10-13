#!/bin/sh

if [ ! $1 ]; then
	echo "Usage: adpplay.sh filename.adp [OPTIONS]"
	echo "See 'adp2wav --help' for possible options."
	exit
fi

adp2wav $@ | play -t wav -
