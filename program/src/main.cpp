#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>
#include <queue>

#include <stdlib.h>
#include <stdint.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>

#include "kgmacro.h"
#include "kgTime.h"
#include "kgstdint.h"
#include "kgalgebra.h"
#include "kgbmp.h"

#include "sceneObjectBase.h"
#include "types.h"
#include "kdtree.h"
#include "hdrImage.h"
#include "../lib/ofl/ofl.h" //namespace nsOfl
#include "kgtga.h"
#include "textureMapping.h"
#include "kgDirectxUtils.h"

#define OPENMP_ENABLE ( 1 )
#if OPENMP_ENABLE
#	include <omp.h>
#endif //OPENMP_ENABLE

static const uint32_t kPhotonTracingMode	= 1; //0: cpu, 1: gpu　→cpuはフォトンの位置が特定個所から先は真っ暗になる。
static const uint32_t kPhotonSamplingMode	= 1; //0: cpu, 1: gpu  →cpuはスタックオーバーフロー
static const uint32_t kPhotonSamplingDiv	= 4; //分割サンプリング　1dispatchで50k photonくらい。

static const uint32_t kRenderingRepMax		= 1;
static const uint32_t kRenderingLightMax	= 20;

//フォトンマップ生成時とレンダリングのフォトンサンプリング時と2回レイトレースする。
//上限の打ち切りと、ロシアンルーレットで生死を決める2段階の制限を設けた。
uint32_t kRayTracingMaxBounds	= 0; //上限の打ち切り
uint32_t kRayTracingRRBounds	= 0; //これ以降のデプスでロシアンルーレットを行うので実際にはデプスはもう少し進む。
const float kHitpointOffsetEps	= (const float)1e-2; //1e-3 ~ 1e-4だとまだ鏡面反射のエラーがある。1e-2が良い。
//計測
enum measureTime_t{ 
	kMeasureTimePhotonInit	= 0,
	kMeasureTimePhotonTracing,
	kMeasureTimeGeneratingKdtree,
	kMeasureTimeRendering,
	kMeasureTimePhotonDbCreation,
	kMeasureTimeMax};
uint64_t gMeasuredTime[ kMeasureTimeMax ];

static const uint32_t	kMaxFileLength = 1024;
char					g_fnLog[ kMaxFileLength ];
FILE					*g_fpLog;

nsTga::Texture_t	g_textures[ kTexIdMax ];
frameState_t		g_frameState;

inline float rand01() { return (float)rand()/RAND_MAX; }

//カメラ
Vec gCameraPos = Vec( -20.0f, 110.0f, 90.0f);
Vec gCameraDir = Normalize(Vec( 0.1f, -0.3f, -1.0f ));

//カメラズームの指定：影響箇所は
//	１）photon tiling のためのprojection matrix生成
//	２）projection matrix 生成
//	３）レイトレ画角設定
float kCameraXyZoom=0.25f; // x4ズーム これがレイトレに見栄えがよさそうな雰囲気　
typedef KDTree<Photon> PhotonMap;

//dx11
ID3D11DeviceContext *g_pContextOut					= NULL;
ID3D11Device		*g_pDeviceOut					= NULL;
D3D_FEATURE_LEVEL	g_flOut							= D3D_FEATURE_LEVEL_11_0;
ID3D11ComputeShader *g_pComputeShaderPhotonSampling = NULL;
ID3D11ComputeShader *g_pComputeShaderPhotonTracing	= NULL;
ID3D11Buffer		*g_pCB = NULL; //constant buffer

// *** レンダリングするシーンデータ ****
std::vector< SceneObjectBase * >	g_sceneObject;
uint32_t							g_sceneObjectNum = 0;
std::vector< Triangle * >			g_sceneObjectForGpu;
const int gLightID = 0;//光源のID


Sphere g_spheresLight[] = {//emissionで自己発光の値を指定
	Sphere( 7.5f, Vec( 20.0f, 90.0f, 0.0f ),Color( 12.0f, 12.0f, 12.0f ), Color(), kMatTypeDiffuse),//照明　
	Sphere( 7.5f, Vec( 25.0f, 90.0f, 0.0f ),Color( 12.0f, 12.0f, 12.0f ), Color(), kMatTypeDiffuse),//照明　
	Sphere( 7.5f, Vec( 20.0f, 90.0f, 5.0f ),Color( 12.0f, 12.0f, 12.0f ), Color(), kMatTypeDiffuse),//照明　
	Sphere( 7.5f, Vec( 25.0f, 90.0f, 5.0f ),Color( 12.0f, 12.0f, 12.0f ), Color(), kMatTypeDiffuse),//照明　

	Sphere( 7.5f, Vec( 70.0f, 110.0f, 10.0f ),Color( 12.0f, 12.0f, 12.0f ), Color(), kMatTypeDiffuse),//照明　
	Sphere( 7.5f, Vec( 75.0f, 110.0f, 10.0f ),Color( 12.0f, 12.0f, 12.0f ), Color(), kMatTypeDiffuse),//照明　
	Sphere( 7.5f, Vec( 70.0f, 110.0f, 15.0f ),Color( 12.0f, 12.0f, 12.0f ), Color(), kMatTypeDiffuse),//照明　
	Sphere( 7.5f, Vec( 75.0f, 110.0f, 15.0f ),Color( 12.0f, 12.0f, 12.0f ), Color(), kMatTypeDiffuse),//照明　

	Sphere( 7.5f, Vec( -20.0f, 145.0f, 50.0f ),Color( 12.0f, 1.0f, 1.0f ), Color(), kMatTypeDiffuse),//照明　
	Sphere( 7.5f, Vec( -25.0f, 145.0f, 50.0f ),Color( 12.0f, 1.0f, 1.0f ), Color(), kMatTypeDiffuse),//照明　
	Sphere( 7.5f, Vec( -20.0f, 145.0f, 55.0f ),Color( 12.0f, 1.0f, 1.0f ), Color(), kMatTypeDiffuse),//照明　
	Sphere( 7.5f, Vec( -25.0f, 145.0f, 55.0f ),Color( 12.0f, 1.0f, 1.0f ), Color(), kMatTypeDiffuse),//照明　

	Sphere( 7.5f, Vec( -20.0f, 120.0f, -50.0f ),Color( 1.0f, 12.0f, 1.0f ), Color(), kMatTypeDiffuse),//照明　
	Sphere( 7.5f, Vec( -25.0f, 120.0f, -50.0f ),Color( 1.0f, 12.0f, 1.0f ), Color(), kMatTypeDiffuse),//照明　
	Sphere( 7.5f, Vec( -20.0f, 120.0f, -55.0f ),Color( 1.0f, 12.0f, 1.0f ), Color(), kMatTypeDiffuse),//照明　
	Sphere( 7.5f, Vec( -25.0f, 120.0f, -55.0f ),Color( 1.0f, 12.0f, 1.0f ), Color(), kMatTypeDiffuse),//照明　

	Sphere( 7.5f, Vec( -70.0f, 100.0f, 80.0f ),Color( 1.0f, 1.0f, 12.0f ), Color(), kMatTypeDiffuse),//照明　
	Sphere( 7.5f, Vec( -75.0f, 100.0f, 80.0f ),Color( 1.0f, 1.0f, 12.0f ), Color(), kMatTypeDiffuse),//照明　
	Sphere( 7.5f, Vec( -70.0f, 100.0f, 85.0f ),Color( 1.0f, 1.0f, 12.0f ), Color(), kMatTypeDiffuse),//照明　
	Sphere( 7.5f, Vec( -75.0f, 100.0f, 85.0f ),Color( 1.0f, 1.0f, 12.0f ), Color(), kMatTypeDiffuse),//照明　

	Sphere( 7.5f, Vec( -10.0f, 100.0f, 20.0f ),Color( 12.0f, 1.0f, 1.0f ), Color(), kMatTypeDiffuse),
	Sphere( 7.5f, Vec( 50.0f, 100.0f, 70.0f ),Color( 0.0f, 12.0f, 1.0f ), Color(), kMatTypeDiffuse),
	//Sphere( 16.5f, Vec( 27.0f,16.5f, 47.0f ), Color(), Color(1,1,1)*.99, kMatTypeReflection),// 鏡
	Sphere( 16.5f, Vec( 73.0f, 16.5f, 78.0f ), Color(), Color( 1.0f, 1.0f, 1.0f )*0.99f, kMatTypeRefraction),//ガラス
	Sphere( 8.5f,  Vec( 50.0f, 8.5f ,60.0f ), Color(), Color( 1.0f, 1.0f, 1.0f )*0.999f, kMatTypeDiffuse)//中央のディヒューズ玉。
};


Triangle g_trianglesCornelBox[] = {
	//コーネルボックス（わかりやすい左　青壁、右　赤壁）
	Triangle(Vec(0,0,0),		Vec(100,0,0),	Vec(0,100,0),		
			Color(), 
			Color(0.75,0.75,0.75), 
			kMatTypeReflection,
			//kMatTypeDiffuse,
			kTexIdTestTex,
			Vec(0.0f,0.0f,0.0f),Vec(2.0f,0.0f,0.0f),Vec(0.0f,2.0f,0.0f)),//面後
	Triangle(Vec(100,0,0),		Vec(0,100,0),	Vec(100,100,0),		
			Color(), 
			Color(0.75f,0.75f,0.75f), 
			//kMatTypeDiffuse,
			kMatTypeReflection,
			kTexIdTestTex,
			Vec(2.0f,0.0f,0.0f),Vec(0.0f,2.0f,0.0f),Vec(2.0f,2.0f,0.0f) ),
	Triangle(Vec(0,0,100),	Vec(100,0,100),	Vec(0,100,100),//テクスチャ座標これは適当
			Color(), 
			Color(1.0,0,0), 
			kMatTypeDiffuse,
			kTexIdNone,//面前
			Vec(0.0f,0.0f,0.0f),Vec(0.0f,2.0f,0.0f),Vec(2.0f,0.0f,0.0f)),
	Triangle(Vec(100,0,100),	Vec(0,100,100),	Vec(100,100,100),
			Color(), 
			Color(1.0,0,0), 
			kMatTypeDiffuse,
			kTexIdNone,//テクスチャ座標これは適当
			Vec(2.0f,0.0f,0.0f),Vec(2.0f,2.0f,0.0f),Vec(0.0f,2.0f,0.0f) ),
	Triangle(Vec(0,0,0),	Vec(0,0,100),	Vec(100,0,0),		
			Color(), 
			Color(0.75f,0.75f,0.75f), 
			kMatTypeDiffuse,//面下
			kTexIdTestTex,
			Vec(0.0f,0.0f,0.0f),Vec(2.0f,0.0f,0.0f),Vec(0.0f,2.0f,0.0f)),
	Triangle(Vec(0,0,100),	Vec(100,0,0),	Vec(100,0,100),		
			Color(), 
			Color(0.75f,0.75f,0.75f), 
			kMatTypeDiffuse,
			kTexIdTestTex,
			Vec(2.0f,0.0f,0.0f),Vec(0.0f,2.0f,0.0f),Vec(2.0f,2.0f,0.0f) ),
	Triangle(Vec(0,100,0),	Vec(0,100,100),	Vec(100,100,0),		
			Color(), 
			Color(0.75f,0.75f,0.75f), 
			kMatTypeDiffuse,//面上
			kTexIdTestTex, //h26-08-30
			//kTexIdNone,
			Vec(0.0f,0.0f,0.0f),Vec(2.0f,0.0f,0.0f),Vec(0.0f,2.0f,0.0f)),
	Triangle(Vec(0,100,100),Vec(100,100,0),	Vec(100,100,100),	
			Color(), 
			Color(0.75f,0.75f,0.75f), 
			kMatTypeDiffuse,
			kTexIdTestTex, //h26-08-30
			//kTexIdNone,
			Vec(2.0f,0.0f,0.0f),Vec(0.0f,2.0f,0.0f),Vec(2.0f,2.0f,0.0f) ),

	Triangle(Vec(0,0,0),	Vec(0,100,0),	Vec(0,0,100),		
			Color(), 
			Color(0.75,0.25,0.25), 
			kMatTypeDiffuse,//面右
			kTexIdNone,	
			//kTexIdTeapot,	
			Vec(0.0f,0.0f,0.0f),Vec(2.0f,0.0f,0.0f),Vec(0.0f,2.0f,0.0f)),
	Triangle(Vec(0,100,0),	Vec(0,0,100),	Vec(0,100,100),		
			Color(), 
			Color(0.75,0.25,0.25), 
			kMatTypeDiffuse,
			kTexIdNone,	
			//kTexIdTeapot,	
			Vec(2.0f,0.0f,0.0f),Vec(0.0f,2.0f,0.0f),Vec(2.0f,2.0f,0.0f) ),
	Triangle(Vec(100,0,0),	Vec(100,100,0),	Vec(100,0,100),		
			Color(), 
			Color(0.25,0.25,0.75), 
			kMatTypeDiffuse,//面左
			kTexIdNone,	
			//kTexIdTeapot,	
			Vec(0.0f,0.0f,0.0f),Vec(2.0f,0.0f,0.0f),Vec(0.0f,2.0f,0.0f)),
	Triangle(Vec(100,100,0),Vec(100,0,100),	Vec(100,100,100),	
			Color(), 
			Color(0.25,0.25,0.75), 
			kMatTypeDiffuse,
			kTexIdNone,	
			//kTexIdTeapot,	
			Vec(2.0f,0.0f,0.0f),Vec(0.0f,2.0f,0.0f),Vec(2.0f,2.0f,0.0f) ),
};





//----------------------------------------------------------------
// レンダリング用関数
//----------------------------------------------------------------

