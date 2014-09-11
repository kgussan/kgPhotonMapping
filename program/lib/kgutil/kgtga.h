// TGA画像リーダライタ
#ifndef __TGA_H__
#define __TGA_H__
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <ctype.h>
#include "kgmacro.h"
namespace nsTga{
const unsigned char gUncompressedTgaHeader[12]={0,0,2,0,0,0,0,0,0,0,0,0};	// Uncompressed TGA Header
const unsigned char gCompressedTgaHeader[12]={0,0,10,0,0,0,0,0,0,0,0,0};	// Compressed TGA Header

typedef	struct									
{
	unsigned char	*imageData;
	unsigned int	bitPerPixel;
	unsigned int	width;
	unsigned int	height;
	unsigned int	texID;
	//unsigned int	type;
}Texture_t;	

typedef struct
{
	unsigned char Header[12]; // TGA File Header
}TGAHeader_t;
typedef struct
{
	unsigned char	header[6]; // First 6 Useful Bytes From The Header
	unsigned int	bytesPerPixel; // Holds Number Of Bytes Per Pixel Used In The TGA File
	unsigned int	imageSize; // Used To Store The Image Size When Setting Aside Ram
	unsigned int	temp; // Temporary Variable
	unsigned int	type;	
	unsigned int	Height; //Height of Image
	unsigned int	Width; //Width ofImage
	unsigned int	Bpp; // Bits Per Pixel
} TGA_t;

bool LoadTGA(Texture_t *texture,char *filename);
bool LoadUncompressedTGA(Texture_t *,char *,FILE *);
bool LoadCompressedTGA(Texture_t *,char *,FILE *);

} //nsTga

#endif // __TGA_H__
