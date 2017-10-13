#!/bin/sh

JPG_QUALITY=85
FPS=30

if [ ! $1 ]; then
	echo "Usage: tga2mjpg.sh filename.mjpg [fps] [jpg_quality]"
	echo "Creates mjpg out of all tga files in current directory using mencoder."
	exit
fi

if [ $2 ]; then
	FPS=$2
fi

if [ $3 ]; then
	JPG_QUALITY=$3
fi

for x in *.tga; do
	base=${x%%.*}
	echo "Converting $base to jpg..."
	convert -quality $JPG_QUALITY "$x" "$base.jpg"
done

mencoder mf://*.jpg -quiet -mf fps=$FPS -ovc copy -o $1
rm -f *.jpg