// シーンとの交差判定関数
//すべてのオブジェクトをなめることになる。後で三角形を追加
inline bool intersect_scene( Intersection *info, const Ray *ray )
{
	Intersection iquery;
	info->m_distance = (const float)INF;
	for(uint32_t i=0; i<g_sceneObjectNum; i++) {
		bool intersectValid = g_sceneObject[i]->intersect(&iquery,ray);
		//printf("iquery.m_distance=%f \n",iquery.m_distance);
		if( intersectValid 
			&& ( iquery.m_distance > 0.0f )
			&& ( iquery.m_distance < info->m_distance ) ){//一番近いものを適用
			
			info->m_distance=iquery.m_distance;//衝突点との距離更新
			info->m_position=iquery.m_position;
			info->m_normal	=iquery.m_normal;
			info->m_texCoord=iquery.m_texCoord;
			info->setObject( g_sceneObject[i] );
		}
	}
	//printf("info->m_distance=%f \n",info->m_distance);
	return ( info->m_distance < INF );//INFよりも小さい、すなわち衝突判定で更新があった時。
}


// フォトン追跡法によりフォトンマップ構築
//
//　emitPhotonNum	: フォトン投げる数
//	photonMap			: フォトンマップ構造体
//
//#todo ここをCompute shader 化したい。
//
//入力データ　
//	バッファ
//		ジオメトリ
//		フォトン
//	定数
//		フォトン数
//
//出力データ　
//	バッファ
//		フォトンマップデータ(kdtree ?)
//
void CreatePhotonMapCpu(const int	emitPhotonNum, 
						PhotonMap	*photonMap,
						Sphere		*lightSources ) 
{
	printf( "shooting photons %d photons\n", emitPhotonNum );
	uint64_t timeS1 = nsKg::GetMilliSecCount( );//時間計測
	std::vector<Photon> photonDb;

	for(int i=0; i<emitPhotonNum; i++){//フォトン数ごとにループ
		srand(i*i*i);//乱数初期化
		const float r1 = 2.0f*PI*rand01();		//[0.0f,2.0f]
		const float r2 = 1.0f - 2.0f*rand01();	//[1.0f,1.0f]
		const Vec light_pos = lightSources[0].m_position											//光源の位置
							+ ( (lightSources[0].m_radius + EPS)*Vec( sqrtf(1.0f - r2*r2)*cosf(r1),	//x
								sqrtf(1.0f - r2*r2)*sinf(r1),										//y
								r2));																//z

		const Vec normal = Normalize( light_pos - lightSources[0].m_position );
		Vec w, u, v;//法線上のスペース normal tangent binormalのようなものか。
		w = normal;
		if(fabs(w.x)>0.1f){// この法線のxがちょっと大きくなくてはならない条件はなんだろうか？
			u = Normalize( Cross( Vec(0.0f,1.0f,0.0f), w) );
		}else{
			u = Normalize( Cross( Vec(1.0f,0.0f,0.0f), w) );
		}
		v = Cross( w, u );

		//フォトン投射ベクトルの生成
		// コサイン項に比例させる。フォトンが運ぶのが放射輝度ではなく放射束であるため。（エネルギーの計算式から？）
		const float u1 = 2.0f*PI*rand01();//[0,2PI]
		const float u2 = rand01();
		const float u2s = sqrt( u2 );//ランダム
		Vec light_dir = Normalize( ( u*cos( u1 )*u2s + v*sin(u1)*u2s + w*sqrt(1.0f - u2) ) );
		Ray cur_ray( light_pos, light_dir );//現在のレイは光源の位置からランダムな方向へ
		Material lightMat = lightSources[0].m_mat;
		Color cur_flux = lightMat.m_emission * 4.0f * PI * powf( lightSources[0].m_radius, 2 ) * PI / (float)emitPhotonNum;

		// フォトンがシーンを飛ぶ
		bool trace_end = false;
		uint32_t bounds_num=0;
		uint32_t bounds_diffuse_num=0;
		uint32_t bounds_specular_num=0;

		//for(;!trace_end;){//拡散反射面にたどり着くまでフォトンをトレース。
		while(!trace_end){//拡散反射面にたどり着くまでフォトンをトレース。
			//反射回数の打ち切り判定// ちょっと嫌だけどここで書かないとちゃんと打ち切れない。再帰してしまう。
			bounds_num++;
			if(bounds_num>kRayTracingMaxBounds){//指定された回数に達したらやめる
				break;
			}else{//指定回数に達してないなら継続
			}

			// 放射束が0.0なフォトンを追跡してもしょうがないので打ち切る
			if(max(cur_flux.x, max(cur_flux.y, cur_flux.z)) <= 0.0f){
				break;
			}
			Intersection intersection;
			if (!intersect_scene(&intersection, &cur_ray)){
				break;//何も交差しなかったらやめる。
			}
			const Vec hitpoint=intersection.m_position; // 交差位置
			const Vec normal=intersection.m_normal;
			Vec orienting_normal;// 方向を付けた交差位置の法線（物体からのレイの入出を考慮して正負反転）
			if( Dot(normal, cur_ray.dir) < 0.0f ){//交差位置の法線とレイの内積が負なら
				orienting_normal=normal;
			}else{
				orienting_normal=(-1.0f * normal);//orienting_normalはray.dirに負に寄与する方向に設定される。
			}
			Material *mat=intersection.m_object->getMaterial();

			switch (mat->m_matType) {
				case kMatTypeDiffuse: {
					bounds_diffuse_num++;
					// 拡散面なのでフォトンをフォトンマップに格納する
					photonDb.push_back( Photon(	hitpoint, 
												cur_flux, 
												cur_ray.dir,
												bounds_num,
												bounds_diffuse_num,
												bounds_specular_num) );
					// 反射するかどうかをロシアンルーレットで決める
					const float probability = (mat->m_color.x + mat->m_color.y + mat->m_color.z) / 3;
					if(probability > rand01()) { // ロシアンルーレットの結果により反射の場合
						// orienting_normalの方向を基準とした正規直交基底(w, u, v)を作る。
						//この基底に対する半球内で次のレイを飛ばす。
						Vec w, u, v;

						w = orienting_normal;//交差位置の法線（物体からのレイの入出を考慮）
						//w = normal;//交差位置の法線（物体からのレイの入出を考慮）
						if (fabs(w.x) > 0.1f)
							u = Normalize(Cross(Vec(0.0f, 1.0f, 0.0f), w));
						else
							u = Normalize(Cross(Vec(1.0f, 0.0f, 0.0f), w));
						v = Cross(w, u);

						// コサイン項を使った重点的サンプリング
						const float r1=2.0f*PI*rand01();
						const float r2=rand01(), r2s=sqrt(r2);

						Vec reflectionDir=Normalize( (u*cos(r1)*r2s + v*sin(r1)*r2s + w*sqrt(1.0f - r2) ) );
						Vec hitpointWithOffset=hitpoint + reflectionDir*kHitpointOffsetEps;
						//反射ベクトルがめり込まないようにちょっと浮かす
						//Vec dir=Normalize( (u*cos(r1)*r2s + v*sin(r1)*r2s + w*sqrt(1.0f - r2) ) );

						cur_ray =Ray(hitpointWithOffset, reflectionDir);

						Color texColor=GetTexMap(intersection,*mat,g_textures[mat->m_texId],g_fpLog);
						//ここでマテリアル色をcur_fluxに入れているが
						//これをテクスチャ変えたら後段でサンプリングしないで済むか？

						cur_flux=Multiply(cur_flux, texColor) / probability;
						//cur_flux=Multiply(cur_flux, mat->m_color) / probability;

						//h26-08-29 continue;//次のフォトンへ。ここではtrace_endしていないのでもう一度ループ。
						break;
					} else { // 吸収（すなわちここで追跡終了）
						trace_end=true;
						//h26-08-29 continue;//次のフォトンへ
						break;
					}
				} 
				break;

				case kMatTypeReflection: {//鏡面反射 完全鏡面なのでフォトン格納しない	
					bounds_specular_num++;
					if(0){ printf("spec \n"); }
					// 完全鏡面なのでレイの反射方向は決定的。
					Vec reflectionDir=cur_ray.dir - normal*2.0f*Dot(normal,cur_ray.dir);
					Vec hitpointWithOffset=hitpoint + reflectionDir*kHitpointOffsetEps;
						//反射ベクトルがめり込まないようにちょっと浮かす
					cur_ray = Ray(hitpointWithOffset, reflectionDir );
					//cur_ray = Ray(hitpoint, cur_ray.dir - normal*2.0f*Dot(normal, cur_ray.dir));
					Color texColor=GetTexMap(intersection,*mat,g_textures[mat->m_texId],g_fpLog);
					//cur_flux = Multiply(cur_flux, mat->m_color);
					cur_flux = Multiply(cur_flux, texColor);
					//h26-08-29 continue;
					break;

				}
				break;
			
				case kMatTypeRefraction: {//屈折 完全鏡面なのでフォトン格納しない
					bounds_specular_num++;//caustics 用フォトンマップのため
					Vec reflectionDir		= cur_ray.dir - normal*2.0f*Dot(normal,cur_ray.dir);
					Vec hitpointWithOffset	= hitpoint + reflectionDir*kHitpointOffsetEps;
					//反射ベクトルがめり込まないようにちょっと浮かす				
					Ray reflection_ray = Ray(hitpointWithOffset, reflectionDir);
					bool into = Dot(normal, orienting_normal) > 0.0f; // レイがオブジェクトから出るのか、入るのか
					//orienting_normal : 交差位置の法線

					// Snellの法則
					const float nc=REFTRACTION_AIR;
					const float nt=mat->m_refraction; // オブジェクトの屈折率これは本当はマテリアル設定
					const float nnt=into ? nc / nt : nt / nc;
						//レイがオブジェクトから出るときには真空の屈折率/オブジェクトの屈折率。逆の時。
					const float ddn=Dot(cur_ray.dir, orienting_normal);//レイの方向と交差位置の法線の内積
					const float cos2t = 1.0f - nnt*nnt*(1.0f - ddn*ddn);

					//テクスチャマッピング　ここでは3回使う。
					Color texColor = GetTexMap(intersection,*mat,g_textures[mat->m_texId],g_fpLog);
					if(cos2t<0.0) { // 全反射した
						cur_ray =reflection_ray;//レイは反射したレイに
						//cur_flux=Multiply(cur_flux, mat->m_color);
							//放射束は現在の放射束にオブジェクトの色を乗算。ここで色が伝播する。
						cur_flux=Multiply(cur_flux, texColor);
							//放射束は現在の放射束にオブジェクトの色を乗算。ここで色が伝播する。
						break;
					}
					// 屈折していく方向
					Vec tDir;//屈折ベクトル
					if(into){	//入射
						tDir=Normalize( cur_ray.dir*nnt - normal*( 1.0)*(ddn*nnt + sqrt(cos2t)));
					}else{		//出射
						tDir=Normalize( cur_ray.dir*nnt - normal*(-1.0)*(ddn*nnt + sqrt(cos2t)));
					}
					const float probability = (texColor.x + texColor.y + texColor.z) / 3.0f;
					// 屈折と反射のどちらか一方を追跡する。ロシアンルーレットで決定する。
					if (rand01() < probability) {		// 屈折
						Vec hitpointWithRefractionOffset = hitpoint + tDir*kHitpointOffsetEps;	//反射ベクトルがめり込まないようにちょっと浮かす
						cur_ray = Ray( hitpointWithRefractionOffset, tDir );	//次回ループ時に判定に使われるレイ。当たったところから決定した方向へ。
						cur_flux = Multiply( cur_flux, texColor );//現在の色を放射束へ反映
						break;
					} else {							// 反射
						cur_ray = reflection_ray;		//次回ループ時に判定に使われるレイ。屈折率を計算した方向へ
						cur_flux = Multiply(cur_flux, texColor);//現在の色を放射束へ反映
						break;
					}
				}
				break;

				case kMatTypeGlossy:
				{
					break;
				}
				break;
			}
		}
	}

	std::cout << "start summalize photons to unified photon map." << std::endl;
	for(uint32_t j=0; j<photonDb.size(); j++){//増えたフォトンを集計
		photonMap->AddPoint( photonDb[j] );
	}
	if ( 0 ) {
		FILE *fp;
		fopen_s(&fp,"log/output-photonTracingCpuData.txt","w");
		if(fp==NULL){
			printf("error at file open \n");
			exit(-1);
		}
		fprintf( fp, "emit photon num = %d \n", emitPhotonNum );
		fprintf( fp, "result photon num = %d \n", photonDb.size() );
		for(uint32_t j=0; j<photonDb.size(); j++){
			fprintf( fp, "p, %3.2f, %3.2f, %3.2f, ", photonDb[j].position.x, photonDb[j].position.y, photonDb[j].position.z );
			fprintf( fp, "c, %3.2f, %3.2f, %3.2f, ", photonDb[j].power.x, photonDb[j].power.y, photonDb[j].power.z );
			fprintf( fp, "\n" );
		}
		fclose( fp );
	}

	for(uint32_t j=0; j<photonDb.size(); j++){//掃除
		photonDb.pop_back();
	}

	uint64_t timeE1=nsKg::GetMilliSecCount();//時間計測 フォトントレース終了
	//フォトンマップ作成終了。集計ログを出す。
	std::cout << "Done. (" << photonMap->Size() << " photons are stored)" << std::endl;
	fprintf(g_fpLog,"total photons =, %d, photons are stored \n", photonMap->Size() );
	uint64_t timeS2 = nsKg::GetMilliSecCount();//時間計測
	const uint32_t tileW = g_frameState.width / 4;
	const uint32_t tileH = g_frameState.height/ 4;
	uint32_t tileXnum = g_frameState.width / tileW;
	uint32_t tileYnum = g_frameState.height/ tileH;
	float matProj[ 16 ];
	float matProjInv[ 16 ];
	float matView[ 16 ];
	float matViewProj[ 16 ];
	float matViewProjInv[ 16 ];
	float matrixViewport[ 16 ];
	float matrixViewportInv[ 16 ];
	nsKg::BuildViewportMatrix( matrixViewport, g_frameState.width, g_frameState.height );

	//projection matrix generation
	//画面のproj matrixだからこっちでいい。
	nsKg::BuildFrustumMatrix(	matProj,	
								-1.0f * kCameraXyZoom * (float)(g_frameState.width),
								+1.0f * kCameraXyZoom * (float)(g_frameState.width),
								-1.0f * kCameraXyZoom * (float)(g_frameState.height),
								+1.0f * kCameraXyZoom * (float)(g_frameState.height),
								g_frameState.cam_near,
								g_frameState.cam_far );
	nsKg::MatrixInverse( matProjInv, matProj );
	nsKg::MatrixInverse( matrixViewportInv, matrixViewport );
	//view matrix generation
	Vec cam_pos = gCameraPos;
	const float cam_dir_offset = 100.0f;//適当なオフセット値
	nsKg::BuildLookAtMatrix(	matView,
								cam_pos.x,
								cam_pos.y,
								cam_pos.z,
								cam_pos.x + gCameraDir.x * cam_dir_offset,//視点の指定
								cam_pos.y + gCameraDir.y * cam_dir_offset,
								cam_pos.z + gCameraDir.z * cam_dir_offset,//視点方向はカメラ位置から
																		//カメラ方向へ適当に伸ばした場所に。
									0,
									1,
									0 );//upベクタ
	nsKg::MatrixMultiply( matViewProj, matProj, matView );
	nsKg::MatrixInverse( matViewProjInv, matViewProj );

	// near / far calulate by geom
	for( uint32_t y=0; y<tileYnum; y++ ){
		for( uint32_t x=0; x<tileXnum; x++ ){
			uint32_t id = y * tileXnum + x;
			//本当は個々でタイルごとにZバッファを見る。今回はZバッファもフレームバッファもないのでここは省略！
			//後で有効になっているか、テストはやらないといけない。
		}
	}

	//fruscum culling
	//tileW x tileHごとに処理が進む。
	//uint32_t timeCount=nsKg::GetTimeCount();
	uint64_t timeE2=nsKg::GetMilliSecCount();//時間計測

	//KD-treeの作成
	std::cout << "Creating KD-tree..." << std::endl;
	uint64_t timeS3=nsKg::GetMilliSecCount();//時間計測

	photonMap->CreateKDtree();

	if(false){//フォトンマップをログに出す。（KDツリー作成後）
		fprintf(g_fpLog,"log after generating kd tree.\n");
		for(uint32_t i=0;i<photonMap->Size(); i++){
			Photon t=photonMap->GetPointAll(i);
			fprintf(g_fpLog,"id=,%d, pos=, %f, %f, %f, "
							"incident=, %f, %f, %f, "
							"power=, %f, %f, %f, "
							"bound=, %d, bound_diffuse=, %d, bound_specular=, %d, \n",
								i, t.position.x, t.position.y, t.position.z,
								t.incident.x, t.incident.y, t.incident.z,
								t.power.x, t.power.y, t.power.z,
								t.bound, t.bound_diffuse, t.bound_specular);
		}
	}
	uint64_t timeE3=nsKg::GetMilliSecCount();//時間計測
	printf("end \n");
	gMeasuredTime[kMeasureTimePhotonTracing]=timeE1-timeS1;
	gMeasuredTime[kMeasureTimePhotonDbCreation]=timeE2-timeS2;
	gMeasuredTime[kMeasureTimeGeneratingKdtree]=timeE3-timeS3;
}


