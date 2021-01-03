#!/bin/bash
PROJECT=Neophyte
HARDWARE=3,0,0

make clean
TOOLROOT=`pwd`/../Tools/gcc-arm-none-eabi-9-2019-q4-major/bin/ make -j17 CONFIG=Debug PLATFORM=Neophyte || exit 1
FirmwareSender -in Build/${PROJECT}.bin -flash `crc32 Build/${PROJECT}.bin` -save ${PROJECT}.syx || exit 1
echo "Entering bootloader mode"
amidi -p hw:${HARDWARE} -S f07d527ef7 || exit 1
sleep 1
echo "Sending firmware"
amidi -p hw:${HARDWARE} -s ${PROJECT}.syx || exit 1
sleep 1
echo "Restart"
amidi -p hw:${HARDWARE} -S f07d527df7 || exit 1
echo "Done!"
