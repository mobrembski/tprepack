#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/evp.h>

#define FOOTER_TP_PRODUCT_ID_OFFSET 0x4C
#define FOOTER_TP_PRODUCT_ID_SIZE 4
#define FOOTER_VERSION_OFFSET 0x50
#define FOOTER_VERSION_SIZE 4
#define FOOTER_MD5_CHECKSUM_OFFSET 0xD4
#define FOOTER_SIZE 0xE8

#define HEADER_SIZE 0x100
#define HEADER_MAGIC_NUMBER 0x32524448
#define HEADER_MAGIC_DEVICE 0x00000100
#define HEADER_MAGIC_NUMBER_OFFSET 0
#define HEADER_MAGIC_DEVICE_OFFSET 4
#define HEADER_TCLINUX_SIZE_OFFSET 8
#define HEADER_TCLINUX_CRC_OFFSET  0x0C

const char crc32_c[] = "0000000077073096ee0e612c990951ba076dc419706af48fe963a5359e6495a30edb883279dcb8a4e0d5e91e97d2d98809b64c2b7eb17cbde7b82d0790bf1d911db710646ab020f2f3b9714884be41de1adad47d6ddde4ebf4d4b55183d385c7136c9856646ba8c0fd62f97a8a65c9ec14015c4f63066cd9fa0f3d638d080df53b6e20c84c69105ed56041e4a26771723c03e4d14b04d447d20d85fda50ab56b35b5a8fa42b2986cdbbbc9d6acbcf94032d86ce345df5c75dcd60dcfabd13d5926d930ac51de003ac8d75180bfd0611621b4f4b556b3c423cfba9599b8bda50f2802b89e5f058808c60cd9b2b10be9242f6f7c8758684c11c1611dabb6662d3d76dc419001db710698d220bcefd5102a71b1858906b6b51f9fbfe4a5e8b8d4337807c9a20f00f9349609a88ee10e98187f6a0dbb086d3d2d91646c97e6635c016b6b51f41c6c6162856530d8f262004e6c0695ed1b01a57b8208f4c1f50fc45765b0d9c612b7e9508bbeb8eafcb9887c62dd1ddf15da2d498cd37cf3fbd44c654db261583ab551cea3bc0074d4bb30e24adfa5413dd895d7a4d1c46dd3d6f4fb4369e96a346ed9fcad678846da60b8d044042d7333031de5aa0a4c5fdd0d7cc95005713c270241aabe0b1010c90c20865768b525206f85b3b966d409ce61e49f5edef90e29d9c998b0d09822c7d7a8b459b33d172eb40d81b7bd5c3bc0ba6cadedb883209abfb3b603b6e20c74b1d29aead547399dd277af04db261573dc1683e3630b1294643b840d6d6a3e7a6a5aa8e40ecf0b9309ff9d0a00ae277d079eb1f00f93448708a3d21e01f2686906c2fef762575d806567cb196c36716e6b06e7fed41b7689d32be010da7a5a67dd4accf9b9df6f8ebeeff917b7be4360b08ed5d6d6a3e8a1d1937e38d8c2c44fdff252d1bb67f1a6bc57673fb506dd48b2364bd80d2bdaaf0a1b4c36034af641047a60df60efc3a867df55316e8eef4669be79cb61b38cbc66831a256fd2a05268e236cc0c7795bb0b4703220216b95505262fc5ba3bbeb2bd0b282bb45a925cb36a04c2d7ffa7b5d0cf312cd99e8b5bdeae1d9b64c2b0ec63f226756aa39c026d930a9c0906a9eb0e363f720767850500571395bf4a82e2b87a147bb12bae0cb61b3892d28e9be5d5be0d7cdcefb70bdbdf2186d3d2d4f1d4e24268ddb3f81fda836e81be16cdf6b9265b6fb077e118b7477788085ae6ff0f6a7066063bca11010b5c8f659efff862ae69616bffd3166ccf45a00ae278d70dd2ee4e0483543903b3c2a7672661d06016f74969474d3e6e77dbaed16a4ad9d65adc40df0b6637d83bf0a9bcae53debb9ec547b2cf7f30b5ffe9bdbdf21ccabac28a53b3933024b4a3a6bad03605cdd7069354de572923d967bfb3667a2ec4614ab85d681b022a6f2b94b40bbe37c30c8ea15a05df1b2d02ef8d";
#define crc32_c_size ((sizeof(crc32_c) - 1) / sizeof(char))
unsigned char crc32_m[crc32_c_size >> 1];
#define crc32_size sizeof(crc32_m) / sizeof(char)

uint32_t swap_uint32( uint32_t val )
{
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | (val >> 16);
}

uint32_t calc_crc32(uint32_t sum, const char *filename, size_t offset, size_t fileSize) {
  for(size_t i=offset; i< fileSize; i++) {
    sum = swap_uint32(*((uint32_t*)(crc32_m + (((filename[i] ^ sum) & 0xFF) << 2)))) ^ sum >> 8;
  }
  
  return sum;
}

void init_crc32(unsigned char *crc_input)
{
    unsigned int j;
    unsigned int i;
    char cnv[] = {0, 0};
    for (i = 0; i < crc32_size; i++) {
        j = i << 1;
        cnv[0] = crc32_c[j];   
        const int high = (int) strtol(cnv, NULL, 16);
        cnv[0] = crc32_c[j + 1];
        const int low = (int) strtol(cnv, NULL, 16);
        const int val = 16 * high + low;
        //printf("%d %d %d %02x\n", low, high, val, val);
        crc_input[i] = val;
    } 
}

