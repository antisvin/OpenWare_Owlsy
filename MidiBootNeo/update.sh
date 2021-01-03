#make clean
TOOLROOT=`pwd`/../Tools/gcc-arm-none-eabi-9-2019-q4-major/bin/ make -j17 CONFIG=Debug PLATFORM=Neophyte || exit 1
make PLATFORM=Neophyte deploy
