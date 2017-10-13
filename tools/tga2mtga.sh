#!/bin/sh

FPS=30

if [ ! $1 ]; then
	echo "Usage: tga2mtga.sh filename.mjpg [fps]"
	echo "Creates mtga out of all tga files in current directory using mencoder."
	exit
fi

if [ $2 ]; then
	FPS=$2
fi

mencoder mf://*.tga -quiet -mf fps=$FPS -ovc copy -o $1
