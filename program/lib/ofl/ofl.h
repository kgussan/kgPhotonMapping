// oflライブラリ
//概要：ウェーブフロントオブジェクトファイルの読み込み/書き込み/操作を行なうライブラリ。
//		ofl はObject File Loader の略。
//環境：windows, linux問わず。openglなどグラフィックスライブラリにも依存しない。
//注意：#pragma warning(disable:4996)を行っている。
//		sscanfなど古いC関数を使っている警告を抑えている。
//		windowsとlinuxで同じコードを使いたいため、windows関数を使っていない。
//		テクスチャは2次元までである。拡張は容易なので必要に応じて３Dへ
//		このソフトウェアに関する損害は補償されません。ライセンス関係の文言を残せば再配布・改変が可能です。
//----------------------------------------------------------------
//
//LICENCE
//
// Copyright (c) 2014 KOGUCHI Takahiro. kgussan@gmail.com
//
// This software is released under the MIT License.
// A part of this software is based on glm library by Nate Robins under the following license.
//
//Copyright 2010 Nate Robins. All rights reserved.
//Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
//2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
//THIS SOFTWARE IS PROVIDED BY NATE ROBINS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL NATE ROBINS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//The views and conclusions contained in the software and documentation are those of the authors and should not be interpreted as representing official policies, either expressed or implied, of Nate Robins.
//
#ifndef __HEADER_OFL_H__
#define __HEADER_OFL_H__
#include <assert.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>

//windows の時にstdintの設定を書き換える。
#ifdef _MSC_VER
	#include <limits.h>
	#if (_MSC_VER < 1300)
	typedef signed char			int8_t;
	typedef signed short		int16_t;
	typedef signed int			int32_t;
	typedef unsigned char		uint8_t;
	typedef unsigned short		uint16_t;
	typedef unsigned int		uint32_t;
	#else
	typedef signed __int8     int8_t;
	typedef signed __int16    int16_t;
	typedef signed __int32    int32_t;
	typedef unsigned __int8   uint8_t;
	typedef unsigned __int16  uint16_t;
	typedef unsigned __int32  uint32_t;
	#endif
	typedef signed __int64		int64_t;
	typedef unsigned __int64	uint64_t;

	#ifndef INT8_MIN
	#define INT8_MIN     ((int8_t)_I8_MIN)
	#endif
	#ifndef INT8_MAX
	#define INT8_MAX     _I8_MAX
	#endif
	#ifndef INT16_MIN
	#define INT16_MIN    ((int16_t)_I16_MIN)
	#endif
	#ifndef INT16_MAX
	#define INT16_MAX    _I16_MAX
	#endif
	#ifndef INT32_MIN
	#define INT32_MIN    ((int32_t)_I32_MIN)
	#endif
	#ifndef INT32_MAX
	#define INT32_MAX    _I32_MAX
	#endif
	#ifndef INT64_MIN
	#define INT64_MIN    ((int64_t)_I64_MIN)
	#endif
	#ifndef INT64_MAX
	#define INT64_MAX    _I64_MAX
	#endif
	#ifndef UINT8_MAX
	#define UINT8_MAX    _UI8_MAX
	#endif
	#ifndef UINT16_MAX
	#define UINT16_MAX   _UI16_MAX
	#endif
	#ifndef UINT32_MAX
	#define UINT32_MAX   _UI32_MAX
	#endif
	#ifndef UINT64_MAX
	#define UINT64_MAX   _UI64_MAX
	#endif
	#ifdef _WIN64
	#	ifndef INTPTR_MIN
	#	define INTPTR_MIN   INT64_MIN
	#	endif
	#	ifndef INTPTR_MAX
	#	define INTPTR_MAX   INT64_MAX
	#	endif
	#	ifndef UINTPTR_MAX
	#	define UINTPTR_MAX  UINT64_MAX
	#	endif
	#else
	#	ifndef INTPTR_MIN
	#	define INTPTR_MIN   INT32_MIN
	#	endif
	#	ifndef INTPTR_MAX
	#	define INTPTR_MAX   INT32_MAX
	#	endif
	#	ifndef UINTPTR_MAX
	#	define UINTPTR_MAX  UINT32_MAX
	#	endif
	#endif

#else
#	include <stdint.h> 
#endif

#include <string.h>

#if defined(_WIN32)
#	include <sys/timeb.h>
#	include <malloc.h>
#else
#	include <limits.h>
#	include <unistd.h>
#	include <sys/types.h>
#	include <sys/time.h>
#endif
#define APPLY_MEMALIGN 0

