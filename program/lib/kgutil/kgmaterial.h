#ifndef __KGMATERIAL_H__
#define __KGMATERIAL_H__

namespace nsKg{

namespace nsMaterial{

void DisplayMaterial(void);
void DisplayMaterialArrow(void);

	/// order is rgba
	static const float c_fRed[]			= { 1.0f, 0.0f, 0.0f, 1.0f };
	static const float c_fRedHalf[]		= { 0.5f, 0.0f, 0.0f, 1.0f };
	static const float c_fGreen[]		= { 0.0f, 1.0f, 0.0f, 1.0f };
	static const float c_fGreenHalf[]	= { 0.0f, 0.5f, 0.0f, 1.0f };
	static const float c_fBlue[]		= { 0.0f, 0.0f, 1.0f, 1.0f };
	static const float c_fBlueHalf[]	= { 0.0f, 0.0f, 0.5f, 1.0f };

	static const float c_fWhite[]		= { 1.0f, 1.0f, 1.0f, 1.0f };
	static const float c_fBlack[]		= { 0.0f, 0.0f, 0.0f, 1.0f };
	static const float c_fGray[]		= { 0.5f, 0.5f, 0.5f, 1.0f };

	static const float c_fArrowxambient[]   = {0.4, 0.4, 0.4, 1.0};
	static const float c_fArrowXDiffuse[]   = {1.0, 0.0, 0.0, 1.0};
	static const float c_fArrowXSpecular[]  = {1.0, 0.0, 0.0, 1.0};
	static const float c_fArrowXShininess[] = {1.0};
	static const float c_fArrowYAmbient[]   = {0.4, 0.4, 0.4, 1.0};
	static const float c_fArrowYDiffuse[]   = {0.0, 1.0, 0.0, 1.0};
	static const float c_fArrowYSpecular[]  = {0.0, 1.0, 0.0, 1.0};
	static const float c_fArrowYShininess[] = {1.0};
	static const float c_fArrowZAmbient[]   = {0.4, 0.4, 0.4, 1.0};
	static const float c_fArrowZDiffuse[]   = {0.0, 0.0, 1.0, 1.0};
	static const float c_fArrowZSpecular[]  = {0.0, 0.0, 1.0, 1.0};
	static const float c_fArrowZShininess[] = {1.0};

