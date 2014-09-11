// フォトンDB
struct photon_t {
	float	x, y, z;
	float	r, g, b;
	uint	padding[2];
	//float	incident[3];
};
struct vertex_t {
	float x, y, z;
	float nx, ny, nz;
	float u, v;
};
struct triangle_t {
	float		x, y, z;	// center
	vertex_t	v0, v1, v2;
	uint		matType;		// 0:diff, 1:spec_refl, 2:spec_refr, 3:glossy
	uint		texId;		
	//uint		pagging[7];		
};
struct hitpoint_t {
	//float		distance;
	float3		position;
	float3		normal;
	float2		texcoord;
	uint		matType;		// 0:diff, 1:spec_refl, 2:spec_refr, 3:glossy
	uint		texId;		
	uint		padding[6];	
};
static const uint	kMatTypeDiffuse		= 0;
static const uint	kMatTypeReflection	= 1;
static const uint	kMatTypeRefraction	= 2;
static const uint	kMatTypeGlossy		= 3;
static const float	kHitpointOffsetEps	= 1e-2; //1e-3 ~ 1e-4だとまだ鏡面反射のエラーがある。1e-2が良い。
static const uint	kMemorySpaceTimes	= 4;

//グローバル扱い
cbuffer constBuf_t : register( b0 )
{
	uint	g_emitPhotonNum;
	uint	g_triangleNum;
	float	g_lightX, g_lightY, g_lightZ;
	float	g_lightR, g_lightG, g_lightB;//8
	float	g_lightRadius;
	uint	g_randSeed;
	uint	g_padding[6];
};

struct ray_t {
	float3	org;
	float3	dir;
	uint	padding[2];
};


StructuredBuffer<triangle_t>	bufTriangles	: register( t0 );
StructuredBuffer<uint>			bufTex1			: register( t1 );
StructuredBuffer<uint>			bufTex2			: register( t2 );

RWStructuredBuffer<photon_t>	OutPhotonDb : register( u0 );
RWStructuredBuffer<uint>		OutParams : register( u1 );

static const float	kPi = 3.1415926535898f;
static const float	kEps = 1e-6;
static const float	kInf = 1e20;
static const uint	kMaxBounds = 4;
static hitpoint_t g_hitpoint[ kMaxBounds ];	//グローバルの変数。/Gecオプション入れないと怒られる。どう書けば正しい？→static 


#define USE_NATHAN_REED_RANDOM (1)
#if (USE_NATHAN_REED_RANDOM)

//http://www.reedbeta.com/blog/2013/01/12/quick-and-easy-gpu-random-numbers-in-d3d11/
static uint rng_state;
 uint rand_lcg()
{
    // LCG values from Numerical Recipes
    rng_state = 1664525 * rng_state + 1013904223;
    return rng_state;
}
uint rand_xorshift()
{
    // Xorshift algorithm from George Marsaglia's paper
    rng_state ^= (rng_state << 13);
    rng_state ^= (rng_state >> 17);
    rng_state ^= (rng_state << 5);
    return rng_state;
}

float rand01(void)
{
    // Generate a few numbers...
    //uint r0 = rand_xorshift();
    //uint r1 = rand_xorshift();
    // Do some stuff with them...
 
    // Generate a random float in [0, 1)...
    float f0 = float(rand_xorshift()) * (1.0 / 4294967296.0);
	return f0;
}

#else
static float g_randSeedPix = 0.0f;
static float g_randPrev = 0.0f;
float rand01( void ){
	const float bias = kPi*1.0f;
	const float step = kPi*1.0f;
	float rad = g_randSeedPix*1.414210356f*bias + g_randSeed*2.23620679f*bias;
	float random = frac( sin( rad ) * 1.7320508f*bias + g_randPrev );
	g_randPrev = random;
	g_randSeedPix += random*step;
	//tmp += rad / 2;
	return random;
}

#endif 

