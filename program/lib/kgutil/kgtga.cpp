// nehe ‚æ‚èB
#include "kgtga.h"

namespace nsTga{

TGAHeader_t tgaHeader;// TGA header
TGA_t tga;// TGA image data

bool LoadTGA(Texture_t *texture,char *filename)
{
	FILE *fTGA;
	//fTGA = fopen_s( filename, "rb" );
	fopen_s( &fTGA, filename, "rb" );
	if (fTGA==NULL) {
		LogError("Could not open texture file\n");
		return false;
	}
	if (fread(&tgaHeader,sizeof(TGAHeader_t),1,fTGA)==0) {
		LogError("Could not read file header\n");
		if (fTGA!=NULL) {
			fclose(fTGA);
		}
		return false;
	}
	if (memcmp(gUncompressedTgaHeader,&tgaHeader,sizeof(TGAHeader_t))==0) {
		LoadUncompressedTGA(texture, filename, fTGA);
	}else if (memcmp(gCompressedTgaHeader,&tgaHeader,sizeof(TGAHeader_t))==0) {
		LoadCompressedTGA(texture,filename,fTGA);
	}else{
		LogError("TGA file be type 2 or type 10. invalid image format.\n");
		fclose(fTGA);
		return false;
	}
	return true;
}

bool LoadUncompressedTGA(Texture_t *texture,char *filename,FILE *fTGA)
{
	if (fread(tga.header,sizeof(tga.header),1,fTGA)==0) {
		LogError("Could not read info header\n");
		if (fTGA!=NULL) {
			fclose(fTGA);
		}
		return false;
	}
	texture->width  =tga.header[1]*256+tga.header[0]; // highbyte*256+lowbyte
	texture->height =tga.header[3]*256+tga.header[2]; // highbyte*256+lowbyte
	texture->bitPerPixel=tga.header[4];
	tga.Width		=texture->width;
	tga.Height		=texture->height;
	tga.Bpp			=texture->bitPerPixel;
	if ( (texture->width <= 0) || (texture->height <= 0) ||
		( (texture->bitPerPixel != 24) && (texture->bitPerPixel !=32) ) )
	{
		LogError("Invalid texture information\n");
		if (fTGA!=NULL) {
			fclose(fTGA);
		}
		return false;
	}
	tga.bytesPerPixel=(tga.Bpp/8);
	tga.imageSize=(tga.bytesPerPixel*tga.Width*tga.Height);
	texture->imageData=(unsigned char *)malloc(tga.imageSize);
	if (texture->imageData==NULL) {
		LogError("Could not allocate memory for image");
		fclose(fTGA);
		return false;
	}
	if (fread(texture->imageData,1,tga.imageSize,fTGA)!=tga.imageSize) {
		LogError("Could not read image data");
		if (texture->imageData!=NULL) {
			free(texture->imageData);
		}
		fclose(fTGA);
		return false;
	}
	// Byte Swapping Optimized By Steve Thomas
	for (unsigned int cswap=0;cswap<(int)tga.imageSize;cswap+=tga.bytesPerPixel) {
		texture->imageData[cswap] ^= texture->imageData[cswap+2] ^=
		texture->imageData[cswap] ^= texture->imageData[cswap+2];
	}
	fclose(fTGA);
	return true;
}

bool LoadCompressedTGA(Texture_t *texture, char *filename,FILE *fTGA)
{ 
	if (fread(tga.header,sizeof(tga.header),1,fTGA)==0) {
		LogError("Could not read info header");
		if (fTGA!=NULL) {
			fclose(fTGA);
		}
		return false;
	}
	texture->width  =tga.header[1]*256+tga.header[0];//highbyte*256+lowbyte
	texture->height =tga.header[3]*256+tga.header[2];//highbyte*256+lowbyte
	texture->bitPerPixel=tga.header[4];
	tga.Width		=texture->width;
	tga.Height		=texture->height;
	tga.Bpp			=texture->bitPerPixel;
	if ( (texture->width <=0) || (texture->height <=0) ||
		( (texture->bitPerPixel !=24) && (texture->bitPerPixel !=32) ) )
	{
		LogError("Invalid texture information");
		if (fTGA!=NULL) {
			fclose(fTGA);
		}
		return false;
	}
	tga.bytesPerPixel	=(tga.Bpp/8);
	tga.imageSize		=(tga.bytesPerPixel*tga.Width*tga.Height);
	texture->imageData	=(unsigned char *)malloc(tga.imageSize);
	if (texture->imageData==NULL) {
		LogError("Could not allocate memory for image");
		fclose(fTGA);
		return false;
	}
	unsigned int pixelcount=tga.Height*tga.Width;
	unsigned int currentpixel=0;
	unsigned int currentbyte=0;
	unsigned char *colorbuffer=(unsigned char *)malloc(tga.bytesPerPixel);
	do{
		unsigned char chunkheader=0;
		if (fread(&chunkheader,sizeof(unsigned char),1,fTGA)==0) {
			LogError("Could not read RLE header");
			if (fTGA!=NULL) {
				fclose(fTGA);
			}
			if (texture->imageData!=NULL) {
				free(texture->imageData);
			}
			return false;
		}
		if (chunkheader<128) {// If the ehader is < 128, it means the that is the number of RAW color packets minus 1 that follow the header
			chunkheader++;// add 1 to get number of following color values
			for (short counter=0;counter<chunkheader;counter++) {// Read RAW color values
				if (fread(colorbuffer,1,tga.bytesPerPixel,fTGA)!=tga.bytesPerPixel) {// Try to read 1 pixel
					LogError("Could not read image data");// IF we cant, display an error
					if (fTGA!=NULL) {// See if file is open
						fclose(fTGA); // If so, close file
					}
					if (colorbuffer != NULL) {// See if colorbuffer has data in it
						free(colorbuffer); // If so, delete it
					}
					if (texture->imageData != NULL) {// See if there is stored Image data
						free(texture->imageData);// If so, delete it too
					}
					return false;// Return failed
				}
				// write to memory
				texture->imageData[currentbyte	]=colorbuffer[2];// Flip R and B vcolor values around in the process 
				texture->imageData[currentbyte+1]=colorbuffer[1];
				texture->imageData[currentbyte+2]=colorbuffer[0];
				if (tga.bytesPerPixel==4) {// if its a 32 bpp image
					texture->imageData[currentbyte+3]=colorbuffer[3];// copy the 4th byte
				}
				currentbyte+=tga.bytesPerPixel;// Increase thecurrent byte by the number of bytes per pixel
				currentpixel++;// Increase current pixel by 1
				if (currentpixel>pixelcount) {// Make sure we havent read too many pixels
					LogError("Too many pixels read");// if there is too many... Display an error!
					if (fTGA != NULL) {
						fclose(fTGA);
					}	
					if (colorbuffer!=NULL) {// If there is data in colorbuffer
						free(colorbuffer);
					}
					if (texture->imageData!=NULL) {
						free(texture->imageData);
					}
					return false;
				}
			}
		}else{// chunkheader > 128 RLE data, next color reapeated chunkheader - 127 times
			chunkheader -= 127;// Subteact 127 to get rid of the ID bit
			if (fread(colorbuffer, 1, tga.bytesPerPixel, fTGA) != tga.bytesPerPixel) {// Attempt to read following color values
				LogError("Could not read from file");// If attempt fails.. Display error (again)
				if (fTGA != NULL) {// If thereis a file open
					fclose(fTGA);
				}
				if (colorbuffer != NULL) {// If there is data in the colorbuffer
					free(colorbuffer);
				}
				if (texture->imageData != NULL) {// If thereis image data
					free(texture->imageData);
				}
				return false;
			}
			for (short counter=0;counter<chunkheader;counter++) {
				texture->imageData[currentbyte	]=colorbuffer[2];// switch R and B bytes areound while copying
				texture->imageData[currentbyte+1]=colorbuffer[1];
				texture->imageData[currentbyte+2]=colorbuffer[0];
				if (tga.bytesPerPixel==4) {
					texture->imageData[currentbyte+3]=colorbuffer[3];// Copy 4th byte
				}
				currentbyte+=tga.bytesPerPixel;// Increase current byte by the number of bytes per pixel
				currentpixel++;// Increase pixel count by 1
				if (currentpixel>pixelcount) {// Make sure we havent written too many pixels
					LogError("Too many pixels read");
					if (fTGA != NULL) {
						fclose(fTGA);
					}	
					if (colorbuffer != NULL) { // If there is data in colorbuffer
						free(colorbuffer);
					}
					if (texture->imageData != NULL) { // If there is Image data
						free(texture->imageData);
					}
					return false;
				}
			}
		}
	}while (currentpixel<pixelcount);// Loop while there are still pixels left
	fclose(fTGA);
	return true;
}


}//nsTga