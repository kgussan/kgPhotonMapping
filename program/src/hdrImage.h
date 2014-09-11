#ifndef __HDR_IMAGE_H__
#define __HDR_IMAGE_H__

#include <string>
#include <iostream>
#include <vector>
#include "kgmacro.h"
#include "kgTime.h"
#include "kgstdint.h"
#include "types.h"

// *** .hdrフォーマットで出力するための関数 ***
struct HDRPixel {
	unsigned char r, g, b, e;
	HDRPixel(const unsigned char r_ = 0, const unsigned char g_ = 0, const unsigned char b_ = 0, const unsigned char e_ = 0) :
		r(r_), g(g_), b(b_), e(e_) {};
	unsigned char get(int idx) {
		switch (idx) {
			case 0: return r;
			case 1: return g;
			case 2: return b;
			case 3: return e;
		}
		return 0;
	}

};

//HDRPixel get_hdr_pixel(const Color &color);

// 書き出し用関数
void save_hdr_file(const std::string &filename, const Color* image, const int width, const int height);

void flipVertical(Color* image, const int width, const int height);

#endif 
