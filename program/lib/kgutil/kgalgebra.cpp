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
//式の導き方はここに　http://marina.sys.wakayama-u.ac.jp/~tokoi/?date=20090829
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
//		2/(right-left)		0				0			-(right+left)/(right-left)
//			0			2/(top-bottom)		0			-(top+bottom)/(top-bottom)
//			0				0			-2/(far-near)	-(far+near)/(far-near)
//			0				0				0					1
//やはりこれを転置して返す。（最終形）
//		2/(right-left)						0						0						0
//			0						2/(top-bottom)					0						0
//			0								0					-2/(far-near)				0
//		-(right+left)/(right-left)	-(top+bottom)/(top-bottom)	-(far+near)/(far-near)		1
//@param	入力
//		マトリクスポインタ
//		行列情報群
//@retval	出力
//		マトリクスポインタへ
//@par		注意
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

//OpenGL定義
//
//h25-12-12 Rotate してからTranslateする。
//	vector math 方式 はOpenGLと異なる。これが正しい手法、というよりは定義次第なので一つ決めればよい。
//h25-11-13 期待通りの値の様。
//	・・・と思ったら間違えていたのを木村さんに指摘してもらう。治ったからいいが、もうちょっとやはり正しい動きをイメージしないと。
//h26-08-05
//	proj matrix の向きと合わせる。（transpose追加）
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
	/*
    float forward[3], side[3], up[3];
    //GLfloat m[4][4];
    forward[0] = centerx - eyex;
    forward[1] = centery - eyey;
    forward[2] = centerz - eyez;
    up[0] = upx;
    up[1] = upy;
    up[2] = upz;
	Vector3Normalize(forward);

    // Side = forward x up 
	Vector3Cross(side, forward, up);
    Vector3Normalize(side);

    // Recompute up as: up = side x forward 
	Vector3Cross(up, side, forward);

	MatrixIdentity(M);
    //__gluMakeIdentityf(&m[0][0]);
    M[0*4 + 0] = side[0];
    M[0*4 + 1] = side[1];
    M[0*4 + 2] = side[2];

    M[1*4 + 0] = up[0];
    M[1*4 + 1] = up[1];
    M[1*4 + 2] = up[2];

    M[2*4 + 0] = -forward[0];
    M[2*4 + 1] = -forward[1];
    M[2*4 + 2] = -forward[2];
	
    M[3*4 + 3] = 1.0f;
	//http://www.opengl.org/sdk/docs/man2/xhtml/gluLookAt.xml
	MatrixTranspose(M);//ここまでは転置している行列を作成した。transposeして適した形にする。

	//最後に移動成分をつける。
	M[3*4 + 0]=-eyex;//x translate
	M[3*4 + 1]=-eyey;//y translate
	M[3*4 + 2]=-eyez;//z translate


	//Vector3 At = new Vector3(10, 10, 10);//注視点
	//Vector3 Eye = new Vector3(0, 0, 0);//視点
	//Vector3 zaxis = Vector3.Normalize(Eye-At);//左手系なら逆(Eye-At)にする //forward
	//Vector3 yaxis = Vector3.Cross(zaxis, xaxis); //up
	//Vector3 xaxis = Vector3.Normalize(Vector3.Cross(new Vector3(0, 1, 0), zaxis)); //side

	//Matrix これが欲しかったMatrix =new Matrix(
	//xaxis.X ,yaxis.X ,zaxis.X ,0,
	//xaxis.Y ,yaxis.Y ,zaxis.Y ,0,
	//xaxis.Z ,yaxis.Z ,zaxis.Z ,0,
	//-Vector3.Dot(xaxis,Eye) ,-Vector3.Dot(yaxis,Eye) ,-Vector3.Dot(zaxis,Eye) , 1
	//);
	*/
}

//translate -> rotate
//移動成分生成に回転を加味しない。
//	使わないかもしれない
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

