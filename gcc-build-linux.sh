#!/bin/bash
DBG_FILE_NAME="c-deltaparser-dbg"
RLS_FILE_NAME="c-deltaparser"
if [ "$1" = "-debug" ]; then
	echo "building with debug - outputting as $DBG_FILE_NAME"
	gcc -std=c99 -g c-deltaparser.c -o $DBG_FILE_NAME
else
	echo "building as release O3 - outputting as $RLS_FILE_NAME"
	gcc -std=c99 -O3 c-deltaparser.c -o $RLS_FILE_NAME
fi
echo "script finished"
