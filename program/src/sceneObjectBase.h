#ifndef __OBJECT_H__
#define __OBJECT_H__
#include "kgmacro.h"
#include "kgstdint.h"
#include <cmath>
#include "types.h"

class SceneObjectBase;//このクラスの存在を知らせる。
class Material;

class Intersection
{
protected:
private:
public:
	//bool		m_hit;
	float		m_distance;		//衝突時のみ、
								//交差点までの距離(何回も反射するときには精度が不足する場合がある。)
	Vec			m_position;		//ワールド座標位置(xyz)
	Vec			m_normal;		//ワールド座標法線(nx ny nz)
	uint16_t	m_texId;		//テクスチャID
	Vec			m_texCoord;		//テクスチャ座標(s t r)
	Color		m_color;		//色
	Color		m_emission;	
	//Vec rayOrigin;	//レイの起点
	//Vec rayDirection;	//レイの方向
	matType_t		m_matType;
	SceneObjectBase		*m_object;
	Intersection(void) : m_distance(-1.0f){ }
	inline void setObject(SceneObjectBase *o){ m_object=o; }
};

class Material
{
protected:
private:
public:
	Color			m_color;
	Color			m_emission;
	float			m_refraction;
	matType_t		m_matType;
	uint16_t		m_texId;
	
	Material(): m_color(Color(0,0,0)), 
				m_emission(Color(0,0,0)), 
				m_refraction(1.0f),
				m_matType( kMatTypeDiffuse ){;}

	virtual ~Material(){;}
};


class SceneObjectBase
{
private:
	Material	*m_material_ptr;
public:
	//typedef SceneObjectBase self;
	virtual ~SceneObjectBase(){}
	virtual bool intersect(Intersection *info, const Ray *r)=0;
	//virtual Bound bound()const=0; //BVH用
	inline Material *getMaterial(void) const { return m_material_ptr; }
	void setMaterial( Material *mat ) { m_material_ptr = mat; }
};

//----------------------------------------------------------------
//オブジェクト
//----------------------------------------------------------------


class Triangle : public SceneObjectBase {
private:
public:
	Vec m_pos0, m_pos1, m_pos2;
	Vec m_centerPos;
	Vec m_normal;
	Vec m_texCoord0, m_texCoord1, m_texCoord2;
	Material m_mat;
	Triangle( void ){ m_mat.m_refraction = 1.5f; }
	virtual ~Triangle(void){ ; }
	Triangle(	const Vec &pos0, 
				const Vec &pos1, 
				const Vec &pos2, 
				const Color &emi, 
				const Color &col, 
				const matType_t ref_type, 
				const uint16_t tid, 
				const Vec &t0, 
				const Vec &t1, 
				const Vec &t2 ) :
		m_pos0(pos0),m_pos1(pos1),m_pos2(pos2),m_texCoord0(t0),m_texCoord1(t1),m_texCoord2(t2){
		m_centerPos.x=(m_pos0.x + m_pos1.x + m_pos2.x)/3.0f;
		m_centerPos.y=(m_pos0.y + m_pos1.y + m_pos2.y)/3.0f;
		m_centerPos.z=(m_pos0.z + m_pos1.z + m_pos2.z)/3.0f;
		Vec en1=m_pos1 - m_pos0;
		Vec en2=m_pos2 - m_pos0;
		m_normal=Normalize( Cross(en1, en2) );
		m_mat.m_color = col;
		m_mat.m_emission = emi;
		m_mat.m_matType = ref_type;
		m_mat.m_texId=tid;
	}
	// 入力のrayに対する交差点までの距離を返す。交差しなかったら0を返す。
	bool intersect(Intersection *info, const Ray *ray)
	{
		const float eps=0.000001f;
		//Möller-Trumbore 法 (高速な交差判定)
		//http://www.cs.virginia.edu/~gfx/Courses/2003/ImageSynthesis/papers/Acceleration/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf
		float u,v,t;//この値がfloatになると精度が悪くなるため注意。重心座標と距離。
		Vec edge1 = m_pos1 - m_pos0;
		Vec edge2 = m_pos2 - m_pos0;
		Vec pvec = Cross(ray->dir, edge2);
		float det = Dot(edge1, pvec);
		
		//参考：高速化するためにcullingするアルゴリズムも論文にある。

		//non-culling algorithm
		//if(det == 0){
		if(det>-eps && det<eps){
			return false;
		}
		float invDet=1.0f/det;
		Vec tvec=ray->org - m_pos0;
		u=Dot(tvec, pvec)*invDet;
		if(u<0.0f || u>1.0f){
			return false;
		}

		Vec qvec=Cross(tvec, edge1);
		v=Dot(ray->dir, qvec)*invDet;
		if(v<0.0f || u+v>1.0f){
			return false;
		}
		t=Dot(edge2, qvec)*invDet;

		//パラメタ設定
		//info->m_hit=true;
		info->m_distance	= t;
		info->m_position	= ray->org + t*ray->dir;//交差した点
		info->m_normal		= m_normal;//三角形の面法線そのまま渡せばよい。
		info->m_texCoord	= m_texCoord0*(1-u-v)+m_texCoord1*u+m_texCoord2*v;
		info->m_color		= m_mat.m_color;
		info->m_emission	= m_mat.m_emission;
		info->m_matType		= m_mat.m_matType;
		info->m_texId		= m_mat.m_texId;
		setMaterial( &m_mat );
		info->setObject( this );
		return true;
	}
};
class Sphere : public SceneObjectBase {
protected:
private:
public:
	Material m_mat;
	float	m_radius;	//半径
	Vec		m_position;	//中心位置

	Sphere(void){ m_mat.m_refraction=1.5f; }
	virtual ~Sphere(void){ ; }
	Sphere( const float radius, 
			const Vec &position, 
			const Color &emission, 
			const Color &color, 
			const matType_t matType) :
		m_radius(radius),
		m_position(position){
		m_mat.m_color		= color;
		m_mat.m_emission	= emission;
		m_mat.m_matType		= matType;
		m_mat.m_refraction	= 1.5f;
		m_mat.m_texId		= kTexIdNone;//no tex
	}
	// 入力のrayに対する交差点までの距離を返す。交差しなかったら0を返す。


	// 入力のrayに対する交差点までの距離を返す。交差しなかったら0を返す。
	bool intersect(Intersection *info, const Ray *ray)
	{
		Vec o_p = m_position - ray->org;
		const float b = Dot(o_p, ray->dir);
		const float det = b * b - Dot(o_p, o_p) + m_radius * m_radius;
		if (det >= 0.0) {
			const float sqrt_det = sqrt(det);
			const float t1 = b - sqrt_det;
			const float t2 = b + sqrt_det;
			if (t1 > EPS){
				info->m_distance=t1;
			}else if(t2 > EPS){
				info->m_distance=t2;
			}
			info->m_position=ray->org + info->m_distance*ray->dir;//交差した点
			info->m_normal=Normalize(info->m_position - m_position); // 球の法線計算式
			info->m_texCoord.x=abs(info->m_normal.x);// 法線を流用。適当な値になる
			info->m_texCoord.y=abs(info->m_normal.y);
			info->m_texCoord.z=abs(info->m_normal.z); 
			info->m_texId=m_mat.m_texId;
			setMaterial( &m_mat );
			info->setObject( this );
			return true;
		}
		info->m_distance=0.0f;
		return false;
	}
};

#endif //__OBJECT_H__