// GPU でフォトンマップ生成 #trace
void CreatePhotonDbGpu(	uint32_t						*photonResultNum,
						uint32_t						photonEmitNum,
						std::vector<Photon>				*photonsDb,
						uint32_t						triangleNum,
						std::vector<Triangle *>			triangles,
						uint32_t						lightNum,
						Sphere							*lights,		//点光源
						uint32_t						textureNum,
						nsTga::Texture_t				*textures )
{
	printf("emit photons on GPU start \n");
	::std::vector<triangle_t>	bufTriangles;
	::std::vector<texel_t>		bufTex1;
	::std::vector<texel_t>		bufTex2;
	::std::vector<photon_t>		bufPhoton;
	::std::vector<uint32_t>		bufResultParams;


	//入力バッファを生成
	bufTriangles.resize( triangleNum );
	bufTex1.resize( textures[1].width * textures[1].height );
	bufTex2.resize( textures[2].width * textures[2].height );
	bufPhoton.resize( photonEmitNum*10 );	//emit フォトンの10倍を適当にマップ
	bufResultParams.resize( 8 );

	//値を設定する。
	for ( uint32_t i = 0; i < triangleNum; ++i ) {
		bufTriangles[i].x = triangles[i]->m_centerPos.x;
		bufTriangles[i].y = triangles[i]->m_centerPos.y;
		bufTriangles[i].z = triangles[i]->m_centerPos.z;
		
		bufTriangles[i].v0.x = triangles[i]->m_pos0.x;
		bufTriangles[i].v0.y = triangles[i]->m_pos0.y;
		bufTriangles[i].v0.z = triangles[i]->m_pos0.z;
		bufTriangles[i].v0.nx = triangles[i]->m_normal.x;
		bufTriangles[i].v0.ny = triangles[i]->m_normal.y;
		bufTriangles[i].v0.nz = triangles[i]->m_normal.z;
		bufTriangles[i].v0.u = triangles[i]->m_texCoord0.x;
		bufTriangles[i].v0.v = triangles[i]->m_texCoord0.y;

		bufTriangles[i].v1.x = triangles[i]->m_pos1.x;
		bufTriangles[i].v1.y = triangles[i]->m_pos1.y;
		bufTriangles[i].v1.z = triangles[i]->m_pos1.z;
		bufTriangles[i].v1.nx = triangles[i]->m_normal.x;
		bufTriangles[i].v1.ny = triangles[i]->m_normal.y;
		bufTriangles[i].v1.nz = triangles[i]->m_normal.z;
		bufTriangles[i].v1.u = triangles[i]->m_texCoord1.x;
		bufTriangles[i].v1.v = triangles[i]->m_texCoord1.y;

		bufTriangles[i].v2.x = triangles[i]->m_pos2.x;
		bufTriangles[i].v2.y = triangles[i]->m_pos2.y;
		bufTriangles[i].v2.z = triangles[i]->m_pos2.z;
		bufTriangles[i].v2.nx = triangles[i]->m_normal.x;
		bufTriangles[i].v2.ny = triangles[i]->m_normal.y;
		bufTriangles[i].v2.nz = triangles[i]->m_normal.z;
		bufTriangles[i].v2.u = triangles[i]->m_texCoord2.x;
		bufTriangles[i].v2.v = triangles[i]->m_texCoord2.y;

		bufTriangles[i].texId	= triangles[i]->m_mat.m_texId;
		bufTriangles[i].matType	= triangles[i]->m_mat.m_matType;
    }
	uint32_t width  = textures[1].width;
	uint32_t height = textures[1].height;
	for ( uint32_t j = 0; j < height; ++j ) {
		for ( uint32_t i = 0; i < width; ++i ) {
			bufTex1[ j*width + i].r = textures[1].imageData[( j * width + i )*3 + 0 ];
			bufTex1[ j*width + i].g = textures[1].imageData[( j * width + i )*3 + 1 ];
			bufTex1[ j*width + i].b = textures[1].imageData[( j * width + i )*3 + 2 ];
			bufTex1[ j*width + i].a = 0xff;
		}
	}
	width  = textures[2].width;
	height = textures[2].height;
	for ( uint32_t j = 0; j < height; ++j ) {
		for ( uint32_t i = 0; i < width; ++i ) {
			bufTex2[ j*width + i].r = textures[2].imageData[( j * width + i )*3 + 0 ];
			bufTex2[ j*width + i].g = textures[2].imageData[( j * width + i )*3 + 1 ];
			bufTex2[ j*width + i].b = textures[2].imageData[( j * width + i )*3 + 2 ];
			bufTex2[ j*width + i].a = 0xff;
		}
	}	
	
	ID3D11Buffer *pBufTriangles		= NULL;
	ID3D11Buffer *pBufTex1			= NULL;
	ID3D11Buffer *pBufTex2			= NULL;
	ID3D11Buffer *pBufPhoton		= NULL;
	ID3D11Buffer *pBufResultParams	= NULL;
	
	const uint32_t kSafeDataSize = 4;

	CreateStructuredBufferOnGPU(	g_pDeviceOut, 
									sizeof(triangle_t), 
									(UINT)bufTriangles.size(), 
									&bufTriangles[0],	//vectorコンテナの最初のポインタ指定の必要あり 
									&pBufTriangles );

	CreateStructuredBufferOnGPU(	g_pDeviceOut, 
									sizeof(texel_t), 
									(UINT)bufTex1.size(), 
									&bufTex1[0], 
									&pBufTex1 );
    CreateStructuredBufferOnGPU(	g_pDeviceOut, 
									sizeof(texel_t), 
									(UINT)bufTex2.size(), 
									&bufTex2[0], 
									&pBufTex2 );
    CreateStructuredBufferOnGPU(	g_pDeviceOut, 
									sizeof( photon_t ), 
									(UINT)bufPhoton.size() * kSafeDataSize, 
									NULL,				
									&pBufPhoton );
    CreateStructuredBufferOnGPU(	g_pDeviceOut, 
									sizeof( uint32_t ), 
									(UINT)bufResultParams.size(), 
									NULL,				
									&pBufResultParams );
	
	ID3D11ShaderResourceView	*pSrvTriangles		= NULL;
	ID3D11ShaderResourceView	*pSrvTex1			= NULL;
	ID3D11ShaderResourceView	*pSrvTex2			= NULL;
	ID3D11UnorderedAccessView	*pUavPhoton			= NULL;
	ID3D11UnorderedAccessView	*pUavResultParams	= NULL;

	CreateBufferSRV( g_pDeviceOut, pBufTriangles,		&pSrvTriangles );
	CreateBufferSRV( g_pDeviceOut, pBufTex1,			&pSrvTex1 );
	CreateBufferSRV( g_pDeviceOut, pBufTex2,			&pSrvTex2 );
	CreateBufferUAV( g_pDeviceOut, pBufPhoton,		&pUavPhoton );
	CreateBufferUAV( g_pDeviceOut, pBufResultParams,	&pUavResultParams );
	
	//コンピュートシェーダの実行
	ID3D11ShaderResourceView *pSRVs[ ] = { pSrvTriangles, pSrvTex1, pSrvTex2 };

	g_pContextOut->CSSetShader( g_pComputeShaderPhotonTracing, NULL, 0 );
    g_pContextOut->CSSetShaderResources(	0, 
										3, 
										pSRVs );						// shader resource view(読み込みバッファ)
	ID3D11UnorderedAccessView *pUAVs[ ] = { pUavPhoton, pUavResultParams };
    g_pContextOut->CSSetUnorderedAccessViews( 0, 
											2, 
											pUAVs, 
											(UINT *)pUAVs );	// unordered access view(読み書きバッファ)

	ID3D11Buffer	*pConstantBufferCs	= NULL;		//コンスタントバッファ
	struct constBuf_t{
		uint32_t	emitPhotonNum;
		uint32_t	triangleNum;
		float		lightX, lightY, lightZ;
		float		lightR, lightG, lightB;
		float		lightRadius;
		uint32_t	randSeed;
		uint32_t	padding[6];
	};//サイズが8の倍数でなければならない。

	constBuf_t constBuf;
	constBuf.emitPhotonNum	= photonEmitNum;
	constBuf.triangleNum	= g_sceneObjectNum;
	constBuf.lightX			= lights->m_position.x;
	constBuf.lightY			= lights->m_position.y;
	constBuf.lightZ			= lights->m_position.z;
	constBuf.lightR			= lights->m_mat.m_emission.x;
	constBuf.lightG			= lights->m_mat.m_emission.y;
	constBuf.lightB			= lights->m_mat.m_emission.z;
	constBuf.lightRadius	= lights->m_radius;
	constBuf.randSeed		= ( nsKg::GetTimeCount() )%360;
	printf("debug randSeed : %d\n", constBuf.randSeed);
	//printf("debug randSeed from : %d\n", nsKg::GetTimeCount());
	void			*pConstantBufferData   = (void *)&constBuf;//コンスタントバッファ内容
	DWORD			constantBufferNumBytes = sizeof( constBuf_t );
    
    // Create the Const Buffer
    D3D11_BUFFER_DESC constant_buffer_desc;
    memset( &constant_buffer_desc, 0x0, sizeof(constant_buffer_desc) );
    constant_buffer_desc.ByteWidth = sizeof(constBuf_t);
    constant_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    constant_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constant_buffer_desc.CPUAccessFlags = 0;
	constant_buffer_desc.MiscFlags = 0;
	HRESULT hrCb = g_pDeviceOut->CreateBuffer( &constant_buffer_desc, NULL, &g_pCB);
    if( FAILED( hrCb ) ){
		printf( "failed %x \n", hrCb );
		exit(1);
	}

    g_pContextOut->UpdateSubresource( g_pCB, 0, NULL, &constBuf, 0, 0 );
    g_pContextOut->CSSetConstantBuffers( 0, 1, &g_pCB );

    g_pContextOut->Dispatch( (photonEmitNum+63)/64, 1, 1 );		//実行
	g_pContextOut->Flush();

	//解放
	ID3D11UnorderedAccessView* ppUAViewNULL[2] = { NULL, NULL };
    g_pContextOut->CSSetUnorderedAccessViews( 0, 2, ppUAViewNULL, (UINT*)(&ppUAViewNULL) );

    ID3D11ShaderResourceView* ppSRVNULL[3] = { NULL, NULL, NULL };
    g_pContextOut->CSSetShaderResources( 0, 3, ppSRVNULL );
    g_pContextOut->CSSetConstantBuffers( 0, 0, NULL );


	//結果をGPUから読み込み、CPUから読む2つ目のバッファにダンプ
	D3D11_MAPPED_SUBRESOURCE MappedResourcePhoton;
	D3D11_MAPPED_SUBRESOURCE MappedResourceParams;
	ID3D11Buffer *outputBufPhoton		= CreateAndCopyToCpuReadableMem(	g_pDeviceOut, 
																			g_pContextOut, 
																			pBufPhoton );
	ID3D11Buffer *outputBufResultParams	= CreateAndCopyToCpuReadableMem(	g_pDeviceOut, 
																			g_pContextOut, 
																			pBufResultParams );
	g_pContextOut->Map( outputBufPhoton, 0, D3D11_MAP_READ, 0, &MappedResourcePhoton );	
	g_pContextOut->Flush();
	g_pContextOut->Map( outputBufResultParams, 0, D3D11_MAP_READ, 0, &MappedResourceParams );
	g_pContextOut->Flush();
	
	photon_t *resultPhotons;
	uint32_t *resultParams;
	resultPhotons = (photon_t*)MappedResourcePhoton.pData;
	resultParams = (uint32_t *)MappedResourceParams.pData;
	if( resultParams[0]<=0 ){
		printf("error \n");
		exit(1);
	}
	for( uint32_t i=0; i<resultParams[0]; i++ ) {
		//if( resultPhotons[i].g < 5.0f )
		photonsDb->push_back( Photon(	Vec( resultPhotons[i].x, resultPhotons[i].y, resultPhotons[i].z ),
										Color( resultPhotons[i].r, resultPhotons[i].g, resultPhotons[i].b ),
										Vec( 0,0,0 ),// incident is not correct
										0,
										0,
										0) );
	}

	if( false ){//debug data output 
		FILE *fp;
		static uint32_t counter = 0;
		char fn[1024];
		sprintf_s(fn, 1024, "log/output-photonTracingGpuData%d.txt", counter);
		counter++;
		fopen_s(&fp, fn, "w");
		fprintf( fp, "photon emit num = %d \n", photonEmitNum );
		fprintf( fp, "photon result num = %d \n", resultParams[0] );
		for( uint32_t i=0; i<resultParams[0]; i++ ) {
			fprintf( fp, "p, %3.2f, %3.2f, %3.2f, ", 
						resultPhotons[i].x, resultPhotons[i].y, resultPhotons[i].z );
			fprintf( fp, "c, %3.2f, %3.2f, %3.2f, ", 
						resultPhotons[i].r, resultPhotons[i].g, resultPhotons[i].b );
			fprintf( fp, "\n" );
		}

		fprintf( fp, "result params \n" );
		for( int i = 0; i<8; i++ ){	
			fprintf( fp, "%d ", resultParams[i] );
		}
		fprintf( fp, "\n" );

		fclose( fp );
	}
	//結果の格納
	*photonResultNum = resultParams[0];

	//リソースの解放
	g_pContextOut->Unmap( outputBufPhoton, 0 );	
	g_pContextOut->Unmap( outputBufResultParams, 0 );

	SafeRelease( outputBufPhoton );
	SafeRelease( outputBufResultParams );
	
	SafeRelease( pSrvTriangles );
	SafeRelease( pSrvTex1 );
	SafeRelease( pSrvTex2 );
	SafeRelease( pUavPhoton );
	SafeRelease( pUavResultParams );

	SafeRelease( pBufTriangles );
	SafeRelease( pBufTex1 );
	SafeRelease( pBufTex2 );
	SafeRelease( pBufPhoton );
	SafeRelease( pBufResultParams );


	printf("emit photons on GPU end \n");

}