namespace nsOfl{
enum{
	OFL_NONE    =(0UL),      //< render with only vertices
	OFL_FLAT    =(1UL<<0UL), //< render with facet normals
	OFL_SMOOTH  =(1UL<<1UL), //< render with vertex normals
	OFL_TEXTURE =(1UL<<2UL), //< render with texture coords
	OFL_COLOR   =(1UL<<3UL), //< render with colors
	OFL_MATERIAL=(1UL<<4UL), //< render with materials
};
enum{
	TRIANGLE_ORDER_CW=0,
	TRIANGLE_ORDER_CCW,
	TRIANGLE_ORDER_END,
};

const float gOflVersion=(const float)1.4f;

#if (1)	//CELL
	static const uint32_t SAFETY_PADDING_SIZE=1024*1024*1;
	static const uint32_t SAFETY_PADDING_A_BIT=64;
	static const uint32_t SAFETY_ALIGNMENT_SIZE=128;
#else //X86
#	if APPLY_MEMALIGN
#		include <malloc.h>
#	endif //APPLY_MEMALIGN
	static const uint32_t SAFETY_PADDING_SIZE=0;
	static const uint32_t SAFETY_PADDING_A_BIT=0;
	static const uint32_t SAFETY_ALIGNMENT_SIZE=1;
#endif

struct oflNode_t{
	uint32_t index;
	bool averaged;
	struct oflNode_t *next;
};

// oflMaterial_t: Structure that defines a material in a model. 
struct oflMaterial_t{
	char *name;				//< name of material
	float diffuse[4];	//< diffuse component
	float ambient[4];		//< ambient component
	float specular[4];	//< specular component
	float emmissive[4];		//< emmissive component
	float shininess;		//< specular exponent 
};

// oflTriangle: Structure that defines a triangle in a model.
struct oflTriangle_t{
	uint32_t vindices[3]; //< array of triangle vertex indices
	uint32_t nindices[3]; //< array of triangle normal indices
	uint32_t tindices[3]; //< array of triangle texcoord indices
	uint32_t findex;      //< index of triangle facet normal
};

// oflGroup_t: Structure that defines a group in a model.
struct oflGroup_t{
	char *name;				//< name of this group
	uint32_t numtriangles;	//< number of triangles in this group
	uint32_t *triangles;	//< array of triangle indices
	uint32_t material;		//< index to material for group
	struct oflGroup_t *next;//< pointer to next group in model
};

// oflModel_t: Structure that defines a model.
struct oflModel_t
{
	char *pathname;				//< path to this model
	char *mtllibname;			//< name of the material library
	uint32_t numvertices;		//< number of vertices in model
	float *vertices;			//< array of vertices  
	uint32_t numnormals;		//< number of normals in model 
	float *normals;				//< array of normals 
	uint32_t numtangents;
	float *tangents;			//< array of normals 
	uint32_t numbinormals;		//< number of normals in model 
	float *binormals;			//< array of normals 
	uint32_t numtexcoords;		//< number of texcoords in model 
	float *texcoords;			//< array of texture coordinates 
	uint32_t numfacetnorms;		//< number of facetnorms in model 
	float *facetnorms;			//< array of facetnorms 
	uint32_t numtriangles;		//< number of triangles in model 
	oflTriangle_t *triangles;	//< array of triangle
	uint32_t nummaterials;		//< number of materials in model 
	oflMaterial_t *materials;	//< array of materials
	uint32_t numgroups;			//< number of groups in model
	oflGroup_t *groups;			//<linked list of groups
	float position[3];			//< position of the model
};

struct oflBinHeader_t{
	char signature[32];			// 32Byte
	uint32_t version;			//  4Byte
	uint32_t numTriangles;		//  4Byte
	uint32_t numVerticies;		//  4Byte
	uint32_t elemNumPos;		//  4Byte,48
	uint32_t elemNumNorm;		//  4Byte
	uint32_t elemNumTangent;	//  4Byte
	uint32_t elemNumBinormal;	//  4Byte
	uint32_t elemNumTex;		//  4Byte,64
	uint32_t sizePos;			//  4Byte
	uint32_t sizeNorm;			//  4Byte
	uint32_t sizeTangent;		//  4Byte
	uint32_t sizeBinormal;		//  4Byte,96
	uint32_t sizeTex;			//  4Byte
	uint32_t offsetToData;		//  4Byte
	uint32_t offsetToPos;		//  4Byte
	uint32_t offsetToNorm;		//  4Byte,128
	uint32_t offsetToTangent;	//  4Byte
	uint32_t offsetToBinormal;	//  4Byte
	uint32_t offsetToTex;		//  4Byte
	//TOTAL : 112Byte
	char padding[144];	//合計でヘッダが256Byteになるようにした。もうちょっとあっても良かったかもしれない。
};


//ユニタイズ
//		モデルのサイズを正規化
//@par	入力
//		oflModel_t 構造体
//@par	出力
//		スケール値
//@par	内容
//	-	原点回りへトランスレート、原点回りの単位立方体へスケーリングをする。
//		スケール値を返す。
//		oflUnitize: "unitize" a model by translating it to the origin and
//		scaling it to fit in a unit cube around the origin. Returns the
//		scalefactor used.
//	-	モデルは適切に初期化されているものとする。
//		model - properly initialized oflModel_t structure 
float oflUnitize(oflModel_t *model);

//幅の長さ取得
//		モデルの書く時限の幅を計算
//@par	入力
//	-	モデル構造体
//	-	3次元のフロート
//@par	出力
//		入力のポインタに対して行なう
//@par	内容
//		モデルの幅、高さ、深さを計算する。
//		Calculates the dimensions ( width, height, depth ) of a model.
//		-	model - initialized oflModel_t structure
//		-	dimensions - array of 3 floats ( float dimensions[ 3 ] )
void oflDimensions(oflModel_t *model,float *dimensions);

//スケーリング
//		モデルのスケールを行う。
//@par 入力
//		モデル構造体
//		スケール値
//@par 出力
//		入力されたモデルに対して行なわれる。
//@par 内容
//		モデルを入力された値でスケールする。
//		Scales a model by a given amount.
//	-	model - properly initialized oflModel_t structure
//	-	scale - scalefactor ( 0.5 = half as large, 2.0 = twice as large )
void oflScale(oflModel_t *model,float scale);

//三角形順序反転
//		三角形の頂点順序の反転。
//@par 入力
//		モデル構造体
//@par 出力
//		入力されたモデルに対して行なわれる。
//@par 内容
//	-	モデル中の全てのポリゴンを反転する。
//	-	標準は反時計回り。
//	-	また、法線の向きも変更する。
//	-	( 原文 )
//		oflReverseWinding: Reverse the polygon winding for all polygons in
//		this model. Default winding is counter-clockwise. Also changes
//		the direction of the normals.
void oflReverseWinding(oflModel_t *model);

//面法線生成
//	-	面ごとの法線を生成する。
//		Generates facet normals for a model
//		( by taking the cross product of the two vectors derived 
//		  from the sides of each triangle ).
//	-	反時計回りの頂点並びを想定 (違うならoflReverseWinding で調整)
//		Assumes a counter-clockwise winding.
//	-	model - initialized oflModel_t structure
void oflFacetNormals(oflModel_t *model);

//頂点法線生成
//		モデルの滑らかな頂点の法線を計算する。
//	-	原文	Generates smooth vertex normals for a model.
//@par	入力
//	-#	モデル構造体
//	-	model - initialized oflModel_t structure
//	-#	角度
//	-	angle - maximum angle ( in degrees ) to smooth across
//@par	出力
//	-	モデル構造体に出力される。
//@par	詳細
//	-	-#	始めにそれぞれの頂点が入っている全ての三角形のリストを作成する。
//			First builds a list of all the triangles each vertex is in.
//		-#	それから全てのそれぞれ頂点が入っている三角形のfacet 法線を
//			平均したリストの中で、それぞれの頂点をループする。
//			Then loops through each vertex in the the list averaging all the facet
//			normals of the triangles each vertex is in.
//		-#	現在の頂点がある三角形のリストの最初の三角形に関連した通常の通常の側面と
//			側面の内積が機能への角度パラメータのコサインより大きいならば、
//			通常のその側面は平均的通常の計算に加えられません、
//			そして、対応する頂点は通常の側面を与えられます。
//			これは硬いエッジを保持する傾向があります。
//			角度はモデルに依存するが、90 度が通常よいスタートになる。
//			Finally, sets the
//			normal index in the triangle for the vertex to the generated smooth normal.
//			If the dot product of a facet normal and the facet normal
//			associated with the first triangle in the list of triangles the
//			current vertex is in is greater than the cosine of the angle
//			parameter to the function, that facet normal is not added into the
//			average normal calculation and the corresponding vertex is given
//			the facet normal. This tends to preserve hard edges.
//			The angle to use depends on the model, but 90 degrees is usually a good start.
void oflVertexNormals(oflModel_t *model,float angle);

//線形テクスチャ座標生成
//		oflLinearTexture
//	-	線形に射影したテクスチャマップによるテクスチャ座標を生成する。
//	-	Generates texture coordinates according to a
//		linear projection of the texture map. It generates these by
//		linearly mapping the vertices onto a square.
//@par 入力
//	-	モデル構造体へのポインタ
//@par 出力
//	-	入力されたモデルポインタへ
//@par 注意
void oflLinearTexture(oflModel_t *model);

//スフィアマップテクスチャ座標生成
//		oflSpheremapTexture
//@par 入力
//	-	モデル構造体への
//@par 出力
//	-	入力されたモデルポインタへ
//@par 内容
//	-	球状に射影したテクスチャマップによるテクスチャ座標を生成する。 \n
//		スフィアマップ、リフレクションマップとして参照される。\n
//		法線をつかって頂点が球の上にマップされ屡であろう場所を計算する。 \n
//		球の上に完璧に平面をマップすることはできないため、歪が極に生じる。 \n
//		X 軸上の極に歪が出る原因がインプリの一部にある。\n
void oflSpheremapTexture(oflModel_t *model);

//OFLモデル削除処理
//		Deletes a oflModel_t structure.
//入力
//	-	モデル構造体へのポインタ
//出力
//	-	入力されたモデルポインタへ
//内容
//	-	モデルデータの削除
void oflDelete(oflModel_t *model);

//オブジェクトデータ読み込み
//		oflReadObj
//入力
//	-	ファイル名称
//出力
//	-	戻り値でモデルポインタ
//内容
//	-	wavefront obj ファイルを読み込む。\n
//		モデルデータを生成し、戻り値でそのポインタを返す。
//		oflDelete で開放される必要がある。
//		-	Reads a model description from a Wavefront .OBJ file.
//			Returns a pointer to the created object which should be free'd with
//			oflDelete( ).
//		-	filename - name of the file containing the Wavefront .OBJ format data. 
oflModel_t* oflReadObj(const char *filename);

// GB (geometry bin)形式読み込み
oflModel_t* oflReadGb(const char *filename);

//立方体生成
oflModel_t* oflCube(void);

//モデルの書き出し
//	oflWriteObj
//@par 入力
//	-#	モデル構造体へのポインタ
//		-	model - initialized oflModel_t structure
//	-#	出力ファイル名称
//		-	filename - name of the file to write the Wavefront .OBJ format data to
//	-#	モード
//		-	mode - a bitwise or of values describing what is written to the file
//			-	OFL_NONE	 - render with only vertices
//			-	OFL_FLAT	 - render with facet normals
//			-	OFL_SMOOTH - render with vertex normals
//			-	OFL_TEXTURE - render with texture coords
//			-	OFL_COLOR	- render with colors ( color material )
//			-	OFL_MATERIAL - render with materials
//			-	OFL_COLOR and OFL_MATERIAL should not both be specified. 
//			-	OFL_FLAT and OFL_SMOOTH should not both be specified. 
//@par 出力
//	-	ファイル出力
//@par 内容
//	-	モデルデータをwavefront obj ファイルへ出力する。
//		-	Writes a model description in Wavefront .OBJ format to a file.
void oflWriteObj(oflModel_t *model,char *filename,uint32_t mode);

//	それぞれεの範囲内にあるベクタを削除する。
//@par 入力
//	-#	モデル構造体へのポインタ
//		-	model:		initialized oflModel_t structure
//	-#	ε( 頂点間の最大の差 )
//		-	epsilon:	maximum difference between vertices
//			( 0.00001 is a good start for a unitized model )
//@par 出力
//	-	入力されたモデルポインタへ
//@par 詳細
//		-eliminate ( weld ) vectors that are within an epsilon of each other.
void oflWeld(oflModel_t *model,float epsilon);

// モデル情報標準出力への表示
//	C / C++ ヘッダ形式で出力する。
//	使用方法：
//	実行プログラムで#include すればよい。
//	複数ファイルを扱う場合、変数名称などはnamespace等で管理すると変数名称の衝突がない。
//@par 入力
//		モデルポインタ
//@par 出力
//		標準出力
void oflExportCHeader(oflModel_t *model, bool exportTangentEnable=false, bool exportBinormalEnable=false);
void oflExportBin(oflModel_t *model,char *fn, bool exportTangentEnable=false, bool exportBinormalEnable=false);
//tangentとbinormalを追加

//ディレクトリ名称の取得
//@param
//		パス名称
//@retval
//		ディレクトリ名称
char* oflDirName(char* path);

//	read a wavefront material library file
//	-	model - properly initialized oflModel_t structure
//	-	name  - name of the material library
//	-	これはオブジェクトファイルローダ内部の関数
int32_t oflReadMtl(oflModel_t *model,char *name);

//	write a wavefront material library file
//	-	model   - properly initialized oflModel_t structure
//	-	modelpath  - pathname of the model being written
//	-	mtllibname - name of the material library to be written
//	-	これはオブジェクトファイルローダ内部の関数
int32_t oflWriteMtl(oflModel_t *model,char *modelpath,char *mtllibname);

//モデル中からグループを検索する。
//@param
//		モデルポインタ、ファイル名称
//@retval
//		グループ
oflGroup_t* oflFindGroup(oflModel_t *model,char *name);

//モデルにグループを追加する。
// Add a group to the model
//@param
//		モデルポインタ、名称
//@retval
//		グループポインタ
oflGroup_t* oflAddGroup(oflModel_t *model,char *name);

} //namespace nsOfl
#endif // HEADER