//再帰をかけないため、自分でループを展開
//グローバルの三角形バッファから判定
int IntersectScene( const ray_t ray, uint rtDepth )
{
	float distance			= kInf;
	float distanceNearest	= kInf;
	uint  nearestId			= 0;
	uint  intersectOccured	= 0;
	uint  triangleNum		= g_triangleNum;
	[ loop ]for( uint i=0; i<triangleNum; i++ ) {
		//ここで三角形との距離を計算
		const float eps=0.000001f;
		//Moller-Trumbore 法 (高速な交差判定)
		float u, v, t; 
		float3 edge1 = float3( bufTriangles[i].v1.x, bufTriangles[i].v1.y, bufTriangles[i].v1.z )
					 - float3( bufTriangles[i].v0.x, bufTriangles[i].v0.y, bufTriangles[i].v0.z );
		float3 edge2 = float3( bufTriangles[i].v2.x, bufTriangles[i].v2.y, bufTriangles[i].v2.z )
					 - float3( bufTriangles[i].v0.x, bufTriangles[i].v0.y, bufTriangles[i].v0.z );
		float3 pvec = cross( ray.dir, edge2);
		float det = dot( edge1, pvec );
		//non-culling algorithm
		if( det > -eps && det < eps){//衝突していない。
			continue;
		}
		float invDet = 1.0f / det;
		float3 tvec = ray.org - float3( bufTriangles[i].v0.x, bufTriangles[i].v0.y, bufTriangles[i].v0.z );
		u = dot( tvec, pvec ) * invDet;
		if( u<0.0f || u>1.0f ){//衝突していない。
			continue;
		}
		float3 qvec = cross( tvec, edge1 );
		v = dot( ray.dir, qvec ) * invDet;
		if(v<0.0f || u+v>1.0f ){//衝突していない。
			continue;
		}
		t = dot( edge2, qvec ) * invDet;
		//パラメタ設定
		intersectOccured = 1;
		distance	= t;
		if( intersectOccured 
			&& ( distance > 0.0f )
			&& ( distance < distanceNearest ) ){//一番近いものを適用
			
			distanceNearest = distance;//衝突点との距離更新
			nearestId = i;
			intersectOccured = 1;
			g_hitpoint[ rtDepth ].position	= ray.org + t * ray.dir;	//交差した点
			g_hitpoint[ rtDepth ].normal	= float3( bufTriangles[i].v0.nx, bufTriangles[i].v0.ny, bufTriangles[i].v0.nz );
											//三角形の面法線そのまま渡せばよい。
			g_hitpoint[ rtDepth ].texcoord	= float2(	  bufTriangles[i].v0.u * (1-u-v) 
											+ bufTriangles[i].v1.u * u 
											+ bufTriangles[i].v2.u * v,
											  bufTriangles[i].v0.v * (1-u-v) 
											+ bufTriangles[i].v1.v * u 
											+ bufTriangles[i].v2.v * v );//重心座標から
			g_hitpoint[ rtDepth ].matType	= bufTriangles[i].matType;
			g_hitpoint[ rtDepth ].texId		= bufTriangles[i].texId;
		}
	}
	int ret;
	if( intersectOccured==1 ){
		ret = nearestId;
	}else{
		ret = -1;
	}
	return ret;
}

uint CalcTexAddr( float2 st, uint texW, uint texH )
{
	uint ret = 0;
	uint2	uv;
	if(0){//clamp
		uv.x = (int)( st.x * (float)texW + 0.5f );
		if ( uv.x < 0 ) {
			uv.x = 0;
		} else if ( uv.x > (texW - 1) ) {
			uv.x = texW - 1;
		}

		uv.y = (int)( st.y * (float)texH + 0.5f );
		if ( uv.y < 0 ) {
			uv.y = 0;
		} else if ( uv.y > (texH - 1) ) {
			uv.y = texH - 1;
		}
	}else{//repeat
		uv.x = (int)( st.x * (float)texW + 0.5f );
		uv.x %= texW;

		uv.y = (int)( st.y * (float)texH + 0.5f );
		uv.y %= texH;
	}
	ret = uv.y * texW + uv.x;
	return ret;
}

