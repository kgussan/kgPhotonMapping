//matrix
//================================================================
#include "kgalgebra.h"
#include <string.h>

//
//matrix order (覚え書き、column↓majar）
//
//		0	4	8	12
//	(	1	5	9	13	)
//		2	6	10	14
//		3	7	11	15
//
//
namespace nsKg{
//プロジェクション行列を作る(OpenGL定義)
//		(2*near)/(right-left)		0						(right+left)/(right-left)			0
//			0					(2*near)/(top-bottom)		(top+bottom)/(top-bottom)			0
//			0						0						-(far+near)/(far-near)			-(2.0f*far*near)/(far-near)
//			0						0								-1							0
//@param	入力
//		マトリクスポインタ
//		プロジェクション行列情報
//@retval	出力
//		マトリクスポインタへ
//@par		注意
void BuildFrustumMatrix(float *M,
						const float left,
						const float right,//LRBTNF
						const float bottom, 
						const float top,
						const float near,
						const float far)
{
	memset(M,0,16*sizeof(float)); 
	M[0*4+0]=(2.0f*near)/(right-left);
	M[1*4+1]=(2.0f*near)/(top-bottom);

	M[2*4+0]=(right+left)/(right-left);
	M[2*4+1]=(top+bottom)/(top-bottom);
	M[2*4+2]=-(far+near)/(far-near);
	M[2*4+3]=-1.0f; 
	M[3*4+2]=-(2.0f*far*near)/(far-near);
	MatrixTranspose( M );
}

//vector mathと同じ
void BuildPerspectiveMatrix(	float *m, 
								const float fovy, 
								const float aspect, 
								const float znear, 
								const float zfar )
{
	const float h = (float)(1.0f/tan(fovy*M_PI/360.0f));
	float neg_depth = znear-zfar;
 
	m[0] = h / aspect;
	m[1] = 0;
	m[2] = 0;
	m[3] = 0;
 
	m[4] = 0;
	m[5] = h;
	m[6] = 0;
	m[7] = 0;
 
	m[8] = 0;
	m[9] = 0;
	m[10] = (zfar + znear)/neg_depth;
	m[11] = -1;
 
	m[12] = 0;
	m[13] = 0;
	m[14] = 2.0f*(znear*zfar)/neg_depth;
	m[15] = 0;
}

//正射影行列を作る(OpenGL定義)
void BuildOrthoMatrix(float *M,
				const float left,const float right,//LRBTNF
				const float bottom,const float top,
				const float near,const float far)
{
	memset(M,0,16*sizeof(float)); 
	M[0*4+0]=(2.0f)/(right-left);//OpenGL redbookと同じ形式
	M[1*4+1]=(2.0f)/(top-bottom);
	M[2*4+2]=-(2.0f)/(far-near);

	M[3*4+0]=(right+left)/(right-left);
	M[3*4+1]=(top+bottom)/(top-bottom);
	M[3*4+2]=(far+near)/(far-near);
	M[3*4+3]=1.0f; 
}

void BuildLookAtMatrix(float M[16],
						const float eyex,	const float eyey,	const float eyez,
						const float centerx,const float centery,const float centerz,
						const float upx,	const float upy,	const float upz)//通常upx=0 upy=1 upz=0でよい。
{
	float l;
	float tx,ty,tz;
	// 下の l は 0 になることがあるから,
	// 必要ならエラー処理を追加して.
	// z 軸 = e - t
	tx = eyex - centerx;
	ty = eyey - centery;
	tz = eyez - centerz;
	l = sqrtf(tx * tx + ty * ty + tz * tz);
	M[ 2] = tx / l;
	M[ 6] = ty / l;
	M[10] = tz / l;
	// x 軸 = up x z 軸
	tx = upy * M[10] - upz * M[ 6];
	ty = upz * M[ 2] - upx * M[10];
	tz = upx * M[ 6] - upy * M[ 2];
	l = sqrtf(tx * tx + ty * ty + tz * tz); 
	M[ 0] = tx / l;
	M[ 4] = ty / l;
	M[ 8] = tz / l;
	// y 軸 = z 軸 x x 軸 
	M[ 1] = M[ 6] * M[ 8] - M[10] * M[ 4];
	M[ 5] = M[10] * M[ 0] - M[ 2] * M[ 8];
	M[ 9] = M[ 2] * M[ 4] - M[ 6] * M[ 0];
	// 平行移動(回転してから移動)
	M[12] = -(eyex * M[ 0] + eyey * M[ 4] + eyez * M[ 8]);
	M[13] = -(eyex * M[ 1] + eyey * M[ 5] + eyez * M[ 9]);
	M[14] = -(eyex * M[ 2] + eyey * M[ 6] + eyez * M[10]);
	// 残り
	M[ 3] = M[ 7] = M[11] = 0.0f;
	M[15] = 1.0f;
}

//translate -> rotate
//移動成分生成に回転を加味しない。
void BuildLookAtMatrixTR(float M[16],
						const float eyex,	const float eyey,	const float eyez,
						const float centerx,const float centery,const float centerz,
						const float upx,	const float upy,	const float upz)	//通常upx=0 upy=1 upz=0でよい。
{
	float l;
	float tx,ty,tz;
	// 下の l は 0 になることがあるから,
	// 必要ならエラー処理を追加して.
	// z 軸 = e - t
	tx = eyex - centerx;
	ty = eyey - centery;
	tz = eyez - centerz;
	l = sqrtf(tx * tx + ty * ty + tz * tz); //この l と,
	M[ 2] = tx / l;
	M[ 6] = ty / l;
	M[10] = tz / l;
	// x 軸 = u x z 軸
	tx = upy * M[10] - upz * M[ 6];
	ty = upz * M[ 2] - upx * M[10];
	tz = upx * M[ 6] - upy * M[ 2];
	l = sqrtf( tx * tx + ty * ty + tz * tz ); // この l. 
	M[ 0] = tx / l;
	M[ 4] = ty / l;
	M[ 8] = tz / l;
	// y 軸 = z 軸 x x 軸 
	M[ 1] = M[ 6] * M[ 8] - M[10] * M[ 4];
	M[ 5] = M[10] * M[ 0] - M[ 2] * M[ 8];
	M[ 9] = M[ 2] * M[ 4] - M[ 6] * M[ 0];
	// 平行移動（移動）
	M[ 3*4 + 0 ] = -eyex; // x translate
	M[ 3*4 + 1 ] = -eyey; // y translate
	M[ 3*4 + 2 ] = -eyez; // z translate
	// 残り
	M[ 3] = M[ 7] = M[11] = 0.0f;
	M[15] = 1.0f;
}

void BuildViewportMatrix(	float M[16],
							const uint32_t width, 
							const uint32_t height)
{
	MatrixIdentity( M );
	M[ 0*4 + 0 ] = +0.5f * ( float )width;
	M[ 1*4 + 1 ] = -0.5f * ( float )height;
	M[ 0*4 + 3 ] = +0.5f * ( float )width;
	M[ 1*4 + 3 ] = +0.5f * ( float )height;

}

void GeneraeteFrustumCullingF4LRBTNF(float *F,		//4float x6 (24float ) (l r b t n f)
									const float *M, 
									bool debug, FILE *fp)	//16 float 
{
	memset(F,0,4*6*sizeof(float)); 
	F[0*4+0]=M[3]+M[0];
	F[0*4+1]=M[7]+M[4];
	F[0*4+2]=M[11]+M[8];
	F[0*4+3]=M[15]+M[12];//left
	F[1*4+0]=M[3]-M[0];
	F[1*4+1]=M[7]-M[4];
	F[1*4+2]=M[11]-M[8];
	F[1*4+3]=M[15]-M[12]; //right

	F[2*4+0]=M[3]+M[1];
	F[2*4+1]=M[7]+M[5];
	F[2*4+2]=M[11]+M[9];
	F[2*4+3]=M[15]+M[13];//bottom
	F[3*4+0]=M[3]-M[1];
	F[3*4+1]=M[7]-M[5];
	F[3*4+2]=M[11]-M[9];
	F[3*4+3]=M[15]-M[13];//top

	F[4*4+0]=M[3]+M[2];
	F[4*4+1]=M[7]+M[6];
	F[4*4+2]=M[11]+M[10];
	F[4*4+3]=M[15]+M[14];//near
	F[5*4+0]=M[3]-M[2];
	F[5*4+1]=M[7]-M[6];
	F[5*4+2]=M[11]-M[10];
	F[5*4+3]=M[15]-M[14];//far

	if (debug) {
		fprintf(fp,"frustum culling formula \n");
		for (uint32_t i=0;i<24;i++) {
			fprintf(fp,"F[%d]=%3.3f ",i,F[i]);
				if (i%4==3) {
					fprintf(fp,"\n");
				}
		}
	}

}

bool PointInFrustum(float x,float y,float z,
					const float *F, bool debug, FILE *fp) //4float x6 (24float ) (l r b t n f)
{
	int p;
	for (p=0;p<6;p++) {
		if (F[p*4+0]*x + F[p*4+1]*y + F[p*4+2]*z + F[p*4+3]*1.0f <= 0) {
			if (debug) {
				fprintf(fp,"frustum rejected side=%d \n",p);
			}
			return false;
		}
	}
	return true;
}

bool SphereInFrustum(float x,float y,float z,float r,
					const float *F, bool debug, FILE *fp) //4float x6 (24float ) (l r b t n f)
{
	int p;
	for (p=0;p<6;p++) {
		if (F[p*4+0]*x + F[p*4+1]*y + F[p*4+2]*z + F[p*4+3] <= -r) {
			return false;
		}
	}
	return true;
}

//----------------------------------------------------------------
//衝突など
//----------------------------------------------------------------
int CheckHitPointAndTriangle3D( float A[3], float B[3], float C[3], float P[3] )
{
	float AB[3];
    float BP[3];
    float BC[3];
    float CP[3];
    float CA[3];
    float AP[3];
	Vector3Sub(AB,B,A);
    Vector3Sub(BP,P,B);
    Vector3Sub(BC,C,B);
    Vector3Sub(CP,P,C);
    Vector3Sub(CA,A,C);
    Vector3Sub(AP,P,A);

    float c1[3];
    float c2[3];
    float c3[3];
    Vector3Cross( c1, AB, BP );
    Vector3Cross( c2, BC, CP );
    Vector3Cross( c3, CA, AP );
			   
    //内積で順方向か逆方向か調べる
    float dot_12 = Vector3Dot(c1, c2);
    float dot_13 = Vector3Dot(c1, c3);
    if ( dot_12 > 0 && dot_13 > 0 ) {
        //三角形の内側に点がある
        return 0;
    }
    //三角形の外側に点がある
    return 1;
}


int CheckIntersectLines( float resultAB[3], float resultCD[3],
                     const float A[3], const float B[3], const float C[3], const float D[3] )
{
    float AB[3];
    float CD[3];
    Vector2Sub(AB,A,B);
    Vector2Sub(CD,C,D);
    //A=B C=Dのときは計算できない
	if (		Vector3GetLengthSquared(AB)==0 
		||	Vector3GetLengthSquared(CD)==0) {
        return 0;
    }
	float n1[3],n2[3];
	Vector3Normalize(n1,AB);
	Vector3Normalize(n2,CD);
    float w1=Vector3Dot(n1,n2);
    float w2=1-w1*w1;
    //直線が平行な場合は計算できない 平行だとwork2が0になる
    if ( w2==0) { return 0; }
    float AC[3];
	Vector3Sub(AC,C,A);
    float d1 = ( Vector3Dot(AC, n1) - w1*Vector3Dot(AC,n2)) / w2;
    float d2 = ( w1*Vector3Dot(AC,n1) - Vector3Dot(AC,n2) ) / w2;
    //AB上の最近点
    resultAB[0] = A[0] + d1*n1[0];
    resultAB[1] = A[1] + d1*n1[1];
    resultAB[2] = A[2] + d1*n1[2];

    //BC上の最近点
    resultCD[0] = C[0] + d2*n2[0];
    resultCD[1] = C[1] + d2*n2[1];
    resultCD[2] = C[2] + d2*n2[2];

	float tmp[3];
	Vector3Sub(tmp,resultAB,resultCD);
    //交差の判定 誤差は用途に合わせてください
	const float eps=1-6;
	if ( Vector3GetLengthSquared(tmp) < eps ) {
        //交差した
        return 1;
    }
    //交差しなかった。
    return 2;
}


//頂点ABCで作られたポリゴンから法線を計算する。
void BuildTriangleNormal( float N[3], const float A[3], const float B[3], const float C[3] )
{
	float AB[3],BC[3];
	Vector3Sub(AB,B,A);
	Vector3Sub(BC,C,B);
	float normal[3];
	Vector3Cross(normal,AB,BC);
	Vector3Normalize(normal);
	Vector3Copy(N,normal);
}

int CheckTriangleSide( float A[3], float B[3], float C[3], float v[3] )
{
    //ABCが三角形かどうか。ベクトルvが0でないかの判定は省略します
    //AB BCベクトル
    float AB[3];
    float BC[3];
	Vector3Sub(AB,B,A);
	Vector3Sub(BC,C,B);
    //AB BCの外積
    float c[3];
	Vector3Cross(c, AB ,BC);//triangle の法線
    //ベクトルvと内積。順、逆方向かどうか調べる
	float d=Vector3Dot(v,c);
    if (d<0.0f) {
        return 0;    //ポリゴンはベクトルvから見て表側
    }else if (d>0.0f) {
        return 1;    //ポリゴンはベクトルvから見て裏側
    }
    // d==0 ポリゴンは真横を向いている。表裏不明
    return -1;
}

void BuildPlaneFromPointNormal( float plane[4], const float p[3], const float normal[3] )
{
	//pとnormalを内積
	float d = Vector3Dot(p,normal);
	plane[0]=normal[0];
	plane[1]=normal[1];
	plane[2]=normal[2];
	plane[3]=d;
}

void BuildPlaneFromVertex( float plane[4], float a[3], float b[3], float c[3] )
{	
	//ポリゴンの法線を計算する
	float normal[3];
	BuildTriangleNormal(normal,a,b,c);
	//ポリゴンのどれかひとつの頂点と法線ベクトルから平面を作成する
	BuildPlaneFromPointNormal(plane,a,normal);
}


//点Aと平面の距離を求める
//その１( P=平面上の点 N=平面の法線 )
float GetDistancePointAndPlane( const float A[3], const float P[3], const float N[3] )
{
	//PAベクトル(A-P)
	float PA[3];
	Vector3Sub(PA,A,P);
	//法線NとPAを内積... その絶対値が点と平面の距離
    return abs( Vector3Dot( N, PA ) );
}

//点Aと平面の距離を求める
//その２(平面方程式 ax+by+cz+d=0 を使う場合 )
float GetDistancePointAndPlane( const float A[3], const float plane[4])
{
    //平面方程式から法線と平面上の点を求める
    //平面の法線N( ax+by+cz+d=0 のとき、abcは法線ベクトルで単位ベクトルです )
    float N[3];
	N[0]=plane[0];
	N[1]=plane[1];
	N[2]=plane[2];
    //平面上の任意の点P (法線*dは平面上の点)
    float P[3];
	P[0]=plane[0]*plane[3];
	P[1]=plane[1]*plane[3];
	P[2]=plane[2]*plane[3];
    return GetDistancePointAndPlane(A, P, N);
}


} // nsKg
