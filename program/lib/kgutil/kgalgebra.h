//ベクタ・行列計算関数群
#ifndef __KGMATRIX_H__
#define __KGMATRIX_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "kgmath.h"
#include "kgmacro.h"

namespace nsKg{
static void Vector3MultiplyScalar(float *r,const float *v,const float s);

// a+b
inline void Vector2Add(float *r,const float *a,const float *b)
{
	r[0]=a[0]+b[0];
	r[1]=a[1]+b[1];
}
// a-b
inline void Vector2Sub(float *r,const float *a,const float *b)
{
	r[0]=a[0]-b[0];
	r[1]=a[1]-b[1];
}
inline float Vector2Dot(const float *a,const float *b)
{
	return a[0]*b[0]+a[1]*b[1];
}
inline void Vector2Dot(float *r,const float *a,const float *b)
{
	*r=a[0]*b[0]+a[1]*b[1];
}

inline void Vector2Cross(float *r, const float a[2], const float b[2])
{
	*r=a[0]*b[1] - a[1]*b[0];
}
inline float Vector2GetLength(const float *a)
{
	return sqrtf(a[0]*a[0]+a[1]*a[1]);
}
inline float Vector2GetLengthSquared(const float *a)
{
	return a[0]*a[0]+a[1]*a[1];
}

inline void Vector2Copy(float *Dest,const float *Src)
{
	memcpy(Dest,Src,sizeof(float)*2);
}
inline void Vector2Normalize(float *r,const float *a)
{
	float mag=sqrtf(a[0]*a[0]+a[1]*a[1]);
	r[0]=a[0]/mag;
	r[1]=a[1]/mag;
}

//----------------
//vector3
//----------------
// a+b
inline void Vector3Add(float *r,const float *a,const float *b)
{
	r[0]=a[0]+b[0];
	r[1]=a[1]+b[1];
	r[2]=a[2]+b[2];
}
// a-b
inline void Vector3Sub(float *r,const float *a,const float *b)
{
	r[0]=a[0]-b[0];
	r[1]=a[1]-b[1];
	r[2]=a[2]-b[2];
}
inline float Vector3Dot(const float *a, const float *b)
{
	return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
inline void Vector3Dot(float *r,const float *a,const float *b)
{
	*r=a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
inline void Vector3GetMag(float *r,const float *a)
{
	*r=sqrtf(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
}

inline float Vector3GetLength(const float *a)
{
	return sqrtf(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
}
inline float Vector3GetLengthSquared(const float *a)
{
	return a[0]*a[0]+a[1]*a[1]+a[2]*a[2];
}

inline void Vector3Normalize(float *r,const float *a)
{
	float mag=sqrtf(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
	r[0]=a[0]/mag;
	r[1]=a[1]/mag;
	r[2]=a[2]/mag;
}
inline void Vector3Normalize(float *r)
{
	float a[3];
	memcpy(a,r,sizeof(float)*3);
	float mag=sqrtf(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
	r[0]=a[0]/mag;
	r[1]=a[1]/mag;
	r[2]=a[2]/mag;
}

inline void Vector3Cross(float *r,const float *a,const float *b)
{
	r[0]=a[1]*b[2]-a[2]*b[1];
	r[1]=a[2]*b[0]-a[0]*b[2];
	r[2]=a[0]*b[1]-a[1]*b[0];
}

// get angle vector3
//in	: angle, vec1[3], vec2[3]
//out	: 
inline void Vector3GetAngleOf2Vector(float *r,const float *a,const float *b)
{
	float t[3];
	float a_mag;
	float b_mag;
	float t_mag;
	Vector3Cross(t,a,b);
	Vector3GetMag(&t_mag,t);
	Vector3GetMag(&a_mag,a);
	Vector3GetMag(&b_mag,b);
	*r=asin(t_mag/(a_mag*b_mag));
}

//２つのベクトルABのなす角度θを求める
inline float Vector3GetAngleOf2Vector(float *a, float *b)
{
	//ベクトルの長さが0だと答えが出ないので注意

	//ベクトルAとBの長さを計算する
	float length_a = Vector3GetLength(a);
	float length_b = Vector3GetLength(b);

	//内積とベクトル長さを使ってcosθを求める
	float dot = Vector3Dot(a,b);
	float cos_theta = dot / ( length_a * length_b );

	//cosθからθを求める
	float theta = acos( cos_theta );	

	//ラジアンでなく0～180の角度でほしい場合はコメント外す
	//theta = theta * 180.0 / PI;

	return theta;
}
static inline void Vector3Copy(float *Dest,const float *Src)
{
	memcpy(Dest,Src,sizeof(float)*3);
}

static inline void Vector3MultiplyScalar(float *vout,const float *v,const float s)
{
	for (size_t i=0;i<3;i++) {
		vout[i]=v[i]*s;
	}
}

static inline void Vector3Show( char *msg, const float *vector, FILE *fp=stdout )
{
	fprintf(fp,"%s\n",msg);
	fprintf(fp, "vector[0]:\t%0.3f\n"
				"vector[1]:\t%0.3f\n"
				"vector[2]:\t%0.3f\n",
				vector[0], vector[1], vector[2]);
}
static inline void Vector3Show( const float *vector, FILE *fp=stdout )
{
	fprintf(fp,	"vector[0]:\t%0.3f\n"
				"vector[1]:\t%0.3f\n"
				"vector[2]:\t%0.3f\n",
				vector[0], vector[1], vector[2]);	
}

inline void Vector4Normalize(float *r,const float *a)
{
	float mag=sqrtf(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]+a[3]*a[3]);
	r[0]=a[0]/mag;
	r[1]=a[1]/mag;
	r[2]=a[2]/mag;
	r[3]=a[3]/mag;
}
inline void Vector4Normalize(float *r)
{
	float a[4];
	memcpy(a,r,sizeof(float)*4);
	float mag=sqrtf(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]+a[3]*a[3]);
	r[0]=a[0]/mag;
	r[1]=a[1]/mag;
	r[2]=a[2]/mag;
	r[3]=a[3]/mag;
}

//----------------------------------------------------------------
//衝突・距離を計算する。
//	関数名はbuild check getで始まる。
//----------------------------------------------------------------
//点と点の距離を計算する。
inline float GetDistancePointAndPoint2D(float a[2], float b[2] )
{
	return sqrtf( (a[0]-b[0])*(a[0]-b[0]) + (a[1]-b[1])*(a[1]-b[1]) ); 
}
inline float GetDistancePointAndPoint3D(float a[3], float b[3] )
{
	return sqrtf( (a[0]-b[0])*(a[0]-b[0]) + (a[1]-b[1])*(a[1]-b[1]) + (a[2]-b[2])*(a[2]-b[2]) ); 
}

//点Pと線ABの距離を計算する。
float GetDistancePointAndLine2D(float P[2], float A[2], float B[2] );
float GetDistancePointAndLine3D(float P[3], float A[3], float B[3] );

//点ABCと点Pの交差を判定する。ABC面上にPがいる必要がある。
// 三角形と点の当たり判定(2Dの場合)
// 戻り値    0:三角形の内側に点がある    1:三角形の外側に点がある
int CheckHitPointAndTriangle2D( float A[2], float B[2], float C[2], float P[2] );
int CheckHitPointAndTriangle3D( float A[3], float B[3], float C[3], float P[3] );

//線AB上で点に一番近い点Resultを得る。
void GetNearPosOnLine2D(float Result[2], const float P[2], const float A[2], const float B[2] );
void GetNearPosOnLine3D(float Result[3], const float P[3], const float A[3], const float B[3] );

//線ABと線CDの交点あるいは最近点を求める
//    戻り値
//    0 計算できず（平行であったりA=B C=Dのばあい）
//    1 交点があった    resultに交点を格納
//    2 交点がない　    resultには最近点を格納
int CheckIntersectLines( float resultAB[3], float resultCD[3],const float A[3], const float B[3], const float C[3], const float D[3] );

//3点ABCから法線Nを作成する。
void BuildTriangleNormal( float N[3], const float A[3], const float B[3], const float C[3] );


// ベクトルvに対してポリゴンが表裏どちらを向くかを求める
// 戻り値    0:表    1:裏    -1:エラー
int CheckTriangleSide( float A[3], float B[3], float C[3], float v[3] );

//ひとつの頂点と法線ベクトルから平面を作成する
//※normalは単位ベクトルであること
void BuildPlaneFromPointNormal( float plane[4], const float p[3], const float normal[3] );

//ポリゴンから平面を作成する
//※abcは同一でないこと
void BuildPlaneFromVertex( float plane[4], float a[3], float b[3], float c[3] );

//点Aと平面の距離を求める
//点と平面の距離 = | PA ・ N |
//参照元 http://www.sousakuba.com/Programming/gs_dot_plane_distance.html
//その１( P=平面上の点 N=平面の法線 )
float GetDistancePointAndPlane( const float A[3], const float P[3], const float N[3] );
//その２(平面方程式 ax+by+cz+d=0 を使う場合 )
float GetDistancePointAndPlane( const float A[3], const float plane[4]);

//平面上の最近点を求める　その１( P=平面上の点 N=平面の法線 )
void GetNearPosOnPlane(float R[3], const float A[3], const float P[3], const float N[3]);
//平面上の最近点を求める　その２(平面方程式 ax+by+cz+d=0 を使う場合 )
void GetNearPosOnPlane(float R[3], const float A[3], const float plane[4]);

//線分ABと平面の交点を計算する
//http://www.sousakuba.com/Programming/gs_plane_line_intersect.html
bool CheckIntersectPlaneAndLine(float out[3], //戻り値　交点が見つかれば格納される
								const float A[3],   //線分始点
								const float B[3],   //線分終点
								const float plane[4] ); //平面


//----------------------------------------------------------------
//Matrix
//----------------------------------------------------------------
static inline void MatrixShow(const float *matrix, FILE *fp=stdout)
{
#if (DEBUG_MODE==1)
	int i;
	float myu;
	//printf("4x4 Matrix display.\n");
	for ( i=0; i<4; i++ ) {
		fprintf(fp,"Matrix[%d][%d][%d][%d]:\t%0.3f %0.3f %0.3f %0.3f\n",
				0*4+i,
				1*4+i,
				2*4+i,
				3*4+i,
				matrix[0*4+i],
				matrix[1*4+i],
				matrix[2*4+i],
				matrix[3*4+i]);
	}
	myu = matrix[0]
		 *matrix[5]
		 *matrix[10]
		+ matrix[1]
		 *matrix[6]
		 *matrix[8]
		+ matrix[2]
		 *matrix[4]
		 *matrix[9]
		- matrix[0]
		 *matrix[9]
		 *matrix[6]
		- matrix[4]
		 *matrix[1]
		 *matrix[10]
		- matrix[8]
		 *matrix[5]
		 *matrix[2];
	fprintf(fp,"myu(3x3 norm)=%f\n",myu);
	//printf( "%s %d myu(3x3 norm)=%f\n", __FILE__, __LINE__, myu );
#endif //DEBUG
}

static inline void MatrixShow(char *msg,const float *matrix, FILE *fp=stdout)
{
	fprintf(fp,"%s\n",msg);
	MatrixShow(matrix,fp);
}

static inline void MatrixShowT2B( const float *matrix, FILE *fp=stdout)
{
	MatrixShow(matrix,fp);
}

static inline void MatrixShowT2B(char *msg,const float *matrix, FILE *fp=stdout)
{
	printf("%s\n",msg);
	MatrixShowT2B(matrix);
}

//matrix order
//	0	1	2	3
//	4	5	6	7	
//	8	9	10	11
//	12	13	14	15
static inline void MatrixShowL2R( const float *matrix, FILE *fp=stdout )
{
#if (DEBUG_MODE==1)
	int i;
	float myu;
	for (i=0;i<4;i++) {
		fprintf(fp,"Matrix[%d][%d][%d][%d]:\t%0.3f %0.3f %0.3f %0.3f\n",
				0*4+i,
				1*4+i,
				2*4+i,
				3*4+i,
				matrix[i*4+0],
				matrix[i*4+1],
				matrix[i*4+2],
				matrix[i*4+3]);
	}
	myu = matrix[0]
		 *matrix[5]
		 *matrix[10]
		+ matrix[1]
		 *matrix[6]
		 *matrix[8]
		+ matrix[2]
		 *matrix[4]
		 *matrix[9]
		- matrix[0]
		 *matrix[9]
		 *matrix[6]
		- matrix[4]
		 *matrix[1]
		 *matrix[10]
		- matrix[8]
		 *matrix[5]
		 *matrix[2];
	fprintf(fp, "%s %d myu(3x3 norm)=%0.3f\n", __FILE__, __LINE__, myu );
#endif //DEBUG
}

static inline void MatrixShowL2R(char *msg,const float *matrix, FILE *fp=stdout)
{
	fprintf(fp,"%s\n",msg);
	MatrixShowL2R(matrix);
}


// 4x4 matrix multiply 4x4 matrix
//		0	4	8	12				0	4	8	12
//	SA(	1	5	9	13	)	x	SB(	1	5	9	13	)
//		2	6	10	14				2	6	10	14
//		3	7	11	15				3	7	11	15
//
// M[ 0]=LA[ 0]*LB[ 0]
//      +LA[ 1]*LB[ 4]
//      +LA[ 2]*LB[ 8]
//      +LA[ 3]*LB[12]
// M[ 1]=LA[ 0]*LB[ 1]+LA[ 1]*LB[ 5]+LA[ 2]*LB[ 9]+LA[ 3]*LB[13]
// M[ 2]=LA[ 0]*LB[ 2]+LA[ 1]*LB[ 6]+LA[ 2]*LB[10]+LA[ 3]*LB[14]
// M[ 3]=LA[ 0]*LB[ 3]+LA[ 1]*LB[ 7]+LA[ 2]*LB[11]+LA[ 3]*LB[15]
// M[ 4]=LA[ 4]*LB[ 0]+LA[ 5]*LB[ 4]+LA[ 6]*LB[ 8]+LA[ 7]*LB[12]
// M[ 8]=LA[ 8]*LB[ 0]+LA[ 9]*LB[ 4]+LA[10]*LB[ 8]+LA[11]*LB[12]
// M[12]=LA[12]*LB[ 0]+LA[13]*LB[ 4]+LA[14]*LB[ 8]+LA[15]*LB[12]
//
static inline void MatrixMultiply(float *M,const float *SA,const float *SB)
{
	float LA[16];
	float LB[16];
	memcpy(LA,SA,sizeof(float)*16);//MとSAかSBが一緒でもいいように入力の値をコピー
	memcpy(LB,SB,sizeof(float)*16);
	for (int i=0;i<4;i++) {
		for (int j=0;j<4;j++) {
			M[i*4+j]=LA[i*4+0]*LB[0*4+j] 
					+LA[i*4+1]*LB[1*4+j] 
					+LA[i*4+2]*LB[2*4+j] 
					+LA[i*4+3]*LB[3*4+j];
		}
	}
}

//マトリクスの定義を変えると大変なのでやめた。
// M[ 0]=LA[ 0]*LB[ 0]
//      +LA[ 4]*LB[ 1]
//      +LA[ 8]*LB[ 2]
//      +LA[12]*LB[ 3]
//static inline void MatrixMultiplyL2R(float *M,const float *SA,const float *SB)
//{
//	float LA[16];
//	float LB[16];
//	memcpy(LA,SA,sizeof(float)*16);
//	memcpy(LB,SB,sizeof(float)*16);
//	for (int i=0;i<4;i++) {
//		for (int j=0;j<4;j++) {
//			M[i*4+j]=LA[0*4+i]*LB[j*4+0] 
//					+LA[1*4+i]*LB[j*4+1] 
//					+LA[2*4+i]*LB[j*4+2] 
//					+LA[3*4+i]*LB[j*4+3];
//		}
//	}
//}

inline void MatrixAdd(float *M,const float *A,const float *B)
{
	float LA[16];
	float LB[16];
	memcpy(LA,A,sizeof(float)*16);
	memcpy(LB,B,sizeof(float)*16);
	for (int i=0;i<16;i++) {
		M[i]=LA[i]+LB[i];
	}
}

//## 4x4 matrix normalize ##
//## info: matrix order ##
//##	0	4	8	12
//##	1	5	9	13
//##	2	6	10	14
//##	3	7	11	15
static inline void MatrixNormalize( float *matrix )
{
	float matrix_org[16];
	memcpy( matrix_org, matrix, sizeof( float )*16 );
	
	float myu = 0.0;
	myu = matrix_org[0]
		 *matrix_org[5]
		 *matrix_org[10]
		+ matrix_org[1]
		 *matrix_org[6]
		 *matrix_org[8]
		+ matrix_org[2]
		 *matrix_org[4]
		 *matrix_org[9]
		- matrix_org[0]
		 *matrix_org[9]
		 *matrix_org[6]
		- matrix_org[4]
		 *matrix_org[1]
		 *matrix_org[10]
		- matrix_org[8]
		 *matrix_org[5]
		 *matrix_org[2];

	matrix[4*0+0] = matrix_org[4*0+0] / myu;
	matrix[4*1+0] = matrix_org[4*1+0] / myu;
	matrix[4*2+0] = matrix_org[4*2+0] / myu;
	matrix[4*3+0] = matrix_org[4*3+0];
	                                
	matrix[4*0+1] = matrix_org[4*0+1] / myu;
	matrix[4*1+1] = matrix_org[4*1+1] / myu;
	matrix[4*2+1] = matrix_org[4*2+1] / myu;
	matrix[4*3+1] = matrix_org[4*3+1];
	                                
	matrix[4*0+2] = matrix_org[4*0+2] / myu;
	matrix[4*1+2] = matrix_org[4*1+2] / myu;
	matrix[4*2+2] = matrix_org[4*2+2] / myu;
	matrix[4*3+2] = matrix_org[4*3+2];
	                                
	matrix[4*0+3] = matrix_org[4*0+3];
	matrix[4*1+3] = matrix_org[4*1+3];
	matrix[4*2+3] = matrix_org[4*2+3];
	matrix[4*3+3] = matrix_org[4*3+3];
}


//inverse 4x4 matrix
//	0	4	8	12
//	1	5	9	13
//	2	6	10	14
//	3	7	11	15
//http://www.cg.info.hiroshima-cu.ac.jp/~miyazaki/knowledge/teche23.html
static inline void MatrixInverseLU( float *matrix_inv_ptr, const float *matrix_org_ptr )
{
	int i, j, k, n;
	float **a, **c, **matrix_org;
	n = 4;
	// malloc
	matrix_org = ( float** )malloc( n*sizeof( float* ) );
	a = ( float ** )malloc( n*sizeof( float* ) );
	c = ( float ** )malloc( n*sizeof( float* ) );

	for ( i=0; i<n; i++ ) {
		a[i] = ( float* )malloc( n*sizeof( float ) );
		c[i] = ( float* )malloc( n*sizeof( float ) );
		matrix_org[i] = ( float* )malloc( n*sizeof(float) );
	}

	// input original matrix
	for ( i=0; i<n; i++ ) {
		for ( j=0; j<n; j++ ) {
			matrix_org[i][j] = matrix_org_ptr[i*4+j];
			a[i][j] = matrix_org[i][j];
			//printf("%lf ", a[i][j]);
		}
		//putchar('\n');
	}

	// LU analysis
	for ( i=0; i<n; i++ ) {
		for ( j=i+1; j<n; j++ ) {
			a[j][i] /= a[i][i];
			for ( k=i+1; k<n; k++ ) {
				a[j][k] -= a[i][k] * a[j][i];
			}
		}
	}
	
	// compute inverse matrix
	for ( k=0; k<n; k++ ) {
		// initialize
		for ( i=0; i<n; i++ ) {
			if ( i==k ) {
				c[i][k] = 1;
			}else{
				c[i][k] = 0;
			}
		}
		// calculate answer
		for ( i=0; i<n; i++ ) {
			for ( j=i+1; j<n; j++ ) {
				c[j][k] -= c[i][k] * a[j][i];
			}
		}
		for ( i=n-1; i>=0; i-- ) {
			for ( j=i+1; j<n; j++ ) {
				c[i][k] -= a[i][j] * c[j][k];
			}
			c[i][k] /= a[i][i];
		}
	}
	// set result
	for ( i=0; i<n; i++ ) {
		for ( j=0; j<n; j++ ) {
			matrix_inv_ptr[i*4+j] = c[i][j];
		}
	}
	//h25-12-13 フリーを忘れていた。追加。
	for ( i=0; i<n; i++ ) {
		free( a[i] );
		free( c[i] );
		free( matrix_org[i] );
	}
	free(a); 
	free(c);
	free(matrix_org);
}

//4x4 inv matrix
//http://stackoverflow.com/questions/1148309/inverting-a-4x4-matrix
static inline bool MatrixInverse(float Mout[16], const float Min[16])
{
	float Minv[16], det;
	int i;
	Minv[0] = Min[5]  * Min[10] * Min[15] - 
			  Min[5]  * Min[11] * Min[14] - 
			  Min[9]  * Min[6]  * Min[15] + 
			  Min[9]  * Min[7]  * Min[14] +
			  Min[13] * Min[6]  * Min[11] - 
			  Min[13] * Min[7]  * Min[10];

	Minv[4] = -Min[4]  * Min[10] * Min[15] + 
			   Min[4]  * Min[11] * Min[14] + 
			   Min[8]  * Min[6]  * Min[15] - 
			   Min[8]  * Min[7]  * Min[14] - 
			   Min[12] * Min[6]  * Min[11] + 
			   Min[12] * Min[7]  * Min[10];

	Minv[8] = Min[4]  * Min[9] * Min[15] - 
			  Min[4]  * Min[11] * Min[13] - 
			  Min[8]  * Min[5] * Min[15] + 
			  Min[8]  * Min[7] * Min[13] + 
			  Min[12] * Min[5] * Min[11] - 
			  Min[12] * Min[7] * Min[9];

	Minv[12] = -Min[4]  * Min[9] * Min[14] + 
			    Min[4]  * Min[10] * Min[13] +
			    Min[8]  * Min[5] * Min[14] - 
			    Min[8]  * Min[6] * Min[13] - 
			    Min[12] * Min[5] * Min[10] + 
			    Min[12] * Min[6] * Min[9];

	Minv[1] = -Min[1]  * Min[10] * Min[15] + 
			   Min[1]  * Min[11] * Min[14] + 
			   Min[9]  * Min[2] * Min[15] - 
			   Min[9]  * Min[3] * Min[14] - 
			   Min[13] * Min[2] * Min[11] + 
			   Min[13] * Min[3] * Min[10];

	Minv[5] = Min[0]  * Min[10] * Min[15] - 
			  Min[0]  * Min[11] * Min[14] - 
			  Min[8]  * Min[2] * Min[15] + 
			  Min[8]  * Min[3] * Min[14] + 
			  Min[12] * Min[2] * Min[11] - 
			  Min[12] * Min[3] * Min[10];

	Minv[9] = -Min[0]  * Min[9] * Min[15] + 
			   Min[0]  * Min[11] * Min[13] + 
			   Min[8]  * Min[1] * Min[15] - 
			   Min[8]  * Min[3] * Min[13] - 
			   Min[12] * Min[1] * Min[11] + 
			   Min[12] * Min[3] * Min[9];

	Minv[13] = Min[0]  * Min[9] * Min[14] - 
			   Min[0]  * Min[10] * Min[13] - 
			   Min[8]  * Min[1] * Min[14] + 
			   Min[8]  * Min[2] * Min[13] + 
			   Min[12] * Min[1] * Min[10] - 
			   Min[12] * Min[2] * Min[9];

	Minv[2] = Min[1]  * Min[6] * Min[15] - 
			  Min[1]  * Min[7] * Min[14] - 
			  Min[5]  * Min[2] * Min[15] + 
			  Min[5]  * Min[3] * Min[14] + 
			  Min[13] * Min[2] * Min[7] - 
			  Min[13] * Min[3] * Min[6];

	Minv[6] = -Min[0]  * Min[6] * Min[15] + 
			   Min[0]  * Min[7] * Min[14] + 
			   Min[4]  * Min[2] * Min[15] - 
			   Min[4]  * Min[3] * Min[14] - 
			   Min[12] * Min[2] * Min[7] + 
			   Min[12] * Min[3] * Min[6];

	Minv[10] = Min[0]  * Min[5] * Min[15] - 
			   Min[0]  * Min[7] * Min[13] - 
			   Min[4]  * Min[1] * Min[15] + 
			   Min[4]  * Min[3] * Min[13] + 
			   Min[12] * Min[1] * Min[7] - 
			   Min[12] * Min[3] * Min[5];

	Minv[14] = -Min[0]  * Min[5] * Min[14] + 
			    Min[0]  * Min[6] * Min[13] + 
			    Min[4]  * Min[1] * Min[14] - 
			    Min[4]  * Min[2] * Min[13] - 
			    Min[12] * Min[1] * Min[6] + 
			    Min[12] * Min[2] * Min[5];

	Minv[3] = -Min[1] * Min[6] * Min[11] + 
			   Min[1] * Min[7] * Min[10] + 
			   Min[5] * Min[2] * Min[11] - 
			   Min[5] * Min[3] * Min[10] - 
			   Min[9] * Min[2] * Min[7] + 
			   Min[9] * Min[3] * Min[6];

	Minv[7] = Min[0] * Min[6] * Min[11] - 
			  Min[0] * Min[7] * Min[10] - 
			  Min[4] * Min[2] * Min[11] + 
			  Min[4] * Min[3] * Min[10] + 
			  Min[8] * Min[2] * Min[7] - 
			  Min[8] * Min[3] * Min[6];

	Minv[11] = -Min[0] * Min[5] * Min[11] + 
			    Min[0] * Min[7] * Min[9] + 
			    Min[4] * Min[1] * Min[11] - 
			    Min[4] * Min[3] * Min[9] - 
			    Min[8] * Min[1] * Min[7] + 
			    Min[8] * Min[3] * Min[5];

	Minv[15] = Min[0] * Min[5] * Min[10] - 
			   Min[0] * Min[6] * Min[9] - 
			   Min[4] * Min[1] * Min[10] + 
			   Min[4] * Min[2] * Min[9] + 
			   Min[8] * Min[1] * Min[6] - 
			   Min[8] * Min[2] * Min[5];
	det = Min[0] * Minv[0] + Min[1] * Minv[4] + Min[2] * Minv[8] + Min[3] * Minv[12];
	if (det == 0) {
		return false;
	}
	det = 1.0f / det;
	for (i = 0; i < 16; i++) {
		Mout[i] = Minv[i] * det;
	}
	return true;
}

// make matrix clear
static inline void MatrixClear(float *matrix_ptr)
{
	memset(matrix_ptr,0,sizeof(float)*16);
	/*
	for (int i=0; i < 4; i++)
	{
		for (int j=0; j < 4; j++)
		{
			matrix_ptr[i*4+j] = 0.0;
		}
	}
	*/
}

// make matrix unit
static inline void MatrixIdentity(float *M)
{
	for (int i=0;i<4;i++) {
		for (int j=0;j<4;j++) {
			if (i==j) {
				M[i*4+j]=1.0f;
			}else{
				M[i*4+j]=0.0f;
			}
		}
	}
}

// 4x4 matrix multiply scalar
//		0	4	8	12
//	A(	1	5	9	13	)	x	B( x )
//		2	6	10	14
//		3	7	11	15
inline void MatrixMultiplyScalar(float *R,const float *M,const float s)
{
	for (int i=0; i<16; i++) {
		R[i]=M[i]*s;
	}
}

//4x4 matrix multiply vector4
//      x    0 4  8 12      x
// vout(y)=M(1 5  9 13) x v(y)
//      z    2 6 10 14      z
//      w    3 7 11 15      w
static inline void MatrixMultiplyVector4(float *vout,const float *M,const float *v)
{
	for (unsigned int i=0;i<4;i++) {
		vout[i]= M[0*4+i]*v[0]
				+M[1*4+i]*v[1]
				+M[2*4+i]*v[2]
				+M[3*4+i]*v[3];
	}
}
//後で実装があるのでプロトタイプ宣言
static inline void Vector4MultiplyMatrix(float *vout,const float *v,const float *mat);


// copy 4x4 matrix
static inline void MatrixCopy(float *Dest,const float *Src)
{
	memcpy(Dest,Src,sizeof(float)*16);
}

static inline void MatrixTranspose(float *M)
{
	int i, j;
	float Morg[16];
	memcpy(Morg,M,sizeof(float)*16);
	for (i=0;i<4;i++) {
		for (j=0;j<4;j++) {
			M[i*4+j]=Morg[j*4+i];
		}
	}
}
// transpose matrix by 2 4x4 matrix (Matrix after, Matrix before)
static inline void MatrixTranspose(float *Ma,const float *Mb)
{
	int i,j;
	float Morg[16];
	memcpy(Morg,Mb,sizeof(float)*16);
	for (i=0;i<4;i++) {
		for (j=0;j<4;j++) {
			Ma[i*4+j]=Morg[j*4+i];
		}
	}
}
//static inline float* MatrixTranspose2(const float *M)
//{
//	int i, j;
//	static float ret[16];
//	for (i=0;i<4;i++) {
//		for (j=0;j<4;j++) {
//			ret[i*4+j]=M[j*4+i];
//		}
//	}
//	return ret;
//}

//scaling
//    x 0 0 0 
// M( 0 y 0 0 )
//    0 0 z 0 
//    0 0 0 1
static inline void MatrixScale(float *M,const float x,const float y,const float z)
{
	memset(M,0,sizeof(float)*16);
	M[4*0+0]=x;
	M[4*1+0]=0.0f;
	M[4*2+0]=0.0f;
	M[4*3+0]=0.0f;
	M[4*0+1]=0.0f;
	M[4*1+1]=y;
	M[4*2+1]=0.0f;
	M[4*3+1]=0.0f;
	M[4*0+2]=0.0f;
	M[4*1+2]=0.0f;
	M[4*2+2]=z;
	M[4*3+2]=0.0f;
	M[4*0+3]=0.0f;
	M[4*1+3]=0.0f;
	M[4*2+3]=0.0f;
	M[4*3+3]=1.0f;
}

static inline void MatrixScale(float *M,const float s)
{
	MatrixScale(M,s,s,s);
}

//matrix order
// 0 4 8 12
// 1 5 9 13
// 2 6 10 14
// 3 7 11 15
// 4x4 matrix and translate xyz #
//    1 0 0 x
// M( 0 1 0 y )
//    0 0 1 z
//    0 0 0 1
static inline void MatrixTranslate(float *M,const float x,const float y,const float z)
{
	memset(M,0,sizeof(float)*16);
	// [4*GYOU+RETSU]
	M[4*0+0]=1.0f;
	M[4*0+1]=0.0f;
	M[4*0+2]=0.0f;
	M[4*0+3]=0.0f;
	M[4*1+0]=0.0f;
	M[4*1+1]=1.0f;
	M[4*1+3]=0.0f;
	M[4*1+2]=0.0f;
	M[4*2+0]=0.0f;
	M[4*2+1]=0.0f;
	M[4*2+2]=1.0f;
	M[4*2+3]=0.0f;
	M[4*3+0]=x;//x translate
	M[4*3+1]=y;//y translate
	M[4*3+2]=z;//z translate
	M[4*3+3]=1.0f;
}

//4x4 matrix with rotation xyz
static inline void MatrixEulerRotate(float *M,const float Rx,const float Ry,const float Rz)
{
	const float Sx=sinf(Rx);
	const float Cx=cosf(Rx);
	const float Sy=sinf(Ry);
	const float Cy=cosf(Ry);
	const float Sz=sinf(Rz);
	const float Cz=cosf(Rz);
	// OpenGL style (multiply matrix from left to Modelg_matView)
	M[4*0+0]=Cy*Cz;
	M[4*1+0]=Cz*Sy*Sx-Sz*Cx;
	M[4*2+0]=Cx*Sy*Cz+Sz*Sx;
	M[4*3+0]=0.0f;
	M[4*0+1]=Sz*Cy;
	M[4*1+1]=Sz*Sy*Sx+Cz*Cx;
	M[4*2+1]=Sz*Sy*Cx-Cz*Sx;
	M[4*3+1]=0.0f;
	M[4*0+2]=-Sy;
	M[4*1+2]=Cy*Sx;
	M[4*2+2]=Cy*Cx;
	M[4*3+2]=0.0f;
	M[4*0+3]=0.0f;
	M[4*1+3]=0.0f;
	M[4*2+3]=0.0f;
	M[4*3+3]=1.0f;
	MatrixNormalize(M);
}

//4x4 matrix multiply vector4
//                               0 4  8 12
// Vout(x y z w )=V(x y z w) x M(1 5  9 13)
//                               2 6 10 14
//                               3 7 11 15
static inline void Vector4MultiplyMatrix(float *vout,const float *v,const float *mat)
{
	for (size_t i=0;i<4;i++) {
		vout[i]=v[0]*mat[i*4+0]
			   +v[1]*mat[i*4+1]
			   +v[2]*mat[i*4+2]
			   +v[3]*mat[i*4+3];
	}
}

//----------------------------------------------------------------
//行列ユーティリティ群
//----------------------------------------------------------------
//@param	入力
//		マトリクスポインタ
//		プロジェクション行列情報
//@retval	出力
//		マトリクスポインタへ
//@par		注意
//			glFrustum関数的なもの。
void BuildFrustumMatrix(float *M,
						const float left,	const float right,//LRBTNF
						const float bottom,	const float top, 
						const float near,	const float far);

//画角を指定したいときに使うプロジェクション行列生成関数
//fovyは角度で入力。vectormathはradianなので気を付ける。
//プロジェクション行列を作る
//		cot(fovyRadians/2)/aspect		0					0							0
//			0						cot(fovyRadians/2)		0							0
//			0							0				(zFar+zNear)/(zNear-zFar)	2*zFar*zNear/(zNear-zFar)
//			0							0					-1							0
void BuildPerspectiveMatrix(float *m, const float fovy, const float aspect, const float znear, const float zfar);

//正射影行列を作る
//		2/(right-left)        0             0         -(right+left)/(right-left)
//			0          2/(top-bottom)       0         -(top+bottom)/(top-bottom)
//			0                 0       -2/(zFar-zNear) -(zFar+zNear)/(zFar-zNear)
//			0                 0             0                    1
//@param	入力
//		マトリクスポインタ
//		行列情報群
//@retval	出力
//		マトリクスポインタへ
//@par		注意
void BuildOrthoMatrix(float *M,
				const float left,const float right,//LRBTNF
				const float bottom,const float top,
				const float near,const float far);

//view 行列(look at行列)を作る。
// viewFrustumCullingEvalにおいて検証済み。
void BuildLookAtMatrix(float M[16],
						const float eyex,		const float eyey,		const float eyez,
						const float centerx,	const float centery,	const float centerz,
						const float upx=0.0f,	const float upy=1.0f,	const float upz=0.0f);//通常upx=0 upy=1 upz=0でよい。

//translate してからrotate。上記はrotateしてからtranslate
void BuildLookAtMatrixTR(float M[16],
						const float eyex,		const float eyey,		const float eyez,
						const float centerx,	const float centery,	const float centerz,
						const float upx=0.0f,	const float upy=1.0f,	const float upz=0.0f);//通常upx=0 upy=1 upz=0でよい。
// viewport 行列生成
//注意：画面中心が(0,0)になっている。必要に応じて左下や左上を(0,0)に調整すること
void BuildViewportMatrix(float M[16], const uint32_t width, const uint32_t height);

//フラスタムカリング
void GeneraeteFrustumCullingF4LRBTNF(float *F,		//4float x6 (24float ) (l r b t n f)
									const float *M, //matViewProj
									bool debug=false, FILE *fp=stdout);//16 float 
bool PointInFrustum(float x,float y,float z,
					const float *F, bool debug=false, FILE *fp=stdout); //4float x6 (24float ) (l r b t n f)
bool SphereInFrustum(float x,float y,float z,float r,
					const float *F, bool debug=false, FILE *fp=stdout); //4float x6 (24float ) (l r b t n f)

} //nsKg

#endif //__KGMATRIX_H__