//----------------------------------------------------------------
// ray方向からの放射輝度を求める。フォトン収集を行う。
//	色
//	時間かかる
//これは色を生成するときに呼ばれる。
Color GetRadiance(	const Ray		&ray,					//レイ（ピクセル）
					const uint32_t	depth,					//探すデプス
					PhotonMap		*photonMap,				//フォトンマップDB
					const float		gather_radius,			//サンプルフォトン半径
					const uint32_t	gahter_max_photon_num)	//収集フォトン数
{
	Intersection intersection;
	if( !intersect_scene( &intersection, &ray ) ){
		return BackgroundColor;//交差しなかったときの色。背景色
	}

	const Vec hitpoint	= intersection.m_position; // 交差位置。tは衝突位置。
	const Vec normal	= intersection.m_normal; // 交差位置の法線
	//const Vec normal=obj.normal; // 交差位置の法線
	//const Vec normal=Normalize(hitpoint - obj.centerPos); // 球独自の計算式
	Material *mat = intersection.m_object->getMaterial();

	//もしかしたらいらないかも。
	//const Vec orienting_normal=Dot(normal, ray.dir) < 0.0 ? normal : (-1.0 * normal); 
	// 交差位置の法線（物体からのレイの入出を考慮して正負反転）
	Vec orienting_normal;// 方向を付けた交差位置の法線（物体からのレイの入出を考慮して正負反転）
	if( Dot(normal, ray.dir) < 0.0f ){//交差位置の法線とレイの内積が負なら
		orienting_normal=normal;
	}else{
		orienting_normal=(-1.0f * normal);//orienting_normalはray.dirに負に寄与する方向に設定される。
	}

	// 色の反射率最大のものを得る。ロシアンルーレットで使う。
	// ロシアンルーレットの閾値は任意だが色の反射率等を使うとより良い。
	float russian_roulette_probability = max( mat->m_color.x, max(mat->m_color.y, mat->m_color.z ) );

	// 一定以上レイを追跡したらロシアンルーレットを実行し追跡を打ち切るかどうかを判断する
	if( depth > kRayTracingRRBounds ){
		if(rand01() >= russian_roulette_probability){//ロシアンルーレット
			return mat->m_emission;//一定の反射回数過ぎたら、RRの結果で探索を打ち切る。
		}
	}else if(depth > kRayTracingMaxBounds){
		return mat->m_emission;//最大の反射回数過ぎたら、強制的に打ち切る。
	}else{
		russian_roulette_probability=1.0; // ロシアンルーレット実行しなかった。普通に実行。
	}//ここで最悪打ち切りの上限を決めた方が読みやすいだろう。

	//switch (obj.ref_type) {
	switch (mat->m_matType) {
		case kMatTypeDiffuse: {
			// フォトンマップをつかって放射輝度推定する
			PhotonMap::ResultQueue pqueue;
			// k近傍探索のための問い合わせ構造体生成。
			//gather_radius半径内のフォトンを最大gather_max_photon_num個集めてくる
			PhotonMap::Query query( hitpoint, orienting_normal, gather_radius, gahter_max_photon_num );
			// k近傍探索
			photonMap->SearchKNN(&pqueue, query);
			Color accumulated_flux;
			float max_distance2 = -1.0f;
			// キューからフォトンを取り出しvectorに格納する
			std::vector<PhotonMap::ElementForQueue> photons;
			photons.reserve(pqueue.size());
			for (;!pqueue.empty();) {
				PhotonMap::ElementForQueue p = pqueue.top(); 
				pqueue.pop();
				photons.push_back(p);
				max_distance2 = max(max_distance2, p.distance2);
			}
			// 円錐（コーン）フィルタを使用して放射輝度推定する
			const float max_distance=sqrt(max_distance2);
			const float k=1.1f;
			Color texColor = GetTexMap( intersection, *mat, g_textures[mat->m_texId], g_fpLog);
			for(uint32_t i=0; i<photons.size(); i++){//収集したフォトン分ループ
				const float w=1.0f - (sqrt(photons[i].distance2)/(k*max_distance)); // 円錐フィルタの重み。
				//			w=1.0f - フォトンの距離/(1.1f*最大距離)　：距離が近いと重みが1に近くなる。
				//			遠いと重みが小さくなっていく
				//マテリアルの色はテクスチャがある時には簡単のため無視した。
				//const Color v=Multiply(mat->m_color, photons[i].point->power)/PI; 
				// kMatTypeDiffuse面のBRDF = 1.0 / πであったのでこれをかける
				const Color v=Multiply( texColor, photons[i].point->power)/PI; 
				// 試しにマテリアルのところをテクスチャに変えてみる。
				accumulated_flux=accumulated_flux + w*v;
				//累積したフォトン放射束（色）に重みをつけた今回のフォトンの放射束（色）を加算
				//●直接光と間接光を分ける。
				/*
				if( (photons[i].point->bound_diffuse==1) && (photons[i].point->bound_specular==0) ){ //直接光
					if(1){
						const float w=1.0f - (sqrt(photons[i].distance2)/(k*max_distance)); // 円錐フィルタの重み。
						const Color v=Multiply(texColor, photons[i].point->power)/PI; 
						// kMatTypeDiffuse面のBRDF = 1.0 / πであったのでこれをかける
						accumulated_flux=accumulated_flux + w*v;
					}
				}else{//間接光
					if(0){
						const float w=1.0f - (sqrt(photons[i].distance2)/(k*max_distance)); // 円錐フィルタの重み。
						const Color v=Multiply(texColor, photons[i].point->power)/PI;
						// kMatTypeDiffuse面のBRDF = 1.0 / πであったのでこれをかける
						accumulated_flux=accumulated_flux + w*v;
					}
				}
				*/
			}
			accumulated_flux=accumulated_flux/(1.0f - 2.0f/(3.0f*k)); 
			// 円錐フィルタの係数：k=1.1で放射束が2.5倍くらいになる。
			// Jensenさんの本にある円錐（コーン）フィルタの規定値
			if(max_distance2>0.0f){//最大半径の二乗が0より大きな正の数であれば（ゼロ除算回避）
				return mat->m_emission + accumulated_flux/(PI*max_distance2)/russian_roulette_probability;
					//衝突物体のエミッション値と足した値
			}
		}
		break;

		case kMatTypeReflection:
		if(1){//スペキュラ
			// kMatTypeReflectionとkMatTypeRefractionの場合はパストレーシングとほとんど変わらない。
			// 単純に反射方向や屈折方向の放射輝度(Radiance)をGetRadiance()で求めるだけ。
			// 完全鏡面にヒットした場合、反射方向から放射輝度をもらってくる
			Vec reflectionDir=ray.dir - normal*2.0f*Dot(normal,ray.dir);
			Vec hitpointWithOffset=hitpoint + reflectionDir*kHitpointOffsetEps;
				//反射ベクトルがめり込まないようにちょっと浮かす
			return mat->m_emission + GetRadiance(Ray(hitpointWithOffset, reflectionDir),
											depth+1,
											photonMap,
											gather_radius, gahter_max_photon_num);
		}
		break;

		case kMatTypeRefraction:
		if(1){//屈折
			Vec reflectionDir = ray.dir - normal*2.0f*Dot( normal, ray.dir );
			Vec hitpointWithReflectionOffset = hitpoint + reflectionDir*kHitpointOffsetEps;
				//反射ベクトルがめり込まないように浮かす

			Ray reflection_ray = Ray( hitpointWithReflectionOffset, reflectionDir );//反射するレイ
			//Ray reflection_ray=Ray( hitpoint, ray.dir - normal*2.0f*Dot(normal, ray.dir) );//反射するレイ
			bool into = Dot( normal, orienting_normal ) > 0.0; // レイがオブジェクトから出るのか、入るのか
			// Snellの法則
			//const float nc=1.0f; // 真空の屈折率
			const float nc = REFTRACTION_AIR;
			const float nt = mat->m_refraction;
			
			//const float nnt = into ? nc / nt : nt / nc;
			float nnt;
			if(into){
				nnt=nc/nt;//真空からオブジェクトへ、入射
			}else{
				nnt=nt/nc;//オブジェクトから真空へ、出射
			}
			const float ddn=Dot(ray.dir, orienting_normal);//レイの方向と、方向を付けた交差位置の法線の内積
			const float cos2t=1.0f - nnt*nnt*(1.0f - ddn*ddn);

			if (cos2t < 0.0) { // 全反射した
				// 反射方向から放射輝度をもらってくる
				return mat->m_emission + Multiply(mat->m_color,
										//GetRadiance( Ray(hitpoint, ray.dir - normal * 2.0 * Dot(normal, ray.dir)),
										GetRadiance( Ray( hitpointWithReflectionOffset, reflectionDir ),
													depth+1,
													photonMap,
													gather_radius,
													gahter_max_photon_num)) / russian_roulette_probability;
			}
			// 屈折していく方向(球に特化した式？)
			//Vec tdir = Normalize(ray.dir * nnt - normal * (into ? 1.0 : -1.0) * (ddn * nnt + sqrt(cos2t)));
			Vec tDir;
			if( into==1 ){//入射であれば
				tDir=Normalize(ray.dir*nnt - normal*(+1.0f)*(ddn*nnt + sqrt(cos2t)));
			}else{//出射の場合
				tDir=Normalize(ray.dir*nnt - normal*(-1.0f)*(ddn*nnt + sqrt(cos2t)));
			}
			// SchlickによるFresnelの反射係数の近似
			const float a = nt - nc;
			const float b = nt + nc;
			const float R0 = (a * a) / (b * b);
			const float c = 1.0f - (into ? -ddn : Dot(tDir, normal));
			const float Re = R0 + (1.0f - R0) * pow( (float)c, (float)5.0f );
			const float Tr = 1.0f - Re; // 屈折光の運ぶ光の量
			const float probability = 0.25f + 0.5f * Re;

			// 一定以上レイを追跡したら屈折と反射のどちらか一方を追跡する。（さもないと指数的にレイが増える）
			// ロシアンルーレットで決定する。
			if (depth > 2) {//デプスが2より大きければ、反射か屈折のどちらかにする。
				if (rand01() < probability) { // 反射
					return mat->m_emission +
						Multiply(mat->m_color, GetRadiance(reflection_ray,
														depth+1,
														photonMap,
														gather_radius,
														gahter_max_photon_num) * Re)
						/ probability
						/ russian_roulette_probability;
				} else { // 屈折
					Vec hitpointWithRefractionOffset = hitpoint + tDir*kHitpointOffsetEps;
						//反射ベクトルがめり込まないようにちょっと浮かす
					return mat->m_emission +
						Multiply(mat->m_color, GetRadiance( Ray( hitpointWithRefractionOffset, tDir ),
														depth+1, 
														photonMap, 
														gather_radius, 
														gahter_max_photon_num) 
													* Tr)
						/ (1.0f - probability)
						/ russian_roulette_probability;
				}
			} else {	// デプスが2よりも小さい場合、屈折と反射の両方を追跡。
						//	（2よりも深いデプスは前述のようにR.R.で片方だけ追う。）
				Vec hitpointWithRefractionOffset=hitpoint + tDir*kHitpointOffsetEps;
					//反射ベクトルがめり込まないようにちょっと浮かす
				return mat->m_emission
						+ Multiply(mat->m_color,
									  GetRadiance( reflection_ray, 
												depth+1, 
												photonMap, 
												gather_radius, 
												gahter_max_photon_num )*Re //反射 reflection
									+ GetRadiance(Ray(hitpointWithRefractionOffset, tDir),
												depth+1, 
												photonMap, 
												gather_radius,
												gahter_max_photon_num ) * Tr )//透過 transparent
							/ russian_roulette_probability; //ロシアンルーレットの可能性で除算
			}
		}
		break;
		case kMatTypeGlossy:
			{
			//まだちゃんと書いていない。
			}
		break;
	}

	return Color();//(0,0,0)を返す
}



