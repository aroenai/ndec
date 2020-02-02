#include "yaz0.c"
#include "crc.c"
#include <stdio.h>
#include <stdint.h>
#include "endian.h"

typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t i32;

u32 * romd;
u32 * outd;
char clean; // whether to do a clean decompress

i32 get_ftable_offset()
{
    u32 i;
    i = 0;
    while(i+4 < 0x01000000)
    {
        if ( swap32(romd[i  ]) == 0x7A656C64 // filetable "fingerprint" ("zelda@srd")
          && swap32(romd[i+1]) == 0x61407372 // shows up 16-byte aligned a few 16-byte lines before the start of the filetable
          &&(swap32(romd[i+2])               // the second file in the filetable ALWAYS refers to 0x00001060 -- if this assumption is wrong, please correct me!
            &0xFF000000)           == 0x64000000)//
        {
            while(swap32(romd[i]) != 0x00001060) i += 4; // second filesystem entry always starts with 000001060
            puts("Found file table!");
            printf("%X\n", (i-4)*sizeof(u32));
            return (i-4)*sizeof(u32);
        }
        i += 4;
    }
    return -1;
}

int load_rom(char * filename)
{
    printf("Loading ROM %s . . . ", filename);
    
	FILE *romf = fopen(filename, "rb");
	if (romf == NULL)
	{ 
        puts("Error opening file!");
		return -1;
	}
    
	fseek(romf, 0, SEEK_END);
	u32 size = ftell(romf);
	fseek(romf, 0, SEEK_SET);
    
	if (size != fread(romd, sizeof(char), size, romf)) 
	{ 
        puts("Error reading file!");
        puts("Is rom filesize in octets divisible by 4? It should be.");
		return -1;
	} 
	fclose(romf);
    
    printf("Loaded ROM\nSize: %X\nWord 0x24F70: %X\n", size, swap32(romd[0x24F70/sizeof(u32)]));
	
	puts("Copying ROM in RAM");
	memcpy(outd, romd, size);
    
    return 0;
}

u32 * ftab1;
u32 * ftab2;
u32 ftabsize;
u32 ftabnum;

typedef struct fdata
{
    u32 vstart;
    u32 vend;
    u32 pstart;
    u32 pend;
} fdata;

fdata get_file_data(u32 i)
{
    fdata my_zelda_file;
    my_zelda_file.vstart = swap32(ftab1[i*4  ]);
    my_zelda_file.vend   = swap32(ftab1[i*4+1]);
    my_zelda_file.pstart = swap32(ftab1[i*4+2]);
    my_zelda_file.pend   = swap32(ftab1[i*4+3]);
    return my_zelda_file;
}

u32 * set_file_data(u32 i, fdata relevant)
{
    ftab2[i*4  ] = swap32(relevant.vstart);
    ftab2[i*4+1] = swap32(relevant.vend);
    ftab2[i*4+2] = swap32(relevant.pstart);
    ftab2[i*4+3] = swap32(relevant.pend);
	//printf("%X, %X, %X, %X\n", relevant.pend, ftab2[i*4+3], &ftab2[i*4+3], outd);
    return &ftab2[i];
}

void write_rom(char * filename)
{
    FILE * outfile = fopen(filename, "wb");
    fwrite(outd, sizeof(u32), 0x01000000, outfile);
    fclose(outfile);
}

int main(int argc, char * argv[])
{
    if(argc < 3)
    {
        puts("ndec decompresses Zelda 64 ROMs.");
        puts("Usage: ndec inrom outrom");
        return 0;
    }
	
	clean = 0;
	if(argc >= 4)
	{
		clean = (strcmp(argv[3], "-clean") == 0);
	}
    
    romd = (u32*)malloc(0x04000000);
    outd = (u32*)malloc(0x04000000);
	printf("%X, %X\n", romd, outd);
    if(load_rom(argv[1]) < 0)
        return -1;
    
    i32 ftable_offset = get_ftable_offset();
	if(ftable_offset < 0)
	{
		puts("Could not find file table.");
		return -1;
    }
    ftab1 = (void *)romd + ftable_offset;
    ftab2 = (void *)outd + ftable_offset;
    ftabsize = get_file_data(2).vend - get_file_data(2).vstart;
    ftabnum = ftabsize/16;
    printf("Filesystem: %X ~ %X\n", get_file_data(2).vstart, get_file_data(2).vend);
    printf("Number of files: %X\n", ftabnum);
	
	if(clean)
	{
		puts("Cleaning file boundaries.");
		unsigned long offset = get_file_data(2).vend;
		memset((uint8_t*)(outd)+offset,0,0x04000000-offset);
    }
	
    puts("Decompressing files...");
    long i = ftabnum-1; // signed to break out of while loop
    while(i >= 0)
    {
        if (i % 10 == 0)
            printf("%d ", i);
        fdata current_file = get_file_data(i);
        if ( current_file.pend == 0xFFFFFFFF // Nothing to do (used in mm debug filesystem -- stub file?)
		   || i == 2 ) // Do not decompress file table! We rebuild it manually
        {
            i -= 1;
            continue;
        }
        if(current_file.pend == 0x00000000) // Need to move the file (already decompressed)
            //memcpy ( void * destination, const void * source, size_t num );
            memcpy((void*)outd + current_file.vstart, (void*)romd + current_file.pstart, current_file.vend - current_file.vstart);
        else                                // Need to decompress to a new location
            //yaz0_decode (u8 *src, u8 *dst, int uncompressedSize)
            yaz0_decode((void*)romd + current_file.pstart, (void*)outd + current_file.vstart, current_file.vend - current_file.vstart);
        current_file.pstart = current_file.vstart;
        current_file.pend   = 0;
        set_file_data(i, current_file);
        i -= 1;
    }
    
    puts("Decompressed! Writing new ROM.");
    write_rom(argv[2]);
    fix_crc(argv[2]);
	
    return 0;
}