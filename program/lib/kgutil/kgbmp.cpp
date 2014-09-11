#include "kgbmp.h"
#include <stdlib.h>
#include <stdio.h>


void DumpBmp24( unsigned char *buf, 
				char		*fn,
				uint32_t	width, 
				uint32_t	height)
{
	FILE *f;
	int filesize = 54 + 3*width*height;  //w is your image width, h is image height, both int

	unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
	unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
	unsigned char bmppad[3] = {0,0,0};

	bmpfileheader[ 2] = (unsigned char)(filesize    );
	bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
	bmpfileheader[ 4] = (unsigned char)(filesize>>16);
	bmpfileheader[ 5] = (unsigned char)(filesize>>24);

	bmpinfoheader[ 4] = (unsigned char)(       width    );
	bmpinfoheader[ 5] = (unsigned char)(       width>> 8);
	bmpinfoheader[ 6] = (unsigned char)(       width>>16);
	bmpinfoheader[ 7] = (unsigned char)(       width>>24);
	bmpinfoheader[ 8] = (unsigned char)(       height    );
	bmpinfoheader[ 9] = (unsigned char)(       height>> 8);
	bmpinfoheader[10] = (unsigned char)(       height>>16);
	bmpinfoheader[11] = (unsigned char)(       height>>24);

	//f = fopen(fn,"wb");
	fopen_s( &f, fn,"wb");
	fwrite(bmpfileheader,1,14,f);
	fwrite(bmpinfoheader,1,40,f);
	for(uint32_t i=0; i<height; i++){
		//fwrite(buf+(width*(height-i-1)*3),3,width,f);
		fwrite(buf+(width*(i)*3),3,width,f);
		fwrite(bmppad,1,(4-(width*3)%4)%4,f);
	}
	fclose(f);
}

