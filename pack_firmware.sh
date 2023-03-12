#!/bin/bash
echo "Packing firmware...Please wait"
sudo mksquashfs squashfs-root/ packed.squash -comp lzma
echo "Rootfs created as packed.squash"
cat extracted_header.bin > output_firmware.bin
cat packed.squash >> output_firmware.bin
echo "Firmware joined as output_firmware.bin"
echo "Generating footer..."
./tprepack
echo "Done...Please double check size. If your file ends at lower 003800e8, then it's good"
hexdump -C result.bin | tail -n 10
RESULT_SIZE=$(stat -c %s result.bin)
if [ $RESULT_SIZE -gt 3670248 ] ; then
  echo "Sorry, you have exceed flash size"
else
  echo "All good! use result.bin to update device"
fi


