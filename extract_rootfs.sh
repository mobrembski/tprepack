#!/bin/bash

if [ -z "$1" ];
then
IMAGE_FILE="./input.bin"
else
IMAGE_FILE="$1"
fi
echo "Using file: "$IMAGE_FILE
echo "Checking image type..."
hexdump -C result_orig.bin | head -n 1 | grep 2RDH
if [ $? -eq 1 ]; then
  echo "Detected full dump!"
  dd if=$IMAGE_FILE of=extracted_updateimage_raw.bin bs=1 skip=$((0x80000))
else
  echo "Detected update image!"
  dd if=$IMAGE_FILE of=extracted_updateimage_raw.bin bs=1 count=$(($(stat -c '%s' $IMAGE_FILE) - 232))
fi
echo "WARNING: Extracting will take some time, please be patient"
echo "During process you will be asked for a password, this is due to unsquashfs cannot fully extract image without root"
echo "Looking for Squashfs image...Please wait"
SQUASHFS_OFFSET=`binwalk extracted_updateimage_raw.bin | grep Squash | awk '{print $1}'`
echo "Found Squashfs! Offset of squashfs: "$SQUASHFS_OFFSET
echo "Copying header with linux to extracted_header.bin"
dd if=extracted_updateimage_raw.bin of=extracted_header.bin bs=1 count=$SQUASHFS_OFFSET
echo "Header has been written"
echo "Copying squashfs to extracted_squashfs"
dd if=extracted_updateimage_raw.bin of=extracted_squashfs.bin bs=1 skip=$SQUASHFS_OFFSET
echo "Squashfs has been written"
echo "Unpacking Squashfs..."
sudo unsquashfs ./extracted_squashfs.bin
echo "DONE! Now you can modify system in squashfs-root. Then run make repack"
