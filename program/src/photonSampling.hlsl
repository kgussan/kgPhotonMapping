
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
	float		distance;
	float3		position;
	float3		normal;
	float2		texcoord;
	uint		matType;		// 0:diff, 1:spec_refl, 2:spec_refr, 3:glossy
	uint		texId;		
	uint		sw;		
	uint		padding[6];	
};

static const uint	kMaxBounds	= 4;
static hitpoint_t	g_hitpoint[kMaxBounds];	//グローバルの変数。/Gecオプション入れないと怒られる。static を付ければOK

static const uint	kMatTypeDiffuse		= 0;
static const uint	kMatTypeReflection	= 1;
static const uint	kMatTypeRefraction	= 2;
static const uint	kMatTypeGlossy		= 3;
static const float	kHitpointOffsetEps	= 1e-2; //1e-3 ~ 1e-4だとまだ鏡面反射のエラーがある。1e-2が良い。
static const uint	kMemorySpaceTimes	= 10;

static const float	kPi = 3.1415926535898f;
static const float	kEps = 1e-6;
static const float	kInf = 1e20;


//グローバル扱い
cbuffer constBuf_t : register( b0 )
{
	float4x4	g_matViewInv;
	float4x4	g_matViewProjInv;
	uint	g_photonNum;
	uint	g_triangleNum;
	float	g_photonSampleRadius2;
	uint	g_photonSampleMax;	//4
	float	g_colorScale;
	uint	g_screenW;
	uint	g_screenH;
	float	g_camNear;				//8
	float	g_camFar;
	uint	g_sw;
	uint	g_samplingDiv;
	uint	g_padding[5];
};
static const float kPixelCenterOffset = 0.5f;

struct ray_t {
	float3	org;
	float3	dir;
	uint	padding[2];
};


StructuredBuffer<triangle_t>	bufTriangles	: register( t0 );
StructuredBuffer<uint>			bufTex1			: register( t1 );
StructuredBuffer<uint>			bufTex2			: register( t2 );
StructuredBuffer<photon_t>		bufPhotons		: register( t3 );