float3 GetTexMap( float2 st, uint texid )
{
	float3	ret = float3(0.0f, 0.0f, 0.0f);
	uint2	uv;
	uint	texW;
	uint	texH;
	uint	texAddr;

	if( texid==1 ){
		//test tex
		texAddr = CalcTexAddr( st, 128, 128 );
		ret.r = (float)( ( bufTex1[ texAddr ] >>  0 ) & 0xff );
		ret.g = (float)( ( bufTex1[ texAddr ] >>  8 ) & 0xff );
		ret.b = (float)( ( bufTex1[ texAddr ] >> 16 ) & 0xff );
	}else if( texid==2 ){
		//cow tex
		texAddr = CalcTexAddr( st, 512, 512 );
		ret.r = (float)( ( bufTex2[ texAddr ] >> 0 ) & 0xff );
		ret.g = (float)( ( bufTex2[ texAddr ] >> 8 ) & 0xff );
		ret.b = (float)( ( bufTex2[ texAddr ] >>16 ) & 0xff );
	}else{
		ret.r = 0x80;
		ret.g = 0x80;
		ret.b = 0x80;
	}
	ret.r /= 255.0f;
	ret.g /= 255.0f;
	ret.b /= 255.0f;

	return ret;
}

//1スレッド、1フォトン
[numthreads(64,1,1)]
void main( uint3 id : SV_DispatchThreadID )
{
	uint pid = id.x + g_randSeed;
#if (USE_NATHAN_REED_RANDOM)
	rng_state = id.x;
#else
	g_randSeedPix = (float)pid;
#endif
	if( g_emitPhotonNum > pid ) {//実行対象のフォトンであれば
		const float r1 = 2.0f*kPi*rand01( );		//[0.0f,2.0f]
		const float r2 = 1.0f - 2.0f*rand01( );	//[1.0f,1.0f]
		const float3 lightCenterPos = float3( g_lightX, g_lightY, g_lightZ );
		const float3 lightPos = lightCenterPos							//光源の位置
								+ ( ( g_lightRadius + kEps )
								* float3(	sqrt(1.0f - r2*r2)*cos(r1),	//x
											sqrt(1.0 - r2*r2)*sin(r1),	//y
											r2));						//z

		const float3 normal = normalize( lightPos - lightCenterPos);
		float3 w,u,v;							//法線上のスペース normal tangent binormalのようなものか。
		w = normal;
		if( abs( w.x ) > 0.1f ){
			u = normalize( cross( float3(0.0f,1.0f,0.0f),w ) );
		}else{
			u = normalize( cross( float3(1.0f,0.0f,0.0f),w ) );
		}
		v = cross(w, u);

		//フォトン投射ベクトルの生成
		// コサイン項に比例させる。フォトンが運ぶのが放射輝度ではなく放射束であるため。（エネルギーの計算式から？）
		float	u1			= 2.0f*kPi*rand01( );//[0,2PI]
		float	u2			= rand01(  );
		float	u2s			= sqrt( u2 );//ランダム
		float3	lightDir	= normalize( ( u*cos( u1 )*u2s + v*sin( u1 )*u2s + w*sqrt( 1.0f - u2 ) ) );
		ray_t curRay;
		curRay.org = lightPos;
		curRay.dir = lightDir;//現在のレイは光源の位置からランダムな方向へ
		//エネルギー保存
		float3 curFlux = float3( g_lightR, g_lightG, g_lightB )*4.0f*kPi*pow( g_lightRadius, 2 )*kPi / g_emitPhotonNum;

		uint trace_end = 0;
		for(uint boundsNum=0; boundsNum<kMaxBounds; boundsNum++){

			if( trace_end==1 ){
				break;
			}
			// 放射束が0.0のフォトンを追跡してもしょうがないので打ち切る
			//if( max( curFlux.r, max(curFlux.g, curFlux.b)) <= 0.0f){
			//	break;
			//}
			int retVal;
			retVal = IntersectScene( curRay, boundsNum );
			if ( retVal < 0 ){//交差した場合にはid が返ってくる。
				break;//何も交差しなかったらやめる。
			}
			float3 orienting_normal;// 方向を付けた交差位置の法線（物体からのレイの入出を考慮して正負反転）
			if( dot( g_hitpoint[ boundsNum ].normal, curRay.dir ) < 0.0f ){//交差位置の法線とレイの内積が負なら
				orienting_normal = g_hitpoint[ boundsNum ].normal;
			}else{
				orienting_normal=(-1.0f * g_hitpoint[ boundsNum ].normal);
			}
			float3 texColor = float3(0,0,0);//debug
			float2 st;
			st.x = g_hitpoint[ boundsNum ].texcoord.x;
			st.y = g_hitpoint[ boundsNum ].texcoord.y;
			texColor = GetTexMap( st, g_hitpoint[ boundsNum ].texId );

			switch ( g_hitpoint[ boundsNum ].matType ) {
				case kMatTypeDiffuse: {
					uint originalIndex;
					InterlockedAdd( OutParams[0], 1, originalIndex );//OutParams はメモリの値。キャッシュにうまく置きたい。
					if( originalIndex >= g_emitPhotonNum*kMemorySpaceTimes ){//メモリ確保している量よりも多くなったらやめる。
						trace_end = 1;
						break;
					}
					OutPhotonDb[ originalIndex ].x = g_hitpoint[ boundsNum ].position.x;
					OutPhotonDb[ originalIndex ].y = g_hitpoint[ boundsNum ].position.y;
					OutPhotonDb[ originalIndex ].z = g_hitpoint[ boundsNum ].position.z;
					OutPhotonDb[ originalIndex ].r = curFlux.r;
					OutPhotonDb[ originalIndex ].g = curFlux.g;
					OutPhotonDb[ originalIndex ].b = curFlux.b;

					// 反射するかどうかをロシアンルーレットで決める
					// 例によって確率は任意。今回はフォトンマップ本に従ってRGBの反射率の平均を使う
					float probability = (texColor.r + texColor.g + texColor.b) / 3.0f;
					//float probability = 0.5;
					if( probability > rand01( ) ) { // ロシアンルーレットの結果により反射の場合
						float3 w, u, v;// orienting_normalの方向を基準とした正規直交基底(w, u, v)を作る。
						w = orienting_normal;//交差位置の法線（物体からのレイの入出を考慮）
						//w = normal;//交差位置の法線（物体からのレイの入出を考慮）
						if (abs(w.x) > 0.1f){
							u = normalize( cross(float3(0.0f, 1.0f, 0.0f), w) );
						}else{
							u = normalize( cross(float3(1.0f, 0.0f, 0.0f), w) );
						}
						v = cross(w, u);
						float r1 = 2.0f*kPi*rand01( );
						float r2 = rand01( );
						float r2s= sqrt( r2 );
						float3 reflectionDir	 = normalize( (u*cos(r1)*r2s + v*sin(r1)*r2s + w*sqrt(1.0f - r2) ) );
						float3 hitpointWithOffset= g_hitpoint[ boundsNum ].position + reflectionDir * kHitpointOffsetEps;//めり込まないようにちょっと浮かす
						curRay.org = hitpointWithOffset;
						curRay.dir = reflectionDir;
						curFlux.r *= texColor.r / probability;
						curFlux.g *= texColor.g / probability;
						curFlux.b *= texColor.b / probability;
						//curFlux.r = texColor.r;
						//curFlux.g = texColor.g;
						//curFlux.b = texColor.b;
						break;//次のフォトンへ。ここではtrace_endしていないのでもう一度ループ。
					} else { // 吸収（すなわちここで追跡終了）
						trace_end = 1;
						break;//次のフォトンへ
					}
				} 
				break;
				case kMatTypeReflection: {//鏡面反射 完全鏡面なのでフォトン格納しない	
					// 完全鏡面なのでレイの反射方向は決定的。
					float3 reflectionDir	 = curRay.dir - g_hitpoint[ boundsNum ].normal*2.0f*dot( g_hitpoint[ boundsNum ].normal, curRay.dir );
					float3 hitpointWithOffset= g_hitpoint[ boundsNum ].position + reflectionDir*kHitpointOffsetEps;//めり込まないようにちょっと浮かす
					curRay.org = hitpointWithOffset;
					curRay.dir = reflectionDir;
					curFlux *= texColor;
					break;
				}
				break;

				case kMatTypeRefraction: {//屈折 完全鏡面なのでフォトン格納しない
					float3 reflectionDir	 = curRay.dir - g_hitpoint[ boundsNum ].normal*2.0f*dot( g_hitpoint[ boundsNum ].normal, curRay.dir );
					float3 hitpointWithOffset= g_hitpoint[ boundsNum ].position + reflectionDir * kHitpointOffsetEps;
					ray_t reflection_ray;
					reflection_ray.org = hitpointWithOffset;
					reflection_ray.dir = reflectionDir;
					bool into; // レイがオブジェクトから出るのか、入るのか
					if( dot(g_hitpoint[ boundsNum ].normal, orienting_normal) > 0.0f ){	//orienting_normal : 交差位置の法線
						into = true;
					} else {
						into = false;
					}
					// Snellの法則
					float nc = 1.0f; // 真空の屈折率
					float nt = 1.5f; // オブジェクトの屈折率これは本当はマテリアル設定
					float nnt;
					if( into ){
						nnt = nc / nt;
					} else {
						nnt = nt / nc;
					}
					//レイがオブジェクトから出るときには真空の屈折率/オブジェクトの屈折率。逆の時。
					
					float  ddn	 = dot( curRay.dir, orienting_normal );//レイの方向と交差位置の法線の内積
					float  cos2t = 1.0f - nnt*nnt*( 1.0f - ddn*ddn );

					if( cos2t<0.0f ) { // 全反射した
						curRay.org = reflection_ray.org;//レイは反射したレイに
						curRay.dir = reflection_ray.dir;//レイは反射したレイに
						curFlux *= texColor;
						break;
					}

					// 屈折していく方向
					float3 kMatTypeRefractionDir;//屈折ベクトル
					if( into ){	//入射
						kMatTypeRefractionDir = normalize( curRay.dir*nnt - g_hitpoint[ boundsNum ].normal*( 1.0)*(ddn*nnt + sqrt(cos2t)));
					}else{		//出射
						kMatTypeRefractionDir = normalize( curRay.dir*nnt - g_hitpoint[ boundsNum ].normal*(-1.0)*(ddn*nnt + sqrt(cos2t)));
					}
					float probability = (texColor.r + texColor.g + texColor.b) / 3;
					// 屈折と反射のどちらか一方を追跡する。
					// ロシアンルーレットで決定する。
					//if ( rand01() < probability ) {		// 屈折
					if ( 1 ) {		// 屈折
						float3 hitpointWithRefractionOffset = g_hitpoint[ boundsNum ].position 
															+ kMatTypeRefractionDir * kHitpointOffsetEps;
															//反射ベクトルがめり込まないようにちょっと浮かす
						curRay.org = hitpointWithRefractionOffset;
						curRay.dir = kMatTypeRefractionDir;
						//次回ループ時に判定に使われるレイ。当たったところから決定した方向へ。
						curFlux *= texColor/probability;//現在の色を放射束へ反映
						break;

					} else {									// 反射
						curRay.org = reflection_ray.org;		//次回ループ時に判定に使われるレイ。屈折率を計算した方向へ
						curRay.dir = reflection_ray.dir;
						curFlux *= texColor/(1.0f-probability);			//現在の色を放射束へ反映
						break;
					}
				}
				break;

				case kMatTypeGlossy:
				{
					// TBD
					break;
				}
				break;
			}
		}
		//debug
		if(pid==0){
			OutParams[1] = g_emitPhotonNum;
			OutParams[2] = g_triangleNum;
			OutParams[3] = g_randSeed;
		}
	} else {
		// no photons executed
	}

}

