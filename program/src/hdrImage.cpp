#include "hdrImage.h"

// doubleのRGB要素を.hdrフォーマット用に変換
HDRPixel get_hdr_pixel(const Color &color)
{
	double d = max(color.x, max(color.y, color.z));
	if (d <= 1e-32){
		return HDRPixel();
	}
	int e;
	double m = frexp(d, &e); // d = m * 2^e
	d = m * 256.0 / d;
	return HDRPixel((unsigned char)(color.x*d),
					(unsigned char)(color.y*d),
					(unsigned char)(color.z*d),
					e + 128);
}

// 書き出し用関数
void save_hdr_file(const std::string &filename, const Color* image, const int width, const int height)
{
	//flipVertical(image, width, height);

	FILE *fp;
	fopen_s(&fp, filename.c_str(), "wb");
	if (fp == NULL) {
		std::cerr << "Error: " << filename << std::endl;
		return;
	}
	// .hdrフォーマットに従ってデータを書きだす
	// ヘッダ
	unsigned char ret = 0x0a;
	fprintf(fp, "#?RADIANCE%c", (unsigned char)ret);
	fprintf(fp, "# Made with 100%% pure HDR Shop%c", ret);
	fprintf(fp, "FORMAT=32-bit_rle_rgbe%c", ret);
	fprintf(fp, "EXPOSURE=1.0000000000000%c%c", ret, ret);

	// 輝度値書き出し
	fprintf(fp, "-Y %d +X %d%c", height, width, ret);
	for(int i=height - 1; i>=0; i--) {
		std::vector<HDRPixel> line;
		for(int j=0; j<width; j++){
			HDRPixel p=get_hdr_pixel(image[j + i*width]);
			line.push_back(p);
		}
		fprintf(fp, "%c%c", 0x02, 0x02);
		fprintf(fp, "%c%c", (width>>8)&0xFF, width&0xFF);
		for(int i=0; i<4; i++) {
			for(int cursor=0; cursor<width;){
				const int cursor_move=min(127, width - cursor);
				fprintf(fp, "%c", cursor_move);
				for(int j=cursor; j<cursor + cursor_move; j++){
					fprintf(fp, "%c", line[j].get(i));
				}
				cursor += cursor_move;
			}
		}
	}

	fclose(fp);
}

void flipVertical(Color* image, const int width, const int height)
{
	Color *copy;
	copy = new Color [width*height];
	for(int i=0; i<height;i++) {
		for(int j=0; j<width; j++){
			copy[i*width+j]=image[i*width+j];
		}
	}
	for(int i=0; i<height;i++) {
		for(int j=0; j<width; j++){
			image[i*width+j]=copy[(height-1-i)*width+j];
		}
	}
}

