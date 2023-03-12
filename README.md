
# XZ000-G3 ModKit

  

## How to modify FW

  

First, you need a valid image file as a source. Search over web, or use serial console to extract it from device.

  

## Extracting FW from console

  

Open the case. No screws, 2 latches on each side except front one with LEDs - 3 latches. When you open, you will see unpopulated 2.54 header with very good description on PCB.

Use UART converter to connect to it, and use following commands to extract and send dump over TFTP:

```

cat /dev/mtd0 > /tmp/mtddump

tftp -p -l /tmp/mtddump -r mtddump 192.168.YOUR.TFTP.SERVER

```

Please keep in mind most TFTP servers disallow file creation, so you should create mtddump file with proper permissions on tftproot.

  

# Some Theory

  

## Flash layout

  

Boot environment related, not included in update package:

```

<5>0x000000000000-0x000000020000 : "tcboot"

<5>0x000000020000-0x000000030000 : "romfile"

<5>0x000000030000-0x000000040000 : "factoryinfo"

factoryinfo part contains MAC address in first 6 bytes.

<5>0x000000040000-0x000000050000 : "loid"

<5>0x000000050000-0x000000060000 : "hwinfo"

hwinfo contains TpProductID which is comparable which image during flashing progress

<5>0x000000060000-0x000000070000 : "config"

<5>0x000000070000-0x000000080000 : "iot"

Start of update package.

<5>0x000000080000-0x0000001b0000 : "kernelA"

<5>0x0000001b0000-0x000000400000 : "rootfsA"

```

At start of kernel, first 256 bytes is a TCLinux header. More on that later.

  

## Update package footer

  

Last 0xE8 bytes (232 in DEC) is the update image footer. Most bytes in footer has unknown meaning, and they're not used by flashing tool on device. However, there are some fields here which are important:

- TP Product ID, 4 bytes, UInt32 at offset 0x4C. Image must contain exactly same TP Product ID as device. To check what TP Product ID has your device, connect serial port and observer console

over update process.

- Version, 4 bytes, array of UInt8 at offset 0x50. Format %u.%u.%u.%u. Unsure the meaning, but it is used by device to determine if image is in version at least 2.

- MD5 checksum of all image, offset 0xD4. Flash script calculating MD5 checksum of all file, including footer to check if image is correct. During calculation, it assumes that MD5 checksum field has following value:

{ 0xA53AD7DC, 0xFB9895C3, 0xF4E7F9DC, 0x3747AE0E};

  

Footer is not saved on flash, it's removed by flash tool. This indicates that it's not possible to easily re-flash extracted firmware from device via web config.

  

## TCLinux Header

  

Checksum in Update package is used only by flash tool, and it's not saved on device.

But still, bootcode needs to check if flash contains valid image. To check it, it uses first 256 bytes of kernel partition as a metadata, where also a CRC32 checksum is stored.

I don't want to going into details of it, because it is a standard TCLinux header, and it's well described.

I suggest to see very good blog post from Vasvir about it:

https://vasvir.wordpress.com/2015/03/08/reverse-engineering-trendchip-firmware-zte-h108ns-part-i/

And resulting tool tcrevenge:

https://github.com/vasvir/tcrevenge

  

## Modifying firmware

tprepack was based on tcrevenge, and it includes both header and footer generation.

Run make all. Put your source image as input.bin in current folder, and then run make unpack.

After changes, run make repack and see result in result.bin.

  

Please read carefully messages from script.