RWStructuredBuffer<float4>		OutPixel	: register( u0 );
RWStructuredBuffer<uint>		OutIParams	: register( u1 );
RWStructuredBuffer<float>		OutFParams	: register( u2 );



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
int IntersectScene( const ray_t ray, const uint depth )
{
	float distance			= kInf;
	float distanceNearest	= kInf;
	uint  nearestId			= 0;
	uint  intersectOccured	= 0;
	uint  triangleNum		= g_triangleNum;
	[loop]for(uint i=0; i<triangleNum; i++) {
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
		if( det > -eps && det < eps){
			//衝突していない。
			continue;
		}
		float invDet = 1.0f / det;
		float3 tvec = ray.org - float3( bufTriangles[i].v0.x, bufTriangles[i].v0.y, bufTriangles[i].v0.z );
		u = dot( tvec, pvec ) * invDet;
		if( u<0.0f || u>1.0f ){
			//衝突していない。
			continue;
		}
		float3 qvec = cross( tvec, edge1 );
		v = dot( ray.dir, qvec ) * invDet;
		if(v<0.0f || u+v>1.0f ){
			//衝突していない。
			continue;
		}
		t = dot( edge2, qvec ) * invDet;
		//パラメタ設定
		intersectOccured = 1;
		distance		 = t;
		if( intersectOccured 
			&& ( distance > 0.0f )
			&& ( distance < distanceNearest ) ){//一番近いものを適用
			distanceNearest = distance;//衝突点との距離更新
			nearestId = i;
			intersectOccured = 1;
			g_hitpoint[depth].position	= ray.org + t * ray.dir;	//交差した点
			g_hitpoint[depth].normal	= float3( bufTriangles[i].v0.nx, bufTriangles[i].v0.ny, bufTriangles[i].v0.nz );
											//三角形の面法線そのまま渡せばよい。
			g_hitpoint[depth].texcoord	= float2(	  bufTriangles[i].v0.u * (1-u-v) 
											+ bufTriangles[i].v1.u * u 
											+ bufTriangles[i].v2.u * v,
											  bufTriangles[i].v0.v * (1-u-v) 
											+ bufTriangles[i].v1.v * u 
											+ bufTriangles[i].v2.v * v );//重心座標から
			g_hitpoint[depth].matType	= bufTriangles[i].matType;
			g_hitpoint[depth].texId	= bufTriangles[i].texId;
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

float3 CalcDiffuse( float3 texColor, float rrProbability, const uint depth)
{
	float3 ret;
	float3 accumulatedFlux	= float3( 0.0f, 0.0f, 0.0f );
	float3 accumulatedFluxA = float3( 0.0f, 0.0f, 0.0f );
	float3 accumulatedFluxB = float3( 0.0f, 0.0f, 0.0f );
	float maxDistance2 = -1.0f;
	photon_t ptn;
	const float samplingR2 = g_photonSampleRadius2;
	const float k=1.1f;

	for( uint i=0; i<g_photonNum/2; i++){
		ptn.x = bufPhotons[i].x;
		ptn.y = bufPhotons[i].y;
		ptn.z = bufPhotons[i].z;
		ptn.r = bufPhotons[i].r;
		ptn.g = bufPhotons[i].g;
		ptn.b = bufPhotons[i].b;
		float dist2 = ( ptn.x - g_hitpoint[depth].position.x )*( ptn.x - g_hitpoint[depth].position.x )
					+ ( ptn.y - g_hitpoint[depth].position.y )*( ptn.y - g_hitpoint[depth].position.y )
					+ ( ptn.z - g_hitpoint[depth].position.z )*( ptn.z - g_hitpoint[depth].position.z );
		if( dist2 < samplingR2 ){
			maxDistance2 = max( maxDistance2, dist2 );
			const float	w1		= 1.0f; // 円錐フィルタの重み。
			float		w2		= -(sqrt( dist2 )/(k)); // 円錐フィルタの重み。
			float3		ptnPow	= float3( ptn.r, ptn.g, ptn.b );
			float3		v		= texColor*ptnPow/kPi;
			accumulatedFluxA += w1 * v;
			accumulatedFluxB += w2 * v;
		}
	}
	accumulatedFlux = accumulatedFluxA + accumulatedFluxB / sqrt( maxDistance2 ); //ループ中から移動して単純ループに最適化
	accumulatedFlux /= (1.0f - 2.0f/(3.0f*k)); 
								
	if(maxDistance2>0.0f){//最大半径の二乗が0より大きな正の数であれば（ゼロ除算回避）
		//衝突物体のエミッション値と足した値
		ret.rgb = accumulatedFlux / ( kPi * maxDistance2 ) / rrProbability;
	}
	return ret;
}

//1スレッド、1フォトン
[numthreads(8,8,1)]
void main( uint3 id : SV_DispatchThreadID )
{
	uint pixId		= id.x + id.y * g_screenW;
	#if (USE_NATHAN_REED_RANDOM)
	rng_state = id.x;
	#else
	g_randSeedPix = (float)pid;
	#endif

	uint wBegin = 0;
	uint wEnd	= 0;
	wBegin	= g_screenW*g_sw      /g_samplingDiv;
	wEnd	= g_screenW*(g_sw + 1)/g_samplingDiv;

	//if( g_screenW > id.x && g_screenH > id.y ) {//実行対象のフォトンであれば
	if( ( wBegin <= id.x ) && ( wEnd > id.x )
		  && ( g_screenH > id.y ) ) {//実行対象のフォトンであれば
		float4 retColor = float4(0,0,0,1); 
		float3 texColor = float3(0,0,0);	//material 

		//[-1.0f,1.0f]-> x value
		float4 rayStart_in_projSpace = float4(	(2.0f*( ( kPixelCenterOffset + (float)(id.x) )/(float)( g_screenW ) ) - 1.0f )/(float)(g_screenW),
												(2.0f*( ( kPixelCenterOffset + (float)(id.y) )/(float)( g_screenH ) ) - 1.0f )/(float)(g_screenH),
												0.0f,
												1.0f );
		float4		eyePos_in_viewSpace	 = float4(0.0f,0.0f,0.0f,1.0f);
		float4		rayStart;
		float4		rayToward;
		float4		rayDir;
		float4x4	matViewInv;
		float4x4	matViewProjInv;
		matViewInv		= g_matViewInv;
		matViewProjInv	= g_matViewProjInv;
		rayStart	= mul( matViewInv,		eyePos_in_viewSpace );		//レイの開始点
		rayToward	= mul( matViewProjInv,	rayStart_in_projSpace );	//レイの方角
		rayToward.x /= rayToward.w;//perspective補正
		rayToward.y /= rayToward.w;
		rayToward.z /= rayToward.w;
		rayToward.w /= rayToward.w;
		rayDir.x = rayToward.x - rayStart.x;//方向計算
		rayDir.y = rayToward.y - rayStart.y;
		rayDir.z = rayToward.z - rayStart.z;
		rayDir.w = rayToward.w - rayStart.w;
		normalize( rayDir );
		rayStart.xyz += rayDir.xyz * g_camNear; //nearクリップから

		ray_t pixRay;
		pixRay.org = float3(0,0,0);
		pixRay.dir = float3(0,0,0);
		ray_t pixRayRefraction;
		pixRayRefraction.org = float3(0,0,0);
		pixRayRefraction.dir = float3(0,0,0);

		uint rtExitEnable = 0;	//途中で抜けるフラグ

		pixRay.org = rayStart.xyz;
		pixRay.dir = rayDir.xyz;

		[unroll]for(uint boundsNum=0; boundsNum<kMaxBounds; boundsNum++) {
			
			//エンドフラグで終了
			if( rtExitEnable==1 ){
				break;
			}

			//レイトレ交差判定
			int retVal;
			retVal = IntersectScene( pixRay, boundsNum );
			if ( retVal < 0 ){//交差した場合にはid が返ってくる。
				OutPixel[ pixId ] = retColor;//交差しなかったときの色。背景色
				return;//終わり
			}

			//※交差した後にグローバルに交点情報が入る。
			float3 orienting_normal;// 方向を付けた交差位置の法線（物体からのレイの入出を考慮して正負反転）
			if( dot( g_hitpoint[ boundsNum ].normal, pixRay.dir ) < 0.0f ){//交差位置の法線とレイの内積が負なら
				orienting_normal = g_hitpoint[ boundsNum ].normal;
			}else{
				orienting_normal = (-1.0f * g_hitpoint[ boundsNum ].normal);
			}
			float2 st;
			st.x = g_hitpoint[ boundsNum ].texcoord.x;
			st.y = g_hitpoint[ boundsNum ].texcoord.y;
			texColor.rgb = GetTexMap( st, g_hitpoint[ boundsNum ].texId ).rgb;

			// 色の反射率最大のものを得る。ロシアンルーレットで使う。
			// ロシアンルーレットの閾値は任意だが色の反射率等を使うとより良い。
			float rrProbability = max( texColor.r, max(texColor.g, texColor.b) );//やめる。
			/*
			これは反射・屈折の時に入れていく。毎時繰り返す。
			// 一定以上レイを追跡したらロシアンルーレットを実行し追跡を打ち切るかどうかを判断する
			if( boundsNum > kRrBounds ){
				if( rand01( ) >= rrProbability ){	//ロシアンルーレット
					//return float4( texColor.rgb, 1.0f );
					OutPixel[ pixId ] = retColor;	//ランダムに処理を終える。
					return;
				}
			}else if( boundsNum >= kMaxBounds ){
				//return float4( texColor.rgb, 1);
				OutPixel[ pixId ] = retColor;
			}else{
				rrProbability = 1.0; // ロシアンルーレット実行しなかった。普通に実行。
			}//ここで最悪打ち切りの上限を決めた方が読みやすいだろう。
			*/
			rrProbability = 1.0f;//RR 無しだから1固定でOK
			retVal = 0;

			//素材に依存して処理を分ける
			switch ( g_hitpoint[ boundsNum ].matType ) {

				case kMatTypeDiffuse: {//ディヒューズ
					retColor.rgb += CalcDiffuse( texColor, rrProbability, boundsNum );
					rtExitEnable = true;
				}
				break;

				case kMatTypeReflection:
				if(1){//スペキュラ
					pixRay.dir = pixRay.dir - g_hitpoint[boundsNum].normal*2.0f*dot(g_hitpoint[boundsNum].normal, pixRay.dir);// 反射方向から放射輝度をもらう
					pixRay.org = g_hitpoint[boundsNum].position + pixRay.dir * kHitpointOffsetEps;//反射ベクトルがめり込まないようにちょっと浮かす
					if( boundsNum+1 < kMaxBounds ){
						retVal = IntersectScene( pixRay, boundsNum+1 ); //次のインデックスでレイを計算して再度計算
						if ( retVal < 0 ){
							rtExitEnable = true;
							break;
						}
					}
					break;
				}
				break;

				case kMatTypeRefraction:
				if(1){//屈折
					//初めに反射ベクトルの生成
					const float nc	= 1.0f;
					const float nt	= 1.5f;
					float ddn		= 0.0f;
					float cos2t		= 0.0f;
					pixRay.dir = pixRay.dir - g_hitpoint[ boundsNum ].normal*2.0f*dot( g_hitpoint[ boundsNum ].normal, pixRay.dir );
					pixRay.org = g_hitpoint[ boundsNum ].position + pixRay.dir * kHitpointOffsetEps;

					//bool into = dot( g_hitpoint[ boundsNum ].normal, orienting_normal ) > 0.0; // レイがオブジェクトから出るのか、入るのか
					bool into = 0;
					if( dot( g_hitpoint[ boundsNum ].normal, orienting_normal ) > 0.0 ){// レイがオブジェクトから出るのか、入るのか
						into = 1;
					}

					// Snellの法則
					float nnt;
					if( into ){
						nnt = nc / nt;//真空からオブジェクトへ、入射
					}else{
						nnt = nt / nc;//オブジェクトから真空へ、出射
					}
					ddn = dot( pixRay.dir, orienting_normal );//レイの方向と、方向を付けた交差位置の法線の内積
					cos2t = 1.0f - nnt * nnt * ( 1.0f - ddn * ddn );

					if (cos2t < 0.0) { // 全反射した
						//次の例を作って、次の反射へ進む
						if( boundsNum+1 < kMaxBounds ){
							retVal = IntersectScene( pixRay, boundsNum + 1 );
							if ( retVal < 0 ){
								rtExitEnable = true;
								break;
							}
						}
						break;
					}

					// 屈折していく方向(球に特化した式？)
					float3 tDir;
					if( into ){//入射であれば
						tDir = normalize( pixRay.dir*nnt - g_hitpoint[ boundsNum ].normal*(+1.0f)*(ddn*nnt + sqrt(cos2t)));
					}else{//出射の場合
						tDir = normalize( pixRay.dir*nnt - g_hitpoint[ boundsNum ].normal*(-1.0f)*(ddn*nnt + sqrt(cos2t)));
					}
					// SchlickによるFresnelの反射係数の近似
					float a = nt - nc;
					float b = nt + nc;
					float R0 = (a * a) / (b * b);

					//float c = 1.0f - (into ? -ddn : dot(tDir, g_hitpoint[ boundsNum ].normal));
					float c;
					if( into ){
						c = 1.0f + ddn;
					} else {
						c = 1.0f - dot( tDir, g_hitpoint[ boundsNum ].normal );
					}
					float Re = R0 + (1.0f - R0) * pow( (float)c, (float)5.0f );
					float Tr = 1.0f - Re; // 屈折光の運ぶ光の量
					float probability = 0.25f + 0.5f * Re;

					{	//shader 構造上、どちらか一方に飛ぶ形式に

						//if( probability > 0.5f ){
						if( 0 ){
							//reflection
							if( boundsNum+1 < kMaxBounds ){
								retVal = IntersectScene( pixRay, boundsNum + 1 );
								if ( retVal < 0 ){
									rtExitEnable = true;
									break;
								}
							}
							break;
						}else{
							pixRayRefraction.dir = tDir;
							pixRayRefraction.org = g_hitpoint[ boundsNum ].position + tDir*kHitpointOffsetEps;
							//refraction
							if( boundsNum+1 < kMaxBounds ){
								retVal = IntersectScene( pixRayRefraction, boundsNum + 1 );
								if ( retVal < 0 ){
									rtExitEnable = true;
									break;
								}
							}
							break;
						}
					}
				}
				break;
				case kMatTypeGlossy:
					{
					//T.B.D
					}
				break;
			}
		}

		//出力生成
		OutPixel[ pixId ].r = retColor.r;
		OutPixel[ pixId ].g = retColor.g;
		OutPixel[ pixId ].b = retColor.b;
		OutPixel[ pixId ].r *= g_colorScale;
		OutPixel[ pixId ].g *= g_colorScale;
		OutPixel[ pixId ].b *= g_colorScale;
		OutPixel[ pixId ].a = 1.0f; 
#if 0
		if( pixId==0 ){
			OutIParams[1] = g_photonNum;
			OutIParams[2] = g_triangleNum;
			OutIParams[3] = g_screenW;
			OutIParams[4] = g_screenH;

			OutFParams[0] = g_camNear;
			OutFParams[1] = g_camFar;
			OutFParams[2] = g_photonSampleRadius2;
			OutFParams[3] = g_colorScale;

			for(uint i=0;i<16;i++){
				OutFParams[16 + i] = g_matViewInv[i/4][i%4];
			}
			for(uint j=0;j<16;j++){
				OutFParams[32 + j] = g_matViewProjInv[j/4][j%4];
			}
			for(uint k=0;k<16;k++){
				OutFParams[48 + k] = (float)k;
			}
		}
#endif
		if( pixId==0 ){ 
			const uint startIndex = 64;
			OutFParams[startIndex + 0] = pixRay.org.x;
			OutFParams[startIndex + 1] = pixRay.org.y;
			OutFParams[startIndex + 2] = pixRay.org.z;
			OutFParams[startIndex + 3] = pixRay.dir.x;
			OutFParams[startIndex + 4] = pixRay.dir.y;
			OutFParams[startIndex + 5] = pixRay.dir.z;
		}
	}

}


