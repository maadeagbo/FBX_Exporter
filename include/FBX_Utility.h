#pragma once

#include <cstdio>
#include <cstdlib>
#include <map>
#include <functional>
#include <fbxsdk.h>
#include <DD_Container.h>

#define MAX_JOINTS ((uint8_t)-1)

typedef float vec4f[4];		//< 4 float vector
typedef float vec3f[3];		//< 3 float vector
typedef float vec2f[2];		//< 2 float vector
typedef uint32_t vec4u[4];	//< 4 unsigned int vector
typedef uint32_t vec3u[3];	//< 3 unsigned int vector

// Enum bitwise flags
template<typename Enum>
struct EnableBitMaskOperators
{
	static const bool enable = false;
};

// bitwise Or
template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator |(Enum lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum> (
		static_cast<underlying>(lhs) |
		static_cast<underlying>(rhs)
		);
}

// bitwise Or-Eq
template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator |=(Enum &lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	lhs = static_cast<Enum> (
		static_cast<underlying>(lhs) |
		static_cast<underlying>(rhs)
		);
	return lhs;
}

// bitwise And
template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator &(Enum lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum> (
		static_cast<underlying>(lhs) &
		static_cast<underlying>(rhs)
		);
}

inline void setVec3(const vec3f &source, vec3f &sink)
{
	sink[0] = source[0];
	sink[1] = source[1];
	sink[2] = source[2];
}

inline void setVec2(const vec3f &source, vec2f &sink)
{
	sink[0] = source[0];
	sink[1] = source[1];
}

static size_t getCharHash(const char* s)
{
	size_t h = 5381;
	int c;
	while( (c = *s++) )
		h = ((h << 5) + h) + c;
	return h;
}

template <const int T>
struct cbuff
{
	bool operator==(const cbuff &other) const
	{
		return hash == other.hash;
	}

	bool operator<(const cbuff &other) const
	{
		return hash < other.hash;
	}

	void set(const char* _str)
	{
		hash = getCharHash(_str);
		snprintf(str, T, "%s", _str);
	}

	const char* _str() const { return str; }
	size_t gethash() const { return hash; }
private:
	char str[T];
	size_t hash;
};

/// Triangle information (uses typedef arrays for least amount of padding)
struct VertPNTUV
{
	vec4u m_joint;
	vec4f m_jblend;
	vec3f m_pos;
	vec3f m_norm;
	vec3f m_tang;
	vec2f m_uv;
};

struct TriFBX
{
	vec3u		m_indices;
	size_t		m_mat_idx;
};

struct CtrlPnt
{
	CtrlPnt()
	{
		for (size_t i = 0; i < 4; i++) {
			m_joint[i] = 0;
			m_blend[i] = 0;
		}
	}
	vec3f		m_pos;
	uint16_t	m_joint[4];
	float		m_blend[4];
	// add joint blend weights to control point
	void addBWPair(const uint16_t idx, const float bw)
	{
		limit += 1;
		if( limit < 4 ) {
			m_joint[limit - 1] = idx;
			m_blend[limit - 1] = bw;
		}
	}
private:
	uint8_t limit = 0;
};

struct MeshFBX
{
	MeshFBX() {}
	MeshFBX(const char* name) { m_id.set(name); }

	cbuff<32>			m_id;
	dd_array<TriFBX>	m_triangles;
	dd_array<CtrlPnt>	m_ctrlpnts;
	dd_array<VertPNTUV> m_verts;

	bool operator==(const MeshFBX &other) const
	{
		return m_id.gethash() == other.m_id.gethash();
	}
};

struct JointFBX
{
	cbuff<32>	m_name;
	uint8_t		m_idx;
	uint8_t		m_parent;
	vec3f		m_lspos = { 0, 0, 0 };
	vec3f		m_lsrot = { 0, 0, 0 };
	vec3f		m_lsscl = { 1, 1, 1 };
};

struct SkelFbx
{
	JointFBX 	m_joints[MAX_JOINTS];
	uint8_t		m_numJoints = 0;
	vec3f		m_wspos = { 0, 0, 0 };
	vec3f		m_wsrot = { 0, 0, 0 };
	vec3f		m_wsscl = { 1, 1, 1 };
};

enum class MatType
{
	NONE = 0x0,
	DIFF = 0x1,
	NORMAL = 0x2,
	SPEC = 0x4,
	ROUGH = 0x8,
	METAL = 0x10,
	EMIT = 0x20,
	AO = 0x40//,
	// ACOL = 0x80,
	// DCOL = 0x100,
	// SCOL = 0x200,
	// ECOL = 0x400,
	// TFAC = 0x800,
	// RFAC = 0x1000,
	// SFAC = 0x2000
};

// enable bitwise operators
template<>
struct EnableBitMaskOperators<MatType> { static const bool enable = true; };

struct MatFBX
{
	MatFBX() {}
	MatFBX(const char* name) { m_id.set(name); }

	cbuff<128>	m_diffmap;
	cbuff<128>	m_normmap;
	cbuff<128>	m_specmap;
	cbuff<128>	m_roughmap;
	cbuff<128>	m_emitmap;
	cbuff<128>	m_aomap;
	cbuff<128>	m_metalmap;
	cbuff<64>	m_id;
	vec3f		m_ambient;
	vec3f		m_diffuse;
	vec3f		m_specular;
	vec3f		m_emmisive;
	float		m_transfactor;
	float		m_reflectfactor;
	float		m_specfactor;
	MatType	m_textypes = MatType::NONE;

	bool operator==(const MatFBX &other) const
	{
		return m_id.gethash() == other.m_id.gethash();
	}
};

struct AnimSample
{
	vec3f		rot = {0, 0, 0};
	//vec3f		pos;
	//float		scal;
};

struct PoseSample
{
	dd_array<AnimSample> pose;
};

struct AnimClipFBX
{
	AnimClipFBX() {}
	AnimClipFBX(const char* name) { m_id.set(name); } 

	cbuff<64> 	m_id;
	float		m_framerate;
	std::map<float, PoseSample> m_clip;
};

struct AssetFBX
{
	struct EboMesh { dd_array<vec3u> indices; };

	cbuff<32>			m_id;
	dd_array<MatFBX> 	m_matbin;
	dd_array<VertPNTUV> m_verts;
	dd_array<EboMesh> 	m_ebos;
	SkelFbx				m_skeleton;
	dd_array<AnimClipFBX> m_clips;

	void addMesh(MeshFBX& _mesh, dd_array<size_t> &ebo_data);
	void exportMesh();
	void exportSkeleton();
};
