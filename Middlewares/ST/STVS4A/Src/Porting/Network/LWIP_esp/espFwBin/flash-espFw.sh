#!/bin/bash
COM=$(./STUsbCom.exe  --mute listcom)
VERSION_FW=1.0


echo "| ---------------------------------------------------- |"
echo "|       Wifi espFW Flash for STVS4A                    |"
echo "|                                                      |"
echo "| Connect the USB to the PC                            |"
echo "| Close all applications using $COM                    |"
echo "| Select Flash esp from the STVS4A GUI Wifi  page      |"
echo "|                                                      |"
echo "| esp-01 GPIO0 must be wired  ( check GUI screen)      |"
echo "|                                                      |"
echo "| When it is done hit any key                          |"
echo "|                                                      |"
echo "| If the download doesn't start,pushes the user button |"
echo "|                                                      |"
echo "| ---------------------------------------------------- |"

read
echo Flashing using the port $COM

./esp_tool.exe  -p$COM -b115200 -rnone -of \
-a0x00000 "espFw.eagle.flash-$VERSION_FW-0x00000.bin"  \
-a0x20000 "espFw.eagle.irom0text-$VERSION_FW-0x20000.bin" \
-a0x7E000 "blank-$VERSION_FW-0x7E000.bin" \
-a0x7C000 "esp_init_data_default-$VERSION_FW-0x7C000.bin"
echo
echo "hit a key to close the window"
read