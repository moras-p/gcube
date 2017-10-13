#!/bin/sh

FPS=30

if [ ! $1 ]; then
	echo "Usage: tga2avi.sh filename.avi [fps]"
	echo "Creates avi (mpeg4) out of all tga files in current directory using mencoder."
	exit
fi

if [ $2 ]; then
	FPS=$2
fi

mencoder mf://*.tga -mf fps=$FPS -ovc lavc -lavcopts vcodec=mpeg4 -o $1