int main(int argc, char *argv[])
{
    char *filename="output_firmware.bin";
    EVP_MD_CTX *mdctx;
    unsigned char *md5_digest;
    int i;
    FILE *inFile = fopen (filename, "rb");
    unsigned int md5_digest_len = EVP_MD_size(EVP_md5());

    if (inFile == NULL) {
        printf ("%s can't be opened.\n", filename);
        return 0;
    }

    fseek(inFile, 0, SEEK_END);
    size_t fsize = ftell(inFile);
    fseek(inFile, 0, SEEK_SET);

    char *fileCont = malloc(fsize + 0xE8);
    fread(fileCont, fsize, 1, inFile);
    fclose(inFile);
    init_crc32(crc32_m);

    uint32_t sum = 0xFFFFFFFF;
    sum = calc_crc32(sum, fileCont, HEADER_SIZE, fsize);

    const uint32_t found_magic_number = swap_uint32(*((uint32_t*)(fileCont + HEADER_MAGIC_NUMBER_OFFSET)));
    printf("Header Magic number: 0x%08X found 0x%08X ...%s\n", HEADER_MAGIC_NUMBER, found_magic_number, HEADER_MAGIC_NUMBER == found_magic_number ? "ok" : "failed");
    const uint32_t found_magic_device = swap_uint32(*((uint32_t*)(fileCont + HEADER_MAGIC_DEVICE_OFFSET)));
    printf("Header Magic device: 0x%08X found 0x%08X ...%s\n", HEADER_MAGIC_DEVICE, found_magic_device, HEADER_MAGIC_DEVICE == found_magic_device ? "ok" : "failed");
    const uint32_t found_tclinux_size =  swap_uint32(*((uint32_t*)(fileCont + HEADER_TCLINUX_SIZE_OFFSET)));
    printf("tclinux.bin size: %lu (0x%08X) found %lu (0x%08X) ...%s\n", fsize, found_tclinux_size, fsize, found_tclinux_size, fsize == found_tclinux_size ? "ok" : "failed");
    const uint32_t found_tclinux_checksum = swap_uint32(*((uint32_t*)(fileCont + HEADER_TCLINUX_CRC_OFFSET)));
    printf("tclinux.bin checksum: 0x%08X found 0x%08X ...%s\n", sum, found_tclinux_checksum, sum == found_tclinux_checksum ? "ok" : "failed");
    sum = swap_uint32(sum);
    memcpy(fileCont + HEADER_TCLINUX_CRC_OFFSET, &sum, sizeof(uint32_t));
    printf("Checksum fixed. Creating footer...\n");

    char footer[0xE8]={0x6f,0x6b,0x30,0x6f,0x77,0x70,0x61,0x6b,0x64,0x69,0x65,0x39,0x38,0x77,0x64,0x6b,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2b,0xdc,0xf2,0x01,
                0x01,0x00,0x00,0x00,0x03,0x01,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    uint32_t tpProductId = swap_uint32(1183166722);
    memcpy(footer + FOOTER_TP_PRODUCT_ID_OFFSET, &tpProductId, 4);
    memcpy(&tpProductId, footer + FOOTER_TP_PRODUCT_ID_OFFSET, 4);
    printf("TP Product ID: 0x%x (%d)\n",swap_uint32(tpProductId),swap_uint32(tpProductId));
    footer[FOOTER_VERSION_OFFSET] = 2;
    footer[FOOTER_VERSION_OFFSET + 1] = 0;
    footer[FOOTER_VERSION_OFFSET + 2] = 0;
    footer[FOOTER_VERSION_OFFSET + 3] = 1;
    uint8_t tpVersion[4];
    memcpy(tpVersion, footer + FOOTER_VERSION_OFFSET, 4);
    printf("TP Version: %d.%d.%d.%d\n",tpVersion[0],tpVersion[1],tpVersion[2],tpVersion[3]);
    printf("TP Version in HEX: 0x%x.0x%x.0x%x.0x%x\n",tpVersion[0],tpVersion[1],tpVersion[2],tpVersion[3]);
    uint32_t mark[4] = { 0xA53AD7DC, 0xFB9895C3, 0xF4E7F9DC, 0x3747AE0E};
    memcpy(fileCont + fsize, footer, FOOTER_SIZE);
    memcpy(fileCont + fsize + FOOTER_MD5_CHECKSUM_OFFSET, (void*) mark, 0x10);

    mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, EVP_md5(), NULL);
    EVP_DigestUpdate(mdctx, fileCont, fsize + FOOTER_SIZE);
    md5_digest = (unsigned char *)OPENSSL_malloc(md5_digest_len);
    EVP_DigestFinal_ex(mdctx, md5_digest, &md5_digest_len);

    printf("Calculated CRC:");
    for(i = 0; i < md5_digest_len; i++) printf("%02x", md5_digest[i]);
    printf (" %s\n", filename);
    memcpy(fileCont + fsize + FOOTER_MD5_CHECKSUM_OFFSET, (void*) md5_digest, 0x10);

    inFile = fopen ("result.bin", "wb");
    //fseek(inFile,0,SEEK_END);
    fseek(inFile,0,SEEK_SET);
    fwrite(fileCont, 1, fsize + FOOTER_SIZE, inFile);
    fclose (inFile);
    EVP_MD_CTX_free(mdctx);

    return 0;
}