//http://www.gamedev.net/topic/531748-the-viewport-matrix-in-opengl/
//http://rudora7.blog81.fc2.com/blog-entry-415.html
void BuildViewportMatrix(	float M[16],
							const uint32_t width, 
							const uint32_t height)
{
	MatrixIdentity( M );
	M[ 0*4 + 0 ] = +0.5f * ( float )width;
	M[ 1*4 + 1 ] = -0.5f * ( float )height;
	M[ 0*4 + 3 ] = +0.5f * ( float )width;
	M[ 1*4 + 3 ] = +0.5f * ( float )height;

	//directX向けの並び（row majar）に作成したため、
	//このライブラリで統一しているcolumn majarに戻す。
	//MatrixTranspose( M );
	// h26-08-05 やっぱりおかしくなってしまったのでそのまま。うーん。行列とベクタの順序を正しくしているのでこれでいいはず。

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
//点Pと線(AB)の距離
//Point P, line AtoB
//http://www.sousakuba.com/Programming/gs_dot_line_distance.html
float GetDistancePointAndLine2D(float P[2], float A[2], float B[2] )
{
	float AB[2],AP[2];
	Vector2Sub(AB,B,A);
	Vector2Sub(AP,P,A);
	float tmp;
	Vector2Cross( &tmp, AB, AP );
	//ベクトルAB、APの外積の絶対値が平行四辺形Dの面積になる
	float D=abs( tmp );
	float L=GetDistancePointAndPoint2D( A, B );	//AB間の距離
	float H = D / L;
	return H;
}

//Point P, line AtoB
float GetDistancePointAndLine3D(float P[3], float A[3], float B[3] )
{
	float AB[3],AP[3];
	Vector3Sub(AB,B,A);
	Vector3Sub(AP,P,A);
	//AB、APを外積して求められたベクトルの長さが、平行四辺形Dの面積になる
	float tmp[3];
	Vector3Cross(tmp,AB,AP);
	float D = Vector3GetLength( tmp );
	//AB間の距離
	float L = GetDistancePointAndPoint3D( A, B );	//ABの長さ
	float H = D / L;
	return H;
}


int CheckHitPointAndTriangle2D( float A[2], float B[2], float C[2], float P[2] )
{
    //線上は外とみなします。
    //ABCが三角形かどうかのチェックは省略...
    float AB[2];
    float BP[2];
    float BC[2];
    float CP[2];
    float CA[2];
    float AP[2];

    Vector2Sub(AB,B,A);
    Vector2Sub(BP,P,B);
    Vector2Sub(BC,C,B);
    Vector2Sub(CP,P,C);
    Vector2Sub(CA,A,C);
    Vector2Sub(AP,P,A);

    //外積    Z成分だけ計算すればよいです
    //float c1 = AB.x * BP.y - AB.y * BP.x;
    //float c2 = BC.x * CP.y - BC.y * CP.x;
    //float c3 = CA.x * AP.y - CA.y * AP.x;
	float c1,c2,c3;
	Vector2Cross(&c1,AB,BP);
	Vector2Cross(&c2,BC,CP);
	Vector2Cross(&c3,CA,AP);

    if ( ( c1 > 0 && c2 > 0 && c3 > 0 ) || ( c1 < 0 && c2 < 0 && c3 < 0 ) ) {
        //三角形の内側に点がある
        return 0;
    }

    //三角形の外側に点がある
    return 1;

}
int CheckHitPointAndTriangle3D( float A[3], float B[3], float C[3], float P[3] )
{
    //点と三角形は同一平面上にあるものとしています。同一平面上に無い場合は正しい結果になりません
    //線上は外とみなします。
    //ABCが三角形かどうかのチェックは省略...
    
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


void GetNearPosOnLine2D(float Result[2], const float P[2], const float A[2], const float B[2] )
{
	float AB[2],AP[2];//ベクトルAB AP
	Vector2Sub(AB,B,A);
	Vector2Sub(AP,P,A);
	//ABの単位ベクトルを計算
	float nAB[2];
	Vector2Normalize(nAB,AB);
	//Aから線上最近点までの距離（ABベクトルの後ろにあるときはマイナス値）
	float dist_AX = Vector2Dot( nAB, AP );
	//線上最近点
	float ret[2];
	ret[0] = A[0] + ( nAB[0] * dist_AX );
	ret[1] = A[1] + ( nAB[1] * dist_AX );
	Vector2Copy(Result,ret);
}
void GetNearPosOnLine3D(float Result[3], const float P[3], const float A[3], const float B[3] )
{
	float AB[3],AP[3];//ベクトルAB AP
	Vector2Sub(AB,B,A);
	Vector2Sub(AP,P,A);
	//ABの単位ベクトルを計算
	float nAB[3];
	Vector3Normalize(nAB,AB);
	//Aから線上最近点までの距離（ABベクトルの後ろにあるときはマイナス値）
	float dist_AX = Vector3Dot( nAB, AP );
	//線上最近点
	float ret[3];
	ret[0] = A[0] + ( nAB[0]*dist_AX );
	ret[1] = A[1] + ( nAB[1]*dist_AX );
	ret[2] = A[2] + ( nAB[2]*dist_AX );
	Vector3Copy(Result,ret);
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

//http://www.sousakuba.com/Programming/gs_near_pos_on_plane.html
//平面上の最近点を求める　その１( P=平面上の点 N=平面の法線 )
void GetNearPosOnPlane(float R[3], const float A[3], const float P[3], const float N[3])
{
	//PAベクトル(A-P)
	float PA[3];
	Vector3Sub(PA,A,P);
	//法線NとPAを内積
	//法線の順方向に点Aがあればd > 0、 逆方向だとd < 0
	float d=Vector3Dot(N,PA);
	//内積値から平面上の最近点を求める
	float ret[3];
	ret[0] = A[0] - (N[0]*d);
    ret[1] = A[1] - (N[1]*d);
    ret[2] = A[2] - (N[2]*d);
	Vector3Copy(R,ret);
}

//平面上の最近点を求める　その２(平面方程式 ax+by+cz+d=0 を使う場合 )
void GetNearPosOnPlane(float R[3], const float A[3], const float plane[4])
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
	GetNearPosOnPlane( R,A,P,N );
}


//線分ABと平面の交点を計算する
//http://www.sousakuba.com/Programming/gs_plane_line_intersect.html
bool CheckIntersectPlaneAndLine(
	float out[3], //戻り値　交点が見つかれば格納される
	const float A[3],   //線分始点
	const float B[3],   //線分終点
	const float plane[4] ) //平面
{	
	//平面上の点P
	float P[3];
	P[0]=plane[0]*plane[3];
	P[1]=plane[1]*plane[3];
	P[2]=plane[2]*plane[3];
	//PA PBベクトル
	float PA[3],PB[3];
	Vector3Sub(PA,P,A);
	Vector3Sub(PB,P,B);
	//PA PBそれぞれを平面法線と内積
	float dot_PA = PA[0]*plane[0] + PA[1]*plane[1] + PA[2]*plane[2];
	float dot_PB = PB[0]*plane[0] + PB[1]*plane[1] + PB[2]*plane[2];
	//これは線端が平面上にあった時の計算の誤差を吸収。スケール・制度により調整が必要かもしれない。
	const float eps = 1e-6f;
	if ( abs(dot_PA) < eps) { dot_PA=0.0f; }	
	if ( abs(dot_PB) < eps) { dot_PB=0.0f; }
	//交差判定
	if ( dot_PA == 0.0 && dot_PB == 0.0 ) {
		//両端が平面上にあり、交点を計算できない。
		return false;
	} else
	if ( ( dot_PA >= 0.0 && dot_PB <= 0.0 ) ||
	     ( dot_PA <= 0.0 && dot_PB >= 0.0 ) ) {
		 //内積の片方がプラスで片方がマイナスなので、交差している
	} else {
		//交差していない
		return false;
	}
	//以下、交点を求める 
	float AB[3];
	Vector3Sub(AB,B,A);
	//交点とAの距離 : 交点とBの距離 = dot_PA : dot_PB
	float ratio=abs(dot_PA)/( abs(dot_PA) + abs(dot_PB) );
	out[0] = A[0] + ( AB[0]*ratio );
	out[1] = A[1] + ( AB[1]*ratio );
	out[2] = A[2] + ( AB[2]*ratio );
	return true;
}


} // nsKg
