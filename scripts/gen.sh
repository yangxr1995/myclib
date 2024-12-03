#!/bin/bash

[ $# -ne 1 ] && echo "usage : $0 <target>" && exit 1

TARGET=${1}
ORIGIN="sercmd"

find "./src" -type f -exec sed -i "s/${ORIGIN}/${TARGET}/g" {} \;
find "./include" -type f -exec sed -i "s/${ORIGIN}/${TARGET}/g" {} \;
sed -i "s/${ORIGIN}/${TARGET}/g" ./Makefile
mv src/${ORIGIN}.c src/${TARGET}.c
mv include/${ORIGIN}.h include/${TARGET}.h

echo "替换完成。"
