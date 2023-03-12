OPENSSL_LIBS = -lssl -lcrypto

CFLAGS = -O2 -g -Wall $(OPENSSL_CFLAGS) $(EXTRACFLAGS)
LDFLAGS	=
LIBS := $(OPENSSL_LIBS)
DEPS = 
OBJ = tprepack 

all: tprepack

help:
	@echo "make - build tprepack"
	@echo "make clean - clean artifacts"
	@echo "make unpack - unpack firmware from file input.bin"
	@echo "make repack - pack again file, and modify headers resulting in result.bin"

tprepack:
	@echo " [tprepack] CC $@"
	@$(CC) $(CFLAGS) -o tprepack tprepack.c $(LIBS) 

	$(SIZECHECK)
	$(CPTMP)

clean:
	rm -f tprepack *.o .*.depend
	rm -f extracted*.bin 
	rm -f output_firmware.bin
	rm -f packed.squash
	rm -f result.bin

unpack: tprepack
	sudo sh extract_rootfs.sh

repack: tprepack
	sudo sh pack_firmware.sh

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

hellomake: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)


