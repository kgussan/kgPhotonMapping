#ifndef __TYPES_H__
#define __TYPES_H__


const float PI = 3.14159265358979323846f;
const float INF = 1e20f;
const float EPS = 1e-6f;
const float REFTRACTION_AIR=1.0f;

// *** データ構造 ***
class Vec {
public:
	float x, y, z;
	Vec(	const float x_ = 0,
			const float y_ = 0, 
			const float z_ = 0) : 
				x(x_), 
				y(y_), 
				z(z_) {}
	inline Vec operator+(const Vec &b) const {
		return Vec(x + b.x, y + b.y, z + b.z);
	}
	inline Vec operator-(const Vec &b) const {
		return Vec(x - b.x, y - b.y, z - b.z);
	}
	inline Vec operator*(const float b) const {
		return Vec(x * b, y * b, z * b);
	}
	inline Vec operator/(const float b) const {
		return Vec(x / b, y / b, z / b);
	}
	inline const float LengthSquared() const {
		return x*x + y*y + z*z; 
	}
	inline const float Length() const { 
		return sqrt(LengthSquared()); 
	}
};
inline Vec operator*( float f, const Vec &v ) { return v * f; }
inline Vec Normalize( const Vec &v ) { return v / v.Length(); }

// 要素ごとの積をとる
inline const Vec Multiply(const Vec &v1, const Vec &v2)
{
	return Vec(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z);
}

inline const float Dot(const Vec &v1, const Vec &v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
inline const Vec Cross(const Vec &v1, const Vec &v2)
{
	return Vec((v1.y * v2.z) - (v1.z * v2.y), (v1.z * v2.x) - (v1.x * v2.z), (v1.x * v2.y) - (v1.y * v2.x));
}

typedef Vec Color;
const Color BackgroundColor(0.0, 0.0, 0.0);

struct Ray {
	Vec org, dir;
	Ray(const Vec org_, const Vec &dir_) : org(org_), dir(dir_) {}
};

enum matType_t {
	kMatTypeDiffuse, // 完全拡散面。いわゆるLambertian面。
	kMatTypeReflection, // 理想的な鏡面。
	kMatTypeRefraction, // 理想的なガラス的物質。
	kMatTypeGlossy,//光沢（GLOSSY）をできれば入れる。
};


struct photon_t {
	float		x, y, z;	//位置
	float		r, g, b;	//光りの強さ（flux）つまり色
	uint32_t	padding[2];
	//float	incident[3];	//入射角 GLOSSY反射で使う。
};

struct vertex_t {
	float x, y, z;
	float nx, ny, nz;
	float u, v;
};

struct triangle_t {
	float		x, y, z;	// center
	vertex_t	v0, v1, v2;
	uint32_t	matType; // 0:diff, 1:spec_refl, 2:spec_refr, 3:glossy
	uint32_t	texId;
	//uint32_t	padding[7]; // 8 byte alignment ? 
};
struct texel_t {
	uint8_t r, g, b, a;
};
struct pixel_t {
	float r, g, b, a;
};


class Photon {
public:
	Vec		position;	//位置
	Color	power;		//光りの強さ（flux）つまり色
	Vec		incident;	//入射角 GLOSSY反射で使う。
	uint8_t bound;
	uint8_t bound_diffuse;	//for caustics
	uint8_t bound_specular;	//for caustics
	Photon(	const Vec& position_, 
			const Color& power_, 
			const Vec& incident_, 
			const uint32_t &b_, 
			const uint32_t &bd_, 
			const uint32_t &bs_ ) :
		position( position_ ), 
		power( power_ ), 
		incident( incident_ ), 
		bound( b_ ), 
		bound_diffuse( bd_ ), 
		bound_specular( bs_ ) {}
};

enum texid_t{
	kTexIdNone=0,
	kTexIdKuma,
	kTexIdTeapot,
	kTexIdTestTex,
	kTexIdOrange,
	kTexIdMax,
};

struct frameState_t{
	uint32_t width;
	uint32_t height;
	uint32_t elemNum;
	float cam_near; //camera
	float cam_far;	

};

#endif //__TYPES_H__