// GPU でピクセル生成 #sample
void GetRadianceGpu(	std::vector<pixel_t>			*pixels,
						std::vector<Photon>				*photonsDb,
						const uint32_t					triangleNum,
						const std::vector<Triangle *>	triangles,
						const uint32_t					textureNum,
						const nsTga::Texture_t			*textures,
						const float						photonSampleRadius,
						const uint32_t					photonSampleMax,
						const float						colorScale,
						const float						*matViewInv,
						const float						*matViewProjInv )
{
	const uint32_t kOutParamNum = 256;	//シェーダデバッグに使うバッファのための要素数
	printf("sample photons on GPU start \n");
	::std::vector<triangle_t>	bufTriangles;
	::std::vector<texel_t>		bufTex1;
	::std::vector<texel_t>		bufTex2;
	::std::vector<photon_t>		bufPhoton;
	::std::vector<pixel_t>		bufPixel;
	::std::vector<uint32_t>		bufResultIParams;
	::std::vector<float>		bufResultFParams;


	//入力バッファを生成
	bufTriangles.resize( triangleNum );
	bufTex1.resize( textures[1].width * textures[1].height );
	bufTex2.resize( textures[2].width * textures[2].height );
	bufPhoton.resize( photonsDb->size() );
	bufPixel.resize( g_frameState.width * g_frameState.height );
	bufResultIParams.resize( kOutParamNum );
	bufResultFParams.resize( kOutParamNum );

	//値を設定する。
	for ( uint32_t i = 0; i < photonsDb->size(); ++i ) {		
		bufPhoton[i].x = photonsDb->at(i).position.x;
		bufPhoton[i].y = photonsDb->at(i).position.y;
		bufPhoton[i].z = photonsDb->at(i).position.z;
		bufPhoton[i].r = photonsDb->at(i).power.x;
		bufPhoton[i].g = photonsDb->at(i).power.y;
		bufPhoton[i].b = photonsDb->at(i).power.z;
		//bufPhoton[i].incident	= (*photonsDb)[i].incident;
	}
	for ( uint32_t i = 0; i < triangleNum; ++i ) {
		bufTriangles[i].x = triangles[i]->m_centerPos.x;
		bufTriangles[i].y = triangles[i]->m_centerPos.y;
		bufTriangles[i].z = triangles[i]->m_centerPos.z;
		
		bufTriangles[i].v0.x = triangles[i]->m_pos0.x;
		bufTriangles[i].v0.y = triangles[i]->m_pos0.y;
		bufTriangles[i].v0.z = triangles[i]->m_pos0.z;
		bufTriangles[i].v0.nx = triangles[i]->m_normal.x;
		bufTriangles[i].v0.ny = triangles[i]->m_normal.y;
		bufTriangles[i].v0.nz = triangles[i]->m_normal.z;
		bufTriangles[i].v0.u = triangles[i]->m_texCoord0.x;
		bufTriangles[i].v0.v = triangles[i]->m_texCoord0.y;

		bufTriangles[i].v1.x = triangles[i]->m_pos1.x;
		bufTriangles[i].v1.y = triangles[i]->m_pos1.y;
		bufTriangles[i].v1.z = triangles[i]->m_pos1.z;
		bufTriangles[i].v1.nx = triangles[i]->m_normal.x;
		bufTriangles[i].v1.ny = triangles[i]->m_normal.y;
		bufTriangles[i].v1.nz = triangles[i]->m_normal.z;
		bufTriangles[i].v1.u = triangles[i]->m_texCoord1.x;
		bufTriangles[i].v1.v = triangles[i]->m_texCoord1.y;

		bufTriangles[i].v2.x = triangles[i]->m_pos2.x;
		bufTriangles[i].v2.y = triangles[i]->m_pos2.y;
		bufTriangles[i].v2.z = triangles[i]->m_pos2.z;
		bufTriangles[i].v2.nx = triangles[i]->m_normal.x;
		bufTriangles[i].v2.ny = triangles[i]->m_normal.y;
		bufTriangles[i].v2.nz = triangles[i]->m_normal.z;
		bufTriangles[i].v2.u = triangles[i]->m_texCoord2.x;
		bufTriangles[i].v2.v = triangles[i]->m_texCoord2.y;

		bufTriangles[i].texId	= triangles[i]->m_mat.m_texId;
		//if(bufTriangles[i].texId!=1 && bufTriangles[i].texId!=2){
		//	printf("texid = %d\n",bufTriangles[i].texId);
		//}
		bufTriangles[i].matType	= triangles[i]->m_mat.m_matType;
    }
	uint32_t width  = textures[1].width;
	uint32_t height = textures[1].height;
	for ( uint32_t j = 0; j < height; ++j ) {
		for ( uint32_t i = 0; i < width; ++i ) {
			bufTex1[ j*width + i].r = textures[1].imageData[( j * width + i )*3 + 0 ];
			bufTex1[ j*width + i].g = textures[1].imageData[( j * width + i )*3 + 1 ];
			bufTex1[ j*width + i].b = textures[1].imageData[( j * width + i )*3 + 2 ];
			bufTex1[ j*width + i].a = 0xff;
		}
	}
	width  = textures[2].width;
	height = textures[2].height;
	for ( uint32_t j = 0; j < height; ++j ) {
		for ( uint32_t i = 0; i < width; ++i ) {
			bufTex2[ j*width + i].r = textures[2].imageData[( j * width + i )*3 + 0 ];
			bufTex2[ j*width + i].g = textures[2].imageData[( j * width + i )*3 + 1 ];
			bufTex2[ j*width + i].b = textures[2].imageData[( j * width + i )*3 + 2 ];
			bufTex2[ j*width + i].a = 0xff;
		}
	}
	ID3D11Buffer *pBufTriangles		= NULL;
	ID3D11Buffer *pBufTex1			= NULL;
	ID3D11Buffer *pBufTex2			= NULL;
	ID3D11Buffer *pBufPhoton		= NULL;//実データは引数から
	ID3D11Buffer *pBufPixel			= NULL;
	ID3D11Buffer *pBufResultIParams	= NULL;
	ID3D11Buffer *pBufResultFParams	= NULL;
	
	CreateStructuredBufferOnGPU(	g_pDeviceOut, 
									sizeof(triangle_t), 
									(UINT)bufTriangles.size(), 
									&bufTriangles[0],	//vectorコンテナの最初のポインタ指定の必要あり 
									&pBufTriangles );

	CreateStructuredBufferOnGPU(	g_pDeviceOut, 
									sizeof(texel_t), 
									(UINT)bufTex1.size(), 
									&bufTex1[0], 
									&pBufTex1 );
    CreateStructuredBufferOnGPU(	g_pDeviceOut, 
									sizeof(texel_t), 
									(UINT)bufTex2.size(), 
									&bufTex2[0], 
									&pBufTex2 );
    CreateStructuredBufferOnGPU(	g_pDeviceOut, 
									sizeof( photon_t ), 
									(UINT)photonsDb->size(), 
									&bufPhoton[0],
									&pBufPhoton );
    CreateStructuredBufferOnGPU(	g_pDeviceOut, 
									sizeof( pixel_t ), 
									(UINT)bufPixel.size(), 
									NULL,				
									&pBufPixel );
    CreateStructuredBufferOnGPU(	g_pDeviceOut, 
									sizeof( uint32_t ), 
									(UINT)bufResultIParams.size(), 
									NULL,				
									&pBufResultIParams );
    CreateStructuredBufferOnGPU(	g_pDeviceOut, 
									sizeof( float ), 
									(UINT)bufResultFParams.size(), 
									NULL,
									&pBufResultFParams );
	
	ID3D11ShaderResourceView	*pSrvTriangles		= NULL;
	ID3D11ShaderResourceView	*pSrvTex1			= NULL;
	ID3D11ShaderResourceView	*pSrvTex2			= NULL;
	ID3D11ShaderResourceView	*pSrvPhoton			= NULL;
	ID3D11UnorderedAccessView	*pUavPixel			= NULL;
	ID3D11UnorderedAccessView	*pUavResultIParams	= NULL;
	ID3D11UnorderedAccessView	*pUavResultFParams	= NULL;

	CreateBufferSRV( g_pDeviceOut, pBufTriangles,		&pSrvTriangles );
	CreateBufferSRV( g_pDeviceOut, pBufTex1,			&pSrvTex1 );
	CreateBufferSRV( g_pDeviceOut, pBufTex2,			&pSrvTex2 );
	CreateBufferSRV( g_pDeviceOut, pBufPhoton,			&pSrvPhoton );
	CreateBufferUAV( g_pDeviceOut, pBufPixel,			&pUavPixel);
	CreateBufferUAV( g_pDeviceOut, pBufResultIParams,	&pUavResultIParams );
	CreateBufferUAV( g_pDeviceOut, pBufResultFParams,	&pUavResultFParams );
	
	//コンピュートシェーダの実行
	ID3D11ShaderResourceView *pSRVs[ ] = { pSrvTriangles, pSrvTex1, pSrvTex2, pSrvPhoton };

	g_pContextOut->CSSetShader( g_pComputeShaderPhotonSampling, NULL, 0 );
    g_pContextOut->CSSetShaderResources(	0, 
										4, 
										pSRVs );						// shader resource view(読み込みバッファ)
	ID3D11UnorderedAccessView *pUAVs[ ] = { pUavPixel, pUavResultIParams, pUavResultFParams };
    g_pContextOut->CSSetUnorderedAccessViews( 0, 
											3, 
											pUAVs, 
											(UINT *)pUAVs );	// unordered access view(読み書きバッファ)

	ID3D11Buffer	*pConstantBufferCs	= NULL;		//コンスタントバッファ
	struct constBuf_t{
		float		matViewInv[16];
		float		matViewProjInv[16];
		uint32_t	photonNum;
		uint32_t	triangleNum;
		float		photonSampleRadius;
		uint32_t	photonSampleMax;	//4
		float		colorScale;
		uint32_t	screenW;
		uint32_t	screenH;
		float		camNear;			//8
		float		camFar;
		uint32_t	sw;
		uint32_t	samplingDiv;
		uint32_t	padding[5];
	};//サイズが8の倍数でなければならない。

	constBuf_t constBuf;
	constBuf.photonNum			= (uint32_t)photonsDb->size();
	constBuf.triangleNum		= g_sceneObjectNum;
	constBuf.photonSampleRadius	= photonSampleRadius;
	constBuf.photonSampleMax	= photonSampleMax;
	constBuf.colorScale			= colorScale;
	constBuf.screenW			= g_frameState.width;
	constBuf.screenH			= g_frameState.height;
	constBuf.camNear			= g_frameState.cam_near;
	constBuf.camFar				= g_frameState.cam_far;
	constBuf.sw					= 0;
	constBuf.samplingDiv		= kPhotonSamplingDiv;
	//memcpy( constBuf.matViewInv,		matViewInv,		sizeof(float)*16 );
	//memcpy( constBuf.matViewProjInv,	matViewProjInv, sizeof(float)*16 );
	for(uint32_t i=0;i<16; i++){
		constBuf.matViewInv[i]		= matViewInv[i];
		constBuf.matViewProjInv[i]	= matViewProjInv[i];
	}
	void	*pConstantBufferData   = (void *)&constBuf;//コンスタントバッファ内容
	DWORD	constantBufferNumBytes = sizeof( constBuf_t );
    
    // Create the Const Buffer
    D3D11_BUFFER_DESC constant_buffer_desc;
    memset( &constant_buffer_desc, 0x0, sizeof(constant_buffer_desc) );
    constant_buffer_desc.ByteWidth		= sizeof(constBuf_t);
    constant_buffer_desc.Usage			= D3D11_USAGE_DEFAULT;
    constant_buffer_desc.BindFlags		= D3D11_BIND_CONSTANT_BUFFER;
    constant_buffer_desc.CPUAccessFlags = 0;
	constant_buffer_desc.MiscFlags		= 0;
	HRESULT hrCb = g_pDeviceOut->CreateBuffer( &constant_buffer_desc, NULL, &g_pCB);
    if( FAILED( hrCb ) ){
		printf( "failed %x \n", hrCb );
		exit(1);
	}

    g_pContextOut->UpdateSubresource( g_pCB, 0, NULL, &constBuf, 0, 0 );
    g_pContextOut->CSSetConstantBuffers( 0, 1, &g_pCB );

    g_pContextOut->Dispatch( (g_frameState.width+7)/8, (g_frameState.height+7)/8, 1 );		//実行
	g_pContextOut->Flush();

	



	//結果をGPUから読み込み、CPUから読む2つ目のバッファにダンプ
	D3D11_MAPPED_SUBRESOURCE MappedResourcePixels;
	D3D11_MAPPED_SUBRESOURCE MappedResourceIParams;
	D3D11_MAPPED_SUBRESOURCE MappedResourceFParams;
	ID3D11Buffer *outputBufPixels		= CreateAndCopyToCpuReadableMem(	g_pDeviceOut, 
																			g_pContextOut, 
																			pBufPixel );
	ID3D11Buffer *outputBufResultIParams= CreateAndCopyToCpuReadableMem(	g_pDeviceOut, 
																			g_pContextOut, 
																			pBufResultIParams );
	ID3D11Buffer *outputBufResultFParams= CreateAndCopyToCpuReadableMem(	g_pDeviceOut, 
																			g_pContextOut, 
																			pBufResultFParams );
	g_pContextOut->Map( outputBufPixels,			0, D3D11_MAP_READ, 0, &MappedResourcePixels );	
	g_pContextOut->Flush();

	pixel_t		*resultPixels;
	resultPixels	= (pixel_t *	)MappedResourcePixels.pData;

	g_pContextOut->Map( outputBufResultIParams,	0, D3D11_MAP_READ, 0, &MappedResourceIParams );
	g_pContextOut->Flush();

	g_pContextOut->Map( outputBufResultFParams,	0, D3D11_MAP_READ, 0, &MappedResourceFParams );
	g_pContextOut->Flush();
	uint32_t	*resultIParams;
	float		*resultFParams;
	resultIParams	= (uint32_t *	)MappedResourceIParams.pData;
	resultFParams	= (float *		)MappedResourceFParams.pData;


	for( uint32_t y=0; y<g_frameState.height; y++ ) {
		for( uint32_t x=0; x<g_frameState.width/kPhotonSamplingDiv; x++ ) {
			uint32_t idx = y*g_frameState.width+x;
			if(		( pixels->at( idx ).r < 0.01f )
				&&	( pixels->at( idx ).g < 0.01f )
				&&	( pixels->at( idx ).b < 0.01f ) )
			{
				pixels->at(idx).r = resultPixels[ idx ].r;
				pixels->at(idx).g = resultPixels[ idx ].g;
				pixels->at(idx).b = resultPixels[ idx ].b;
			}
		}
	}

	if( false ){
		FILE *fp;
		fopen_s(&fp,"log/output-photonSamplingGpuData.txt","w");
		if(fp==NULL){
			printf("error at file open\n");
			exit(-1);
		}
		fprintf( fp, "result params int \n" );
		for( int i = 0; i<kOutParamNum; i++ ){	
			fprintf( fp, "%d ", resultIParams[i] );
			if( i%16 ==15){
				fprintf( fp, "\n" );
			}
		}
		fprintf( fp, "\n" );
		fprintf( fp, "result params float \n" );
		for( int i = 0; i<kOutParamNum; i++ ){	
			fprintf( fp, "%3.4f ", resultFParams[i] );
			if( i%16 ==15){
				fprintf( fp, "\n" );
			}
		}
		fprintf( fp, "\n" );
		fclose( fp );
	}
	g_pContextOut->Unmap( outputBufPixels,			0 );	
	g_pContextOut->Unmap( outputBufResultIParams,	0 );
	g_pContextOut->Unmap( outputBufResultFParams,	0 );

	//#samplediv #div
	for( uint32_t i=1; i<kPhotonSamplingDiv; i++ ){
		printf("%d sample \n", i);
		constBuf.sw							= i;
		memset( &constant_buffer_desc, 0x0, sizeof(constant_buffer_desc) );
		constant_buffer_desc.ByteWidth		= sizeof(constBuf_t);
		constant_buffer_desc.Usage			= D3D11_USAGE_DEFAULT;
		constant_buffer_desc.BindFlags		= D3D11_BIND_CONSTANT_BUFFER;
		constant_buffer_desc.CPUAccessFlags = 0;
		constant_buffer_desc.MiscFlags		= 0;
		hrCb = g_pDeviceOut->CreateBuffer( &constant_buffer_desc, NULL, &g_pCB);
		if( FAILED( hrCb ) ){
			printf( "failed %x \n", hrCb );
			exit(1);
		}
		g_pContextOut->UpdateSubresource( g_pCB, 0, NULL, &constBuf, 0, 0 );
		g_pContextOut->CSSetConstantBuffers( 0, 1, &g_pCB );
		g_pContextOut->Dispatch( (g_frameState.width+7)/8, (g_frameState.height+7)/8, 1 );		//実行
		g_pContextOut->Flush();


		//結果をGPUから読み込み、CPUから読む2つ目のバッファにダンプ
		MappedResourcePixels;
		MappedResourceIParams;
		MappedResourceFParams;
		outputBufPixels		= CreateAndCopyToCpuReadableMem(	g_pDeviceOut, 
																				g_pContextOut, 
																				pBufPixel );
		outputBufResultIParams= CreateAndCopyToCpuReadableMem(	g_pDeviceOut, 
																				g_pContextOut, 
																				pBufResultIParams );
		outputBufResultFParams= CreateAndCopyToCpuReadableMem(	g_pDeviceOut, 
																				g_pContextOut, 
																				pBufResultFParams );
		g_pContextOut->Map( outputBufPixels,			0, D3D11_MAP_READ, 0, &MappedResourcePixels );	
		g_pContextOut->Flush();

		resultPixels	= (pixel_t *	)MappedResourcePixels.pData;

		g_pContextOut->Map( outputBufResultIParams,	0, D3D11_MAP_READ, 0, &MappedResourceIParams );
		g_pContextOut->Flush();

		g_pContextOut->Map( outputBufResultFParams,	0, D3D11_MAP_READ, 0, &MappedResourceFParams );
		g_pContextOut->Flush();
		resultIParams	= (uint32_t *	)MappedResourceIParams.pData;
		resultFParams	= (float *		)MappedResourceFParams.pData;

		for( uint32_t y=0; y<g_frameState.height; y++ ) {
			for( uint32_t x=g_frameState.width*i/kPhotonSamplingDiv; x<g_frameState.width*( i + 1 )/kPhotonSamplingDiv; x++ ) {
				uint32_t idx = y*g_frameState.width+x;
				if(		( pixels->at( idx ).r < 0.01f )
					&&	( pixels->at( idx ).g < 0.01f )
					&&	( pixels->at( idx ).b < 0.01f ) )
				{
					pixels->at(idx).r = resultPixels[ idx ].r;
					pixels->at(idx).g = resultPixels[ idx ].g;
					pixels->at(idx).b = resultPixels[ idx ].b;
				}
			}
		}
		g_pContextOut->Unmap( outputBufPixels,			0 );	
		g_pContextOut->Unmap( outputBufResultIParams,	0 );
		g_pContextOut->Unmap( outputBufResultFParams,	0 );
	}


	//解放
	ID3D11UnorderedAccessView* ppUAViewNULL[2] = { NULL, NULL };
    g_pContextOut->CSSetUnorderedAccessViews( 0, 2, ppUAViewNULL, (UINT*)(&ppUAViewNULL) );

    ID3D11ShaderResourceView* ppSRVNULL[4] = { NULL, NULL, NULL, NULL };
    g_pContextOut->CSSetShaderResources( 0, 4, ppSRVNULL );
    g_pContextOut->CSSetConstantBuffers( 0, 0, NULL );

	//リソースの解放

	SafeRelease( outputBufPixels );
	SafeRelease( outputBufResultIParams );
	SafeRelease( outputBufResultFParams );
	
	SafeRelease( pSrvTriangles );
	SafeRelease( pSrvTex1 );
	SafeRelease( pSrvTex2 );
	SafeRelease( pSrvPhoton );
	SafeRelease( pUavPixel );
	SafeRelease( pUavResultIParams );
	SafeRelease( pUavResultFParams );

	SafeRelease( pBufTriangles );
	SafeRelease( pBufTex1 );
	SafeRelease( pBufTex2 );
	SafeRelease( pBufPhoton );
	SafeRelease( pBufPixel );
	SafeRelease( pBufResultIParams );
	SafeRelease( pBufResultFParams );

	printf("sample photons on GPU end \n");
}

