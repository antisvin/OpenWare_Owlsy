PROJECT=Bluemchen
DEVICE=hw:4,0,0
TOOLROOT=~/bin/arm-gnu-toolchain-12.2.rel1-aarch64-arm-none-eabi/bin/
#TOOLROOT=`pwd`/../Tools/gcc-arm-none-eabi-9-2020-q2-update/bin/

make clean

TOOLROOT=${TOOLROOT} make -j17 CONFIG=Release PLATFORM=Daisy PROJECT=${PROJECT} || exit 1

FirmwareSender -in Build/${PROJECT}.bin -flash `crc32 Build/${PROJECT}.bin` -save Build/${PROJECT}.syx || exit 1

echo "Entering bootloader mode"
amidi -p $DEVICE -S f07d527ef7 || exit 1
sleep 1
echo "Sending firmware"
amidi -p $DEVICE -s Build/${PROJECT}.syx || exit 1
sleep 6
echo "Restart"
amidi -p $DEVICE -S f07d527df7 || exit 1
echo "Done!"