	//1. emerald(エメラルド)
	static const float c_fEmeraldAmbient[]  = {0.0215,  0.1745,   0.0215,  1.0};
	static const float c_fEmeraldDiffuse[]  = {0.07568, 0.61424,  0.07568, 1.0};
	static const float c_fEmeraldSpecular[] = {0.633,   0.727811, 0.633,   1.0};
	static const float c_fEmeraldShininess[]= {76.8};
	//2. jade(翡翠)
	static const float c_fJadeAmbient[]     = {0.135,     0.2225,   0.1575,   1.0};
	static const float c_fJadeDiffuse[]     = {0.54,      0.89,     0.63,     1.0};
	static const float c_fJadeSpecular[]    = {0.316228,  0.316228, 0.316228, 1.0};
	static const float c_fJadeShininess[]   = {12.8};
	//3. obsidian(黒曜石)
	static const float c_fObsidianAmbient[]  = {0.05375, 0.05,    0.06625, 1.0};
	static const float c_fObsidianDiffuse[]  = {0.18275, 0.17,    0.22525, 1.0};
	static const float c_fObsidianSpecular[] = {0.332741,0.328634,0.346435,1.0};
	static const float c_fObsidianShininess[]= {38.4};
	//4. pearl(真珠)
	static const float c_fPearlAmbient[]  = {0.25,     0.20725,  0.20725,  1.0};
	static const float c_fPearlDiffuse[]  = {1,        0.829,    0.829,    1.0};
	static const float c_fPearlSpecular[] = {0.296648, 0.296648, 0.296648, 1.0};
	static const float c_fPearlShininess[]= {10.24 };
	//5. ruby(ルビー)
	static const float c_fRubyAmbient[]  = {0.1745,   0.01175,  0.01175,   1.0};
	static const float c_fRubyDiffuse[]  = {0.61424,  0.04136,  0.04136,   1.0};
	static const float c_fRubySpecular[] = {0.727811, 0.626959, 0.626959,  1.0};
	static const float c_fRubyShininess[]= {76.8 };
	//6. turquoise(トルコ石)
	static const float c_fTurquoiseAmbient[]  ={0.1,     0.18725, 0.1745,  1.0};
	static const float c_fTurquoiseDiffuse[]  ={0.396,   0.74151, 0.69102, 1.0};
	static const float c_fTurquoiseSpecular[] ={0.297254,0.30829, 0.306678,1.0};
	static const float c_fTurquoiseShininess[]={12.8 };
	//7. brass(真鍮)
	static const float c_fBrassAmbient[]  ={0.329412,  0.223529, 0.027451, 1.0};
	static const float c_fBrassDiffuse[]  ={0.780392,  0.568627, 0.113725, 1.0};
	static const float c_fBrassSpecular[] ={0.992157,  0.941176, 0.807843, 1.0};
	static const float c_fBrassShininess[]={27.89743616};
	//8. bronze(青銅)
	static const float c_fBronzeAmbient[]  = {0.2125,   0.1275,   0.054,   1.0};
	static const float c_fBronzeDiffuse[]  = {0.714,    0.4284,   0.18144, 1.0};
	static const float c_fBronzeSpecular[] = {0.393548, 0.271906, 0.166721,1.0};
	static const float c_fBronzeShininess[]= {25.6};
	//9. chrome(クローム)
	static const float c_fChromeAmbient[]  = {0.25,    0.25,     0.25,     1.0};
	static const float c_fChromeDiffuse[]  = {0.4,     0.4,      0.4,      1.0};
	static const float c_fChromeSpecular[] = {0.774597,0.774597, 0.774597, 1.0};
	static const float c_fChromeShininess[]= {76.8};
	//10. copper(銅)
	static const float c_fCopperAmbient[]  = {0.19125,  0.0735,   0.0225,  1.0};
	static const float c_fCopperDiffuse[]  = {0.7038,   0.27048,  0.0828,  1.0};
	static const float c_fCopperSpecular[] = {0.256777, 0.137622, 0.086014,1.0};
	static const float c_fCopperShininess[]= {12.8};
	//11. gold(金)
	static const float c_fGoldAmbient[]  = {0.24725,  0.1995,   0.0745,    1.0};
	static const float c_fGoldDiffuse[]  = {0.75164,  0.60648,  0.22648,   1.0};
	static const float c_fGoldSpecular[] = {0.628281, 0.555802, 0.366065,  1.0};
	static const float c_fGoldShininess[]= {51.2};
	//12. silver(銀)
	static const float c_fSilverAmbient[]  = {0.19225,  0.19225,  0.19225, 1.0};
	static const float c_fSilverDiffuse[]  = {0.50754,  0.50754,  0.50754, 1.0};
	static const float c_fSilverSpecular[] = {0.508273, 0.508273, 0.508273,1.0};
	static const float c_fSilverShininess[]= {51.2};
	//13. プラスチック(黒)
	static const float c_fBlackPlasticAmbient[]  = {0.0,    0.0,    0.0,  1.0};
	static const float c_fBlackPlasticDiffuse[]  = {0.01,   0.01,   0.01, 1.0};
	static const float c_fBlackPlasticSpecular[] = {0.50,   0.50,   0.50, 1.0};
	static const float c_fBlackPlasticShininess[]= {32};
	//14. プラスチック(シアン)
	static const float c_fCyanPlasticAmbient[]  ={0.0,       0.1,       0.06,     1.0};
	static const float c_fCyanPlasticDiffuse[]  ={0.0,       0.50980392,0.50980392,1.0};
	static const float c_fCyanPlasticSpecular[] ={0.50196078,0.50196078,0.50196078,1.0};
	static const float c_fCyanPlasticShininess[]={32};
	//15. プラスチック(緑)
	static const float c_fGreenPlasticAmbient[]  = {0.0,     0.0,   0.0,  1.0};
	static const float c_fGreenPlasticDiffuse[]  = {0.1,     0.35,  0.1,  1.0};
	static const float c_fGreenPlasticSpecular[] = {0.45,    0.55,  0.45, 1.0};
	static const float c_fGreenPlasticShininess[]= {32};
	//16. プラスチック(赤)
	static const float c_fRedPlasticAmbient[]  = {0.0,     0.0,     0.0,  1.0};
	static const float c_fRedPlasticDiffuse[]  = {0.5,     0.0,     0.0,  1.0};
	static const float c_fRedPlasticSpecular[] = {0.7,     0.6,     0.6,  1.0};
	static const float c_fRedPlasticShininess[]= {32};
	//17. プラスチック(白)
	static const float c_fWhitePlasticAmbient[] = {0.0, 0.0, 0.0, 1.0};
	static const float c_fWhitePlasticDiffuse[]  = {0.55,  0.55,    0.55, 1.0};
	static const float c_fWhitePlasticSpecular[] = {0.70,  0.70,    0.70, 1.0};
	static const float c_fWhitePlasticShininess[]= {32};
	//18. プラスチック(黄)
	static const float c_fYellowPlasticAmbient[]  = {0.0,  0.0,     0.0,  1.0};
	static const float c_fYellowPlasticDiffuse[]  = {0.5,  0.5,     0.0,  1.0};
	static const float c_fYellowPlasticSpecular[] = {0.60, 0.60,    0.50, 1.0};
	static const float c_fYellowPlasticShininess[]= {32};
	//19. ゴム(黒)
	static const float c_fBlackRubberAmbient[]  = {0.02,   0.02,    0.02, 1.0};
	static const float c_fBlackRubberDiffuse[]  = {0.01,   0.01,    0.01, 1.0};
	static const float c_fBlackRubberSpecular[] = {0.4,    0.4,     0.4,  1.0};
	static const float c_fBlackRubberShininess[]= {10.0};
	//20. ゴム(シアン)
	static const float c_fCyanRubberAmbient[]  = {0.0,     0.05,    0.05, 1.0};
	static const float c_fCyanRubberDiffuse[]  = {0.4,     0.5,     0.5,  1.0};
	static const float c_fCyanRubberSpecular[] = {0.04,    0.7,     0.7,  1.0};
	static const float c_fCyanRubberShininess[]= {10.0};
	//21. ゴム(緑)
	static const float c_fGreenRubberAmbient[]  = {0.0,    0.05,    0.0,  1.0};
	static const float c_fGreenRubberDiffuse[]  = {0.4,    0.5,     0.4,  1.0};
	static const float c_fGreenRubberSpecular[] = {0.04,   0.7,     0.04, 1.0};
	static const float c_fGreenRubberShininess[]= {10.0};
	//22. ゴム(赤)
	static const float c_fRedRubberAmbient[]  = {0.05,     0.0,     0.0,  1.0};
	static const float c_fRedRubberDiffuse[]  = {0.5,      0.4,     0.4,  1.0};
	static const float c_fRedRubberSpecular[] = {0.7,      0.04,    0.04, 1.0};
	static const float c_fRedRubberShininess[]= {10.0};
	//23. ゴム(白)
	static const float c_fWhiteRubberAmbient[]  = {0.05,   0.05,    0.05, 1.0};
	static const float c_fWhiteRubberDiffuse[]  = {0.5,    0.5,     0.5,  1.0};
	static const float c_fWhiteRubberSpecular[] = {0.7,    0.7,     0.7,  1.0};
	static const float c_fWhiteRubberShininess[]= {10.0};
	//24. ゴム(黄)
	static const float c_fYellowRubberAmbient[]  = {0.05,  0.05,    0.0,  1.0};
	static const float c_fYellowRubberDiffuse[]  = {0.5,   0.5,     0.4,  1.0};
	static const float c_fYellowRubberSpecular[] = {0.7,   0.7,     0.04, 1.0};
	static const float c_fYellowRubberShininess[]= {10.0};
}//nsMaterial
}//nsKg

#endif //__MATERIAL_H__ 