//
//メイン関数
//
int main(int argc, char **argv)
{

	//directx 
	D3D11CreateDevice(	NULL,
						//D3D_DRIVER_TYPE_REFERENCE,	
						D3D_DRIVER_TYPE_HARDWARE,
						NULL, 
						//D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG, 
						0, 
						NULL, 
						0,
						D3D11_SDK_VERSION, 
						&g_pDeviceOut, 
						&g_flOut, 
						&g_pContextOut );

	//初期化 #init
	uint32_t width; 
	uint32_t height;
	float gather_photon_radius;
	uint32_t gahter_max_photon_num;
	uint32_t photon_num;
	bool gammaCorrectionEnable = true;
	width					= 1920;
	height					= 1080;
	
	//gather_photon_radius	= 32.0f;
	gather_photon_radius	= 100.0f;

	gahter_max_photon_num	= 8;
	photon_num				= 300*1000;//4div
	//photon_num				= 500*1000;//8div
	//photon_num				= 1000*1000;//NG
	//photon_num				= 100*1000;	//2div
	kRayTracingMaxBounds	= 4;//CPU向け
	kRayTracingRRBounds		= 4;//CPU向け

	g_frameState.width		= width;
	g_frameState.height		= height;
	g_frameState.elemNum	= 3;
	g_frameState.cam_near	= 10;
	g_frameState.cam_far	= 1000;
	
	for(uint32_t t=0;t<kMeasureTimeMax;t++){
		gMeasuredTime[t]=0;
	}
	// カメラ位置：現在オブジェクトがxyzが[0,100] にあるのでカメラ中心が(50,50,300)になっている。
	//	右手系座標。ちょっと見下している。
	Ray camera( gCameraPos, gCameraDir );//三角形コーネルボックス・cow用カメラ
	
	// スクリーンのx,y方向のベクトル
	// view空間内での指定を書いていることになるのだろうか。

	//座標系の決定　右手系にする
	//0.5135f は？アスペクト比？→たぶん画面サイズの調整だ。どうして？
	//Vec cx=Vec(width*0.5135f/height, 0.0f, 0.0f);				//カメラx 方向のベクトル
	//Vec cy=Normalize(Cross(cx, camera.dir))*0.5135f;			//カメラy 方向のベクトル
	Vec cx = Vec( (float)width / (float)height, 0.0f, 0.0f);	//カメラx 方向のベクトル
	Vec cy = Normalize( Cross( cx, camera.dir ) );				//カメラy 方向のベクトル
	//Vec cy=Normalize(Cross(camera.dir, cx));	//カメラy 方向のベクトル 
												//元の外積の計算順序を変えてｙがラスタライズと同じ向きになるようにした。
	//fbsize 256x256で(0,-1,0)になる。
	Color *image				= new Color[ width*height ];//フレームバッファ
	Color *imageForOutput		= new Color[ width*height ];//フレームバッファ
	Color *imageForOutputAccum	= new Color[ width*height ];//フレームバッファ
	Color *imageForOutputImm	= new Color[ width*height ];//フレームバッファ
	Color *imageDebugPhoton		= new Color[ width*height ];//フレームバッファ
	Color *imageFbAndPhoton		= new Color[ width*height ];//フレームバッファ

	uint8_t *imageForBmpOutput	= new uint8_t[ width*height*3 ];//フレームバッファ

	//出力情報
	//uint32_t timeCount=nsKg::GetTimeCount();
	uint32_t timeCount=0;
	sprintf_s( g_fnLog, kMaxFileLength, "log/%d.log", timeCount );
	fopen_s( &g_fpLog, g_fnLog, "w");
	if ( g_fpLog == NULL) {
		printf("file open error!!\n");
		exit(EXIT_FAILURE);
	}

	fprintf(g_fpLog, "frame width %d height %d \n", width, height);
	fprintf(g_fpLog, "photon_num %d \n", photon_num);
	fprintf(g_fpLog, "gahter_max_photon_num %d \n", gahter_max_photon_num);
	fprintf(g_fpLog, "gather_photon_radius %f \n", gather_photon_radius);
	fprintf(g_fpLog, "gammaCorrectionEnable %d (powf( image[image_index].x, 1.0f/2.2f )) \n", gammaCorrectionEnable);
	fprintf(g_fpLog, "kRayTracingMaxBounds %d \n", kRayTracingMaxBounds);
	fprintf(g_fpLog, "kRayTracingRRBounds %d \n", kRayTracingRRBounds);
	fprintf(g_fpLog, "OPENMP_ENABLE %d \n", OPENMP_ENABLE);
	fprintf(g_fpLog, "light id %d size %3.2f ", gLightID, g_spheresLight[ gLightID ].m_radius);
	fprintf(g_fpLog, "emission %3.2f %3.2f %3.2f \n",	g_spheresLight[ gLightID ].m_mat.m_emission.x, 
														g_spheresLight[ gLightID ].m_mat.m_emission.y,
														g_spheresLight[ gLightID ].m_mat.m_emission.z );
	fprintf(g_fpLog, "light pos %3.2f %3.2f %3.2f \n",	g_spheresLight[ gLightID ].m_position.x, 
														g_spheresLight[ gLightID ].m_position.y,
														g_spheresLight[ gLightID ].m_position.z );
	fprintf(g_fpLog, "camera pos %3.2f %3.2f %3.2f \n",	gCameraPos.x, 
														gCameraPos.y,
														gCameraPos.z );
	fprintf(g_fpLog, "camera dir %3.2f %3.2f %3.2f \n",	gCameraDir.x, 
														gCameraDir.y,
														gCameraDir.z );
	fprintf(g_fpLog, "frameState %3.2f %3.2f %3.2f %3.2f %3.2f \n",	g_frameState.width,
																	g_frameState.height,
																	g_frameState.elemNum,
																	g_frameState.cam_near,
																	g_frameState.cam_far);
	
	char fnHdr[ kMaxFileLength ];
	char fnHdr2[ kMaxFileLength ];
	char fnHdr3[ kMaxFileLength ];

	//シーン設定
	g_sceneObjectNum = 0;
	//●基本オブジェクト
	uint32_t triangleNum	= sizeof( g_trianglesCornelBox ) / sizeof( Triangle );
	uint32_t sphereNum		= sizeof( g_spheresLight ) / sizeof( Sphere );
	if(0){
		for(uint32_t i=0;i<sphereNum; i++){
			g_sceneObject.push_back( &g_spheresLight[i] );
		}
		g_sceneObjectNum+= sphereNum;
	}
	if( 1 ){

		// for GPU rendering
		float scaleParam = 2.0f;
		float transX = -100.0f;
		float transY = 10.0f;
		float transZ = -100.0f;
		for(uint32_t i=0;i<triangleNum; i++){
			g_trianglesCornelBox[i].m_pos0.x *= scaleParam;
			g_trianglesCornelBox[i].m_pos0.y *= scaleParam;
			g_trianglesCornelBox[i].m_pos0.z *= scaleParam;
			g_trianglesCornelBox[i].m_pos1.x *= scaleParam;
			g_trianglesCornelBox[i].m_pos1.y *= scaleParam;
			g_trianglesCornelBox[i].m_pos1.z *= scaleParam;
			g_trianglesCornelBox[i].m_pos2.x *= scaleParam;
			g_trianglesCornelBox[i].m_pos2.y *= scaleParam;
			g_trianglesCornelBox[i].m_pos2.z *= scaleParam;

			g_trianglesCornelBox[i].m_pos0.x += transX;
			g_trianglesCornelBox[i].m_pos1.x += transX;
			g_trianglesCornelBox[i].m_pos2.x += transX;

			g_trianglesCornelBox[i].m_pos0.y += transY;
			g_trianglesCornelBox[i].m_pos1.y += transY;
			g_trianglesCornelBox[i].m_pos2.y += transY;

			g_trianglesCornelBox[i].m_pos0.z += transZ;
			g_trianglesCornelBox[i].m_pos1.z += transZ;
			g_trianglesCornelBox[i].m_pos2.z += transZ;
			g_sceneObjectForGpu.push_back( &g_trianglesCornelBox[i] );
		}

		for(uint32_t i=0;i<triangleNum; i++){
			g_sceneObject.push_back( &g_trianglesCornelBox[i] );
		}
		g_sceneObjectNum+= triangleNum;

	}

	//●任意モデル( wavefront obj / libofl ) #model #geom
	const uint32_t geomNum=5;
	uint32_t geomTriNum=0;
	nsOfl::oflModel_t *models[geomNum];
	//models[0] = nsOfl::oflReadObj("../asset/cow_model_texture/cow_head_normal.obj");//OK
	models[0] = nsOfl::oflReadObj("../asset/teapot.obj");//OK
	models[1] = nsOfl::oflReadObj("../asset/sphere1024.obj");
	models[2] = nsOfl::oflReadObj("../asset/sphere4K.obj");
	models[3] = nsOfl::oflReadObj("../asset/sphere4K.obj");
	models[4] = nsOfl::oflReadObj("../asset/teapot.obj");//OK
	Triangle *modelTris;
	nsOfl::oflTriangle_t *ofltri;
	nsOfl::oflGroup_t *oflgroup;
	float oflScale[geomNum];
	oflScale[0]=20.0f;
	oflScale[1]=10.0f;
	oflScale[2]=5.0f;
	oflScale[3]=5.0f;
	oflScale[4]=15.0f;
	//oflScale[4]=1000.0f;
	Vec oflTrans[geomNum];
	oflTrans[0]=Vec(	-20.0f,	60.0f,	-20.0f );
	oflTrans[1]=Vec(	20.0f,	60.0f,	-30.0f );
	oflTrans[2]=Vec(	0.0f,	50.0f,	0.0f );
	oflTrans[3]=Vec(	-40.0f,	50.0f,	-10.0f );
	oflTrans[4]=Vec(	40.0f, 70.0f, 0.0f );
	for(uint32_t i=0;i<geomNum;i++){
		//頂点の調整・整形
		nsOfl::oflModel_t *m = models[i];
		nsOfl::oflUnitize(m);
		if(m->numnormals==0){
			oflFacetNormals(m);
			oflVertexNormals( m, 90.0f );//smoothing angle
		}
		//三角形生成
		modelTris= new Triangle [ models[i]->numtriangles ];
		oflgroup=m->groups;
		while(oflgroup){
			geomTriNum += oflgroup->numtriangles;
			for(uint32_t j=0; j<oflgroup->numtriangles; j++){
				ofltri = &m->triangles[(oflgroup->triangles[j])];
				modelTris[j].m_pos0=Vec(m->vertices[3*ofltri->vindices[0]+0],
										m->vertices[3*ofltri->vindices[0]+1],
										m->vertices[3*ofltri->vindices[0]+2] )*oflScale[i]+oflTrans[i];
				modelTris[j].m_pos1=Vec(m->vertices[3*ofltri->vindices[1]+0],
										m->vertices[3*ofltri->vindices[1]+1],
										m->vertices[3*ofltri->vindices[1]+2] )*oflScale[i]+oflTrans[i];
				modelTris[j].m_pos2=Vec(m->vertices[3*ofltri->vindices[2]+0],
										m->vertices[3*ofltri->vindices[2]+1],
										m->vertices[3*ofltri->vindices[2]+2] )*oflScale[i]+oflTrans[i];
				modelTris[j].m_centerPos.x = (modelTris[j].m_pos0.x 
											+ modelTris[j].m_pos1.x
											+ modelTris[j].m_pos2.x)/3.0f;
				modelTris[j].m_centerPos.y = (modelTris[j].m_pos0.y
											+ modelTris[j].m_pos1.y
											+ modelTris[j].m_pos2.y)/3.0f;
				modelTris[j].m_centerPos.z = (modelTris[j].m_pos0.z
											+ modelTris[j].m_pos1.z
											+ modelTris[j].m_pos2.z)/3.0f;
				Vec en1 = modelTris[j].m_pos1 - modelTris[j].m_pos0;
				Vec en2 = modelTris[j].m_pos2 - modelTris[j].m_pos0;
				modelTris[j].m_normal = Normalize( Cross(en1, en2) );//法線の生成
				modelTris[j].m_texCoord0=Vec(	m->texcoords[2*ofltri->tindices[0]+0],
												m->texcoords[2*ofltri->tindices[0]+1],
												0.0f );
				modelTris[j].m_texCoord1=Vec(	m->texcoords[2*ofltri->tindices[1]+0],
												m->texcoords[2*ofltri->tindices[1]+1],
												0.0f );
				modelTris[j].m_texCoord2=Vec(	m->texcoords[2*ofltri->tindices[2]+0],
												m->texcoords[2*ofltri->tindices[2]+1],
												0.0f );
				modelTris[j].m_mat.m_color		= Color(1,1,1)*0.999f;
				modelTris[j].m_mat.m_emission	= Color(0,0,0);
				modelTris[j].m_mat.m_matType	= kMatTypeDiffuse;
				//modelTris[j].m_mat.m_matType	= kMatTypeReflection;
				//modelTris[j].m_mat.m_matType	= kMatTypeRefraction;
				modelTris[j].m_mat.m_refraction	= 1.5f;//
				//modelTris[j].m_mat.m_texId=kTexIdTestTex;
				if(i==0){
					modelTris[j].m_mat.m_texId		= kTexIdTeapot;
					//modelTris[j].m_mat.m_matType	= kMatTypeReflection;
				}else if (i==1){
					modelTris[j].m_mat.m_texId		= kTexIdNone;
				}else if (i==2){
					modelTris[j].m_mat.m_texId		= kTexIdTeapot;
					modelTris[j].m_mat.m_matType	= kMatTypeRefraction;
				}else if (i==3){
					modelTris[j].m_mat.m_texId		= kTexIdTeapot;
					modelTris[j].m_mat.m_matType	= kMatTypeReflection;
				}else if (i==4){
					modelTris[j].m_mat.m_texId		= kTexIdKuma;
					modelTris[j].m_mat.m_matType	= kMatTypeDiffuse;
				}else{
					modelTris[j].m_mat.m_texId		= kTexIdNone;
					modelTris[j].m_mat.m_matType	= kMatTypeReflection;
				}

				g_sceneObject.push_back( &modelTris[j] );
				// for GPU rendering
				g_sceneObjectForGpu.push_back( &modelTris[j] );
			}
			oflgroup = oflgroup->next;
		}
	}

	g_sceneObjectNum += geomTriNum;

	//textures 現在は3コンポーネントだけ受付　アルファも考慮しようと思えばできそうだが、現状は無視。
	if( !nsTga::LoadTGA( &g_textures[ kTexIdKuma ],		"../asset/kuma.tga") ){
		LogError("tex load error \n");
		assert(1);
	}
	if( !nsTga::LoadTGA( &g_textures[ kTexIdTestTex ],	"../asset/orange.tga") ){
		LogError("tex load error \n");
		assert(1);
	}
	if( !nsTga::LoadTGA( &g_textures[ kTexIdTeapot ],	"../asset/teapot.tga") ){
		LogError("tex load error \n");
		assert(1);
	}
	if( !nsTga::LoadTGA( &g_textures[ kTexIdOrange ],	"../asset/orange.tga") ){
		LogError("tex load error \n");
		assert(1);
	}

	if ( 0 ) {// hlslランタイムコンパイル
		//この状態ではwindows SDKではなくDirectX June 2010の古いFXCが走る。このコンパイラではエラーになる。
		//WinSDKのコンパイラでは動作OK。
		ID3DBlob *pErrorBlob	= NULL;
		ID3DBlob *pBlob			= NULL;
		//ディレクトリ基点はproject ファイルパス
		HRESULT hr = D3DX11CompileFromFile(		"photonSampling.hlsl", 
												NULL, 
												NULL, 
												"main", 
												"cs_5_0",
												//D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY,	//有効にならない。
												//D3D10_SHADER_ENABLE_STRICTNESS,
												NULL, 
												NULL, 
												NULL, 
												&pBlob, 
												&pErrorBlob, 
												NULL );
		if( pBlob==NULL ){
			printf( "shader load error.\n" );
			exit( 1 ); 
		}
		//compute shader生成
		g_pDeviceOut->CreateComputeShader(	pBlob->GetBufferPointer(),
											pBlob->GetBufferSize(), 
											NULL, 
											&g_pComputeShaderPhotonSampling);
		SafeRelease( pErrorBlob );
		SafeRelease( pBlob );
	} else {
		//シェーダバイナリから生成
		//事前の手間がかかるが、実行時のコンパイルのペナルティがない。
		HRESULT hr = CreateComputeShader( "photonSampling.fxo", g_pDeviceOut, &g_pComputeShaderPhotonSampling );
		if ( FAILED( hr ) ) {
			exit(1);
		}
	}
	if ( 0 ) {// hlslランタイムコンパイル
		//この状態ではwindows SDKではなくDirectX June 2010の古いFXCが走る。このコンパイラではエラーになる。
		//WinSDKのコンパイラでは動作OK。
		ID3DBlob *pErrorBlob	= NULL;
		ID3DBlob *pBlob			= NULL;
		//ディレクトリ基点はproject ファイルパス
		HRESULT hr = D3DX11CompileFromFile(		"photonTracing.hlsl", 
												NULL, 
												NULL, 
												"main", 
												"cs_5_0",
												NULL, 
												NULL, 
												NULL, 
												&pBlob, 
												&pErrorBlob, 
												NULL );
		if( pBlob==NULL ){
			printf( "shader load error.\n" );
			exit( 1 ); 
		}
		//compute shader生成
		g_pDeviceOut->CreateComputeShader(	pBlob->GetBufferPointer(),
											pBlob->GetBufferSize(), 
											NULL, 
											&g_pComputeShaderPhotonTracing);
		SafeRelease( pErrorBlob );
		SafeRelease( pBlob );
	} else {
		//シェーダバイナリから生成
		// > fxc.exe /T cs_5_0 photonTracing.hlsl /Fo photonTracing.fxo
		//事前の手間がかかるが、実行時のコンパイルのペナルティがない。
		HRESULT hr = CreateComputeShader( "photonTracing.fxo", g_pDeviceOut, &g_pComputeShaderPhotonTracing );
		if ( FAILED( hr ) ) {
			exit(1);
		}
	}

	//#coreloop
	PhotonMap				photonMap;				//kd tree付き　CPU向け
	std::vector<Photon>		photonVectorDbAccum;	//kd tree無し　GPU向け

	sprintf_s( fnHdr3, kMaxFileLength, "log/final.hdr",	timeCount );

	for( uint32_t repId=0; repId<kRenderingRepMax; repId++ ){
		printf("rendering repeat id : %d \n", repId );
		//sprintf(fnHdr2,	"log/%d-%04d-photonOnly.hdr",		timeCount, repId);
		for( uint32_t lightId=0; lightId<kRenderingLightMax; lightId++ ){
		
			sprintf_s( fnHdr, kMaxFileLength, "log/%d-r%04d-l%04d.hdr", timeCount, repId, lightId);
		
			printf("emit light id : %d \n", lightId );
			if( kPhotonTracingMode==0 ){
				//CPU フォトンマップ構築
				CreatePhotonMapCpu(	photon_num, 
									&photonMap, 
									g_spheresLight );//lights
				printf("copy results to photon vector db. %d \n", photonMap.Size() );
				for(uint32_t i = 0; i<photonMap.Size(); i++ ) {
					photonVectorDbAccum.push_back( photonMap.GetPointAll( i ) );
				}
			} else {
				//GPU フォトンマップ構築
				uint32_t photonResultNum = 0;
				//ここでは空間構造を意識しない。そのままGPUで処理
				CreatePhotonDbGpu(	&photonResultNum,
									photon_num, 
									&photonVectorDbAccum,			//vectorで渡す
									g_sceneObjectNum,
									g_sceneObjectForGpu,			// triangle だけ
									1,								//lights num
									&g_spheresLight[ lightId ],		//lights
									kTexIdMax,
									g_textures );

				if( kPhotonSamplingMode==0 ){//CPUでサンプリングするときだけKDツリーを作る
					for(uint32_t i = 0; i<photonVectorDbAccum.size(); i++ ) {
						photonMap.AddPoint( photonVectorDbAccum[i] );
					}
					photonMap.CreateKDtree();
				}
				printf("photon database size = %d \n", photonVectorDbAccum.size() );
				//std::vector<Photon>().swap( photonVectorDbTmp );	//一時バッファのリセット
			}

			if( photonVectorDbAccum.size()==0 ){
				printf("gen photon db error \n");
				exit(1);
			}

			//描画
			uint64_t timeS=nsKg::GetMilliSecCount();//時間計測
			float matProj[16];
			float matView[16];
			float matViewProj[16];
			float matrixViewport[16];
			nsKg::BuildFrustumMatrix(	matProj,
										-1.0f * kCameraXyZoom * (float)(g_frameState.width),
										+1.0f * kCameraXyZoom * (float)(g_frameState.width),
										-1.0f * kCameraXyZoom * (float)(g_frameState.height),
										+1.0f * kCameraXyZoom * (float)(g_frameState.height),
										g_frameState.cam_near,
										g_frameState.cam_far );
			//view matrix generation
			Vec cam_pos = gCameraPos;
			const float cam_dir_offset = 100.0f;//適当なオフセット値 いくつでも先を見れば合うはずだから適当でよい。
			nsKg::BuildLookAtMatrix(	matView,
										cam_pos.x,
										cam_pos.y,
										cam_pos.z,
										cam_pos.x + gCameraDir.x * cam_dir_offset,//視点の指定
										cam_pos.y + gCameraDir.y * cam_dir_offset,
										cam_pos.z + gCameraDir.z * cam_dir_offset,
											//視点方向はカメラ位置からカメラ方向へ適当に伸ばした場所に。
										0,
										1,
										0 );//upベクタ
			nsKg::MatrixMultiply( matViewProj, matView, matProj );
			nsKg::BuildViewportMatrix( matrixViewport, g_frameState.width, g_frameState.height );
			if( false ){//デバッグ　ビューポートのデバッグ フォトンの位置をフォトン色でプロットしてHDRで書き出し。
				float coordsOrg[4];
				float coordsView[4]				= {0,0,0,0};
				float coordsClipping[4]			= {0,0,0,0};
				float coordsNormalizedDevice[4] = {0,0,0,0};
				float coordsScreen[4]			= {0,0,0,0};
				nsKg::MatrixShow( "matView",		matView,		g_fpLog );
				nsKg::MatrixShow( "matProj",		matProj,		g_fpLog );
				nsKg::MatrixShow( "matViewProj",	matViewProj,	g_fpLog );
				nsKg::MatrixShow( "matrixViewport",	matrixViewport,	g_fpLog );
				for( uint32_t p=0; p<photonMap.Size(); p++ ){
					Photon photon = photonMap.GetPointAll( p );
					coordsOrg[0] = photon.position.x;
					coordsOrg[1] = photon.position.y;
					coordsOrg[2] = photon.position.z;
					coordsOrg[3] = 1.0f;
					nsKg::MatrixMultiplyVector4( coordsView,		matView, coordsOrg );	
					nsKg::MatrixMultiplyVector4( coordsClipping,	matProj, coordsView );
					coordsNormalizedDevice[0] = coordsClipping[0] / coordsClipping[3];
					coordsNormalizedDevice[1] = coordsClipping[1] / coordsClipping[3];
					coordsNormalizedDevice[2] = coordsClipping[2] / coordsClipping[3];
					coordsNormalizedDevice[3] = coordsClipping[3] / coordsClipping[3];
					nsKg::MatrixMultiplyVector4( coordsScreen, matrixViewport, coordsNormalizedDevice );
					if( false ){
						fprintf(g_fpLog,"coordsView:			%3.2f %3.2f %3.2f \n", 
											coordsView[0], 
											coordsView[1], 
											coordsView[2]);
						fprintf(g_fpLog,"coordsClipping:		%3.2f %3.2f %3.2f %3.2f \n",
											coordsClipping[0], 
											coordsClipping[1], 
											coordsClipping[2] , 
											coordsClipping[3] );
						fprintf(g_fpLog,"coordsNormalizedDevice:%3.2f %3.2f %3.2f \n", 
											coordsNormalizedDevice[0], 
											coordsNormalizedDevice[1], 
											coordsNormalizedDevice[2] );
						fprintf(g_fpLog,"coordsScreen:			%3.2f %3.2f %3.2f \n", 
											coordsScreen[0], 
											coordsScreen[1], 
											coordsScreen[2]);
					}
					uint32_t x = (uint32_t)( (float)( ( coordsScreen[0] + 0.5f ) * (float)g_frameState.width  ) );
					uint32_t y = (uint32_t)( (float)( ( coordsScreen[1] + 0.5f ) * (float)g_frameState.height ) );
					if( x>=0 
						&& x<g_frameState.width 
						&& y>=0 
						&& y<g_frameState.height ){
						imageDebugPhoton[ g_frameState.width * y + x ] = imageDebugPhoton[ g_frameState.width * y + x ]
																		+ photon.power;
					}
				}
				flipVertical( imageDebugPhoton, height, width );
				save_hdr_file( std::string( fnHdr2 ), imageDebugPhoton, width, height );
			}

			if( kPhotonSamplingMode==0 ){
				#if ( OPENMP_ENABLE==1 )
				#pragma omp parallel for schedule( dynamic, 1 )
				#endif
				for( int y=0; y<(int)height; y++ ) { //OpenMPでマルチスレッド化して使うためには変数はintの必要がある。
					std::cerr << "Rendering " << (100.0 * y / (height - 1)) << "%" << std::endl;
					srand( y*y*y );//乱数初期化 マルチスレッドのためにそれぞれにシードを変えている。
					for( int x=0; x<(int)width; x++ ){//OpenMPでマルチスレッド化して使うためには変数はintの必要がある。
						int image_index = y * width + x;
						image[ image_index ] = Color();
						{//1サンプル/pixel
							Vec vecStart, vecDir;
							const float pixelCenterOffset=0.5f;
							{	//viewProjMatから作成する。
								//[注意] レイの原点はすべて視点でスクリーンのピクセルごとに対して方向を計算している。
								//[注意] 方角を計算する時にFOVを決定する値を考える必要はない。
								float rayStart_in_projSpace[4]={//[-1.0f,1.0f]-> x value
													(2.0f*( (pixelCenterOffset+(float)(x) )/(float)(g_frameState.width ) )-1.0f)
														/(float)(g_frameState.width ),
													(2.0f*( (pixelCenterOffset+(float)(y) )/(float)(g_frameState.height) )-1.0f)
														/(float)(g_frameState.height),
													0,1};
								float eyePos_in_viewSpace[4] = {0,0,0,1};
								float rayStart[4];
								float rayToward[4];
								float rayDir[4];
								float matViewInv[16];
								float matViewProjInv[16];
								nsKg::MatrixInverse(matViewInv,matView);
								nsKg::MatrixInverse(matViewProjInv,matViewProj);
								nsKg::MatrixMultiplyVector4( rayStart, matViewInv, eyePos_in_viewSpace );		//レイの開始点
								nsKg::MatrixMultiplyVector4( rayToward, matViewProjInv, rayStart_in_projSpace );//レイの方角
								rayToward[0]/=rayToward[3];//perspective補正
								rayToward[1]/=rayToward[3];
								rayToward[2]/=rayToward[3];
								rayToward[3]/=rayToward[3];
								rayDir[0]=rayToward[0]-rayStart[0];//方向計算
								rayDir[1]=rayToward[1]-rayStart[1];
								rayDir[2]=rayToward[2]-rayStart[2];
								rayDir[3]=rayToward[3]-rayStart[3];
								nsKg::Vector4Normalize(rayDir);
								vecDir	 = Vec( rayDir[0], rayDir[1], rayDir[2] );
								vecStart = Vec( rayStart[0], rayStart[1], rayStart[2] ) 
											+ vecDir * g_frameState.cam_near; //nearクリップ
							}
							image[ image_index ] = image[ image_index ]
												+ GetRadiance( Ray(vecStart,vecDir),//ray
														0,//raytracing depth 基点だから0
														&photonMap,//database
														gather_photon_radius,
														gahter_max_photon_num ) * 4.0f;
							imageFbAndPhoton[ image_index ] = image[ image_index ];//デバッグデータのコピー
						}
					}
				}
			}else if( kPhotonSamplingMode==1 ){
				//gpu rendering ピクセル単位
				std::vector<pixel_t> pixels;
				const float scale = 4.0f;
				float matViewInv[16];
				float matViewProjInv[16];
				nsKg::MatrixInverse(matViewInv,matView);
				nsKg::MatrixInverse(matViewProjInv,matViewProj);
				//数回の反射をしているが一回分にしてマルチパスにすれば何回でもできる。
				
				//あらかじめサイズを規定
				pixels.resize( g_frameState.width*g_frameState.height );

				GetRadianceGpu( 
					&pixels,				//vectorで増やしてもらっている
					&photonVectorDbAccum,	//database
					g_sceneObjectNum,
					g_sceneObjectForGpu,	// triangle だけが入っている。陰関数による衝突判定をしてないため、球は入っていない。
					kTexIdMax,
					g_textures,
					gather_photon_radius,
					gahter_max_photon_num,
					scale, 
					matViewInv,
					matViewProjInv );

				for(uint32_t h=0; h<g_frameState.height; h++){
					for(uint32_t w=0; w<g_frameState.width; w++){
						uint32_t id = h*g_frameState.width + w;
						image[ id ].x	= pixels[ id ].r;
						image[ id ].y	= pixels[ id ].g;
						image[ id ].z	= pixels[ id ].b;
					}
				}
				std::vector< pixel_t >().swap( pixels );//要素の削除
			}

			uint64_t timeE = nsKg::GetMilliSecCount( );//時間計測
			gMeasuredTime[ kMeasureTimeRendering ] = timeE - timeS;//時間計測

			uint64_t totalTime = 0; 
			for( uint32_t i=0; i<kMeasureTimeMax; i++ ){
				totalTime += gMeasuredTime[ i ];
			}
			//debug 出力
			fprintf(g_fpLog, "================================\n");
			fprintf(g_fpLog, "total                      time, %d, ms \n",
								totalTime );
			fprintf(g_fpLog, "\n");
			fprintf(g_fpLog, "photon tracing             time, %d, ms \n",
								gMeasuredTime[ kMeasureTimePhotonTracing ] );
			fprintf(g_fpLog, "generating KDtree          time, %d, ms \n",
								gMeasuredTime[ kMeasureTimeGeneratingKdtree ] );
			fprintf(g_fpLog, "tiled photonDB creation    time, %d, ms \n",
								gMeasuredTime[ kMeasureTimePhotonDbCreation ] );
			fprintf(g_fpLog, "photon sampling and render time, %d, ms \n",
								gMeasuredTime[ kMeasureTimeRendering ] );

			//掃除
			std::vector<Photon>().swap( photonVectorDbAccum );



			//色の調節
			{
				for(uint32_t h=0; h<g_frameState.height; h++){
					for(uint32_t w=0; w<g_frameState.width; w++){
						uint32_t id = h*g_frameState.width + w;
						imageForOutput[ id ].x = image[ id ].x;
						imageForOutput[ id ].y = image[ id ].y;
						imageForOutput[ id ].z = image[ id ].z;
					}
				}
			}

			//gamma変換
			if( gammaCorrectionEnable ){
				LogInfo("gamma correction start \n");
				for(uint32_t y=0; y<height; y++) {
					for(uint32_t x=0; x<width; x++){
						int image_index = y * width + x;
						imageForOutputAccum[image_index] = Color(	imageForOutputAccum[image_index].x + imageForOutput[ image_index ].x, 
																	imageForOutputAccum[image_index].y + imageForOutput[ image_index ].y, 
																	imageForOutputAccum[image_index].z + imageForOutput[ image_index ].z );
						imageForOutput[image_index]		= Color(powf( imageForOutput[ image_index ].x, 1.0f/2.2f ), 
																powf( imageForOutput[ image_index ].y, 1.0f/2.2f ), 
																powf( imageForOutput[ image_index ].z, 1.0f/2.2f ) );
					}
				}
				LogInfo("gamma correction end \n");
			}

			// .hdrフォーマットで出力 #fbout #image
			for(uint32_t y=0; y<height; y++) {
				for(uint32_t x=0; x<width; x++){
					int image_index = y * width + x;
					imageForBmpOutput[ image_index *3 + 0 ] = (uint8_t)( imageForOutput[ image_index ].x * 255.0f );
					imageForBmpOutput[ image_index *3 + 1 ] = (uint8_t)( imageForOutput[ image_index ].y * 255.0f );
					imageForBmpOutput[ image_index *3 + 2 ] = (uint8_t)( imageForOutput[ image_index ].z * 255.0f );
				}
			}
			char bfn[1024];
			sprintf_s( bfn, kMaxFileLength, "log/imm-%d-%d.bmp", repId, lightId );
			DumpBmp24( (unsigned char *)imageForBmpOutput, bfn, width, height );

			{
				static uint32_t hCounter = 0;
				hCounter++;
				const uint32_t hCounterTiming = 10;
				if( hCounter%hCounterTiming == (hCounterTiming-1) ){
					//gamma変換
					if( gammaCorrectionEnable ){
						LogInfo("gamma correction start \n");
						for(uint32_t y=0; y<height; y++) {
							for(uint32_t x=0; x<width; x++){
								int image_index = y * width + x;
								// tmp
								imageForOutputImm[image_index] = Color(	powf( imageForOutputAccum[ image_index ].x, 1.0f/2.2f ), 
																		powf( imageForOutputAccum[ image_index ].y, 1.0f/2.2f ), 
																		powf( imageForOutputAccum[ image_index ].z, 1.0f/2.2f ) );
							}
						}
						LogInfo("gamma correction end \n");
					}
					save_hdr_file( std::string( fnHdr ), imageForOutputImm, width, height );
				}
			}
		}//light loop

	}// rep loop

	//gamma変換
	if( gammaCorrectionEnable ){
		LogInfo("gamma correction start \n");
		for(uint32_t y=0; y<height; y++) {
			for(uint32_t x=0; x<width; x++){
				int image_index = y * width + x;
				imageForOutputAccum[image_index] = Color(	powf( imageForOutputAccum[ image_index ].x/(float)kRenderingRepMax, 1.0f/2.2f ), 
															powf( imageForOutputAccum[ image_index ].y/(float)kRenderingRepMax, 1.0f/2.2f ), 
															powf( imageForOutputAccum[ image_index ].z/(float)kRenderingRepMax, 1.0f/2.2f ) );
			}
		}
		LogInfo("gamma correction end \n");
	}
	save_hdr_file( std::string( fnHdr3 ), imageForOutputAccum, width, height );

	//終了処理
	fclose(g_fpLog);

	SafeRelease( g_pContextOut );
	SafeRelease( g_pDeviceOut );
	SafeRelease( g_pComputeShaderPhotonSampling );
	SafeRelease( g_pComputeShaderPhotonTracing );

	exit(0);
}
