#pragma once
#define M_PI		3.14159265358979323846	
#include <Windows.h>
#include <math.h>
#include "Driver.hpp"
#define RAD2DEG(x) ((x) * (180.0 / M_PI))
#define DEG2RAD(x) ((x) * (M_PI / 180.0f))
#define sqr(n) ((n)*(n))
struct Matrix4x4
{
	float _11, _12, _13, _14;
	float _21, _22, _23, _24;
	float _31, _32, _33, _34;
	float _41, _42, _43, _44;
};
#define Assert( _exp ) ((void)0)
struct matrix3x4_t
{
	matrix3x4_t() {}
	matrix3x4_t(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23)
	{
		m_flMatVal[0][0] = m00;	m_flMatVal[0][1] = m01; m_flMatVal[0][2] = m02; m_flMatVal[0][3] = m03;
		m_flMatVal[1][0] = m10;	m_flMatVal[1][1] = m11; m_flMatVal[1][2] = m12; m_flMatVal[1][3] = m13;
		m_flMatVal[2][0] = m20;	m_flMatVal[2][1] = m21; m_flMatVal[2][2] = m22; m_flMatVal[2][3] = m23;
	}

	float* operator[](int i) { Assert((i >= 0) && (i < 3)); return m_flMatVal[i]; }
	const float* operator[](int i) const { Assert((i >= 0) && (i < 3)); return m_flMatVal[i]; }
	float* Base() { return &m_flMatVal[0][0]; }
	const float* Base() const { return &m_flMatVal[0][0]; }

	float m_flMatVal[3][4];
};
inline float DegreesToRadians(float degrees) {
	return degrees * (M_PI / 180);
}
class QAngle
{
public:
	QAngle(void)
	{
		Init();
	}
	QAngle(float X, float Y, float Z)
	{
		Init(X, Y, Z);
	}
	QAngle(const float* clr)
	{
		Init(clr[0], clr[1], clr[2]);
	}

	void Init(float ix = 0.0f, float iy = 0.0f, float iz = 0.0f)
	{
		pitch = ix;
		yaw = iy;
		roll = iz;
	}

	float operator[](int i) const
	{
		return ((float*)this)[i];
	}
	float& operator[](int i)
	{
		return ((float*)this)[i];
	}

	QAngle& operator+=(const QAngle& v)
	{
		pitch += v.pitch; yaw += v.yaw; roll += v.roll;
		return *this;
	}
	QAngle& operator-=(const QAngle& v)
	{
		pitch -= v.pitch; yaw -= v.yaw; roll -= v.roll;
		return *this;
	}
	QAngle& operator*=(float fl)
	{
		pitch *= fl;
		yaw *= fl;
		roll *= fl;
		return *this;
	}
	QAngle& operator*=(const QAngle& v)
	{
		pitch *= v.pitch;
		yaw *= v.yaw;
		roll *= v.roll;
		return *this;
	}
	QAngle& operator/=(const QAngle& v)
	{
		pitch /= v.pitch;
		yaw /= v.yaw;
		roll /= v.roll;
		return *this;
	}
	QAngle& operator+=(float fl)
	{
		pitch += fl;
		yaw += fl;
		roll += fl;
		return *this;
	}
	QAngle& operator/=(float fl)
	{
		pitch /= fl;
		yaw /= fl;
		roll /= fl;
		return *this;
	}
	QAngle& operator-=(float fl)
	{
		pitch -= fl;
		yaw -= fl;
		roll -= fl;
		return *this;
	}

	QAngle& operator=(const QAngle& vOther)
	{
		pitch = vOther.pitch; yaw = vOther.yaw; roll = vOther.roll;
		return *this;
	}

	QAngle operator-(void) const
	{
		return QAngle(-pitch, -yaw, -roll);
	}
	QAngle operator+(const QAngle& v) const
	{
		return QAngle(pitch + v.pitch, yaw + v.yaw, roll + v.roll);
	}
	QAngle operator-(const QAngle& v) const
	{
		return QAngle(pitch - v.pitch, yaw - v.yaw, roll - v.roll);
	}
	QAngle operator*(float fl) const
	{
		return QAngle(pitch * fl, yaw * fl, roll * fl);
	}
	QAngle operator*(const QAngle& v) const
	{
		return QAngle(pitch * v.pitch, yaw * v.yaw, roll * v.roll);
	}
	QAngle operator/(float fl) const
	{
		return QAngle(pitch / fl, yaw / fl, roll / fl);
	}
	QAngle operator/(const QAngle& v) const
	{
		return QAngle(pitch / v.pitch, yaw / v.yaw, roll / v.roll);
	}

	float Length() const
	{
		return sqrt(pitch * pitch + yaw * yaw + roll * roll);
	}
	float LengthSqr(void) const
	{
		return (pitch * pitch + yaw * yaw + roll * roll);
	}
	bool IsZero(float tolerance = 0.01f) const
	{
		return (pitch > -tolerance && pitch < tolerance &&
			yaw > -tolerance && yaw < tolerance &&
			roll > -tolerance && roll < tolerance);
	}
	float pitch;
	float yaw;
	float roll;
};


class Vector3
{
public:

	Vector3() : x(0.f), y(0.f), z(0.f)
	{

	}

	Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z)
	{

	}
	~Vector3()
	{

	}

	float x;
	float y;
	float z;

	inline float Dot(Vector3 v)
	{
		return x * v.x + y * v.y + z * v.z;
	}

	inline float Distance(Vector3 v)
	{
		return float(sqrtf(powf(v.x - x, 2.0) + powf(v.y - y, 2.0) + powf(v.z - z, 2.0)));
	}
	inline double Length() {
		return sqrt(x * x + y * y + z * z);
	}
	inline bool Empty() const
	{
		if (!x && !y && !z)
			return true;
		if (x != x || y != y || z != z) // NaN check
			return true;
		else
			return false;
	}
	inline void Normalize()
	{
		while (x > 89.0f)
			x -= 180.f;

		while (x < -89.0f)
			x += 180.f;

		while (y > 180.f)
			y -= 360.f;

		while (y < -180.f)
			y += 360.f;
	}

	inline float DistTo(Vector3 ape)
	{
		return (*this - ape).Length();
	}
	inline float distance(Vector3 vec)
	{
		return sqrt(
			sqr(vec.x - x) +
			sqr(vec.y - y)
		);
	}
	inline Vector3 operator+(Vector3 v)
	{
		return Vector3(x + v.x, y + v.y, z + v.z);
	}

	inline Vector3 operator-(Vector3 v)
	{
		return Vector3(x - v.x, y - v.y, z - v.z);
	}
	bool isEqual(Vector3 v)
	{
		if (this->x == v.x && this->y == v.y && this->z == v.z)
			return true;
		else
			return false;
	}
	static float sqrtf(float number)
	{
		long i;
		float x2, y;
		const float threehalfs = 1.5F;

		x2 = number * 0.5F;
		y = number;
		i = *(long*)&y;
		i = 0x5f3759df - (i >> 1);
		y = *(float*)&i;
		y = y * (threehalfs - (x2 * y * y));
		y = y * (threehalfs - (x2 * y * y));

		return 1 / y;
	}

	__forceinline bool Zero() const {
		return (x > -0.1f && x < 0.1f && y > -0.1f && y < 0.1f && z > -0.1f && z < 0.1f);
	}
	inline Vector3 operator*(float flNum) { return Vector3(x * flNum, y * flNum, z * flNum); }

};
inline static void Normalize(float& Yaw, float& Pitch) {
	while (Yaw > 180.f) Yaw -= 360.f;
	while (Yaw < -180.f) Yaw += 360.f;
	if (Pitch > 89.f) Pitch = 89.f;
	if (Pitch < -89.f) Pitch = -89.f;
}
class Vector2
{
public:
	Vector2() : x(0.f), y(0.f)
	{

	}

	Vector2(float _x, float _y) : x(_x), y(_y)
	{

	}
	~Vector2()
	{

	}
	bool Empty() const
	{
		if (x < 0.1f || y < 0.1f)
			return true;
		if (x != x || y != y) // NaN check
			return true;
		else
			return false;
	}
	float x;
	float y;
	inline Vector2 operator+(Vector2 i) {
		return { x + i.x, y + i.y };
	}
	inline Vector2 operator*(float i) {
		return { x * i, y * i };
	}
	inline Vector2 operator-(Vector2 v) {
		return { x - v.x, y - v.y };
	}
	inline Vector2 flip() {
		return { y, x };
	}

};
struct FQuat
{
	float x;
	float y;
	float z;
	float w;
};

inline float GetCrossDistance(float x1, float y1, float z1, float x2, float y2, float z2) {
	return sqrt(sqr((x2 - x1)) + sqr((y2 - y1)));
}
inline inline float calcDist(const Vector3 p1, const Vector3 p2)
{
	float x = p1.x - p2.x;
	float y = p1.y - p2.y;
	float z = p1.z - p2.z;
	return sqrt(x * x + y * y + z * z);
}
inline const float to_degree(float radian)
{
	return radian * 180.f / 3.141592f;
}
inline const float to_radian(float degree)
{
	return degree * 3.141592f / 180.f;
}
inline const float Length(Vector3 v) {
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}
inline Vector3 CalcAngle(const Vector3& LocalPos, const Vector3& EnemyPos)
{
	Vector3 dir;
	dir.x = LocalPos.x - EnemyPos.x;
	dir.y = LocalPos.y - EnemyPos.y;
	dir.z = LocalPos.z - EnemyPos.z;
	float Pitch = to_degree(asin(dir.y / Length(dir)));
	float Yaw = to_degree(-atan2(dir.x, -dir.z));
	Vector3 Return;
	Return.x = Pitch;
	Return.y = Yaw;
	return Return;
}
struct bone_t
{
	BYTE pad[0xCC];
	float x;
	BYTE pad2[0xC];
	float y;
	BYTE pad3[0xC];
	float z;
};

inline Vector3 VAR_Adds(Vector3 src, Vector3 dst)
{
	Vector3 diff;
	diff.x = src.x + dst.x;
	diff.y = src.y + dst.y;
	diff.z = src.z + dst.z;
	return diff;
}

inline Vector3 VAR_toAngless(Vector3 const& v)
{
	double R2D = 57.2957795130823;
	Vector3 angles;
	angles.z = 0.0;
	angles.x = R2D * asin(v.z);
	angles.y = R2D * atan2(v.y, v.x);
	return angles;
}

inline Vector3 VAR_Subtracts(Vector3 src, Vector3 dst)
{
	Vector3 diff;
	diff.x = src.x - dst.x;
	diff.y = src.y - dst.y;
	diff.z = src.z - dst.z;
	return diff;
}

inline void NormalizeAngles(Vector2& angle)
{
	while (angle.x > 89.0f)
		angle.x -= 180.f;

	while (angle.x < -89.0f)
		angle.x += 180.f;

	while (angle.y > 180.f)
		angle.y -= 360.f;

	while (angle.y < -180.f)
		angle.y += 360.f;
}

inline void normalY(float& f) {
	while (f > 180.f)
		f -= 360.f;

	while (f < -180.f)
		f += 360.f;
}

inline float VAR_Magnitudes(Vector3 vec)
{
	return sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}
inline float VAR_deltaDistances(Vector3 src, Vector3 dst)
{
	Vector3 diff = VAR_Subtracts(src, dst);
	return VAR_Magnitudes(diff);
}
inline void VAR_VectorAngless(const float* forward, float* angles)
{
	float	tmp, yaw, pitch;
	if (forward[1] == 0 && forward[0] == 0)
	{
		yaw = 0;
		if (forward[2] > 0)
			pitch = 270;
		else
			pitch = 90;
	}
	else
	{
		yaw = (atan2(forward[1], forward[0]) * 57.295779513082f);
		if (yaw < 0)
			yaw += 360;
		tmp = sqrt(forward[0] * forward[0] + forward[1] * forward[1]);
		pitch = (atan2(-forward[2], tmp) * 57.295779513082f);
		if (pitch < 0)
			pitch += 360;
	}
	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}

inline Vector3 Var_CalcAngless(Vector3& src, Vector3& dst)
{
	float output[3];
	float input[3] = { dst.x - src.x , dst.y - src.y, dst.z - src.z };
	VAR_VectorAngless(input, output);
	if (output[1] > 180) output[1] -= 360;
	if (output[1] < -180) output[1] += 360;
	if (output[0] > 180) output[0] -= 360;
	if (output[0] < -180) output[0] += 360;
	return { output[0], output[1], 0.f };
}
struct Vector4 {
	float x, y, z, w;
};

inline int g_ScreenW = 0;
inline int g_ScreenH = 0;

inline Vector2 WorldToScreen(const Vector3& entity_position, Matrix4x4 view_matrix)
{
	Vector3 trans_vec{ view_matrix._14, view_matrix._24, view_matrix._34 };
	Vector3 right_vec{ view_matrix._11, view_matrix._21, view_matrix._31 };
	Vector3 up_vec{ view_matrix._12, view_matrix._22, view_matrix._32 };

	float w = trans_vec.Dot(entity_position) + view_matrix._44;

	if (w < 0.1f) return Vector2(-1, -1);

	float y = up_vec.Dot(entity_position) + view_matrix._42;
	float x = right_vec.Dot(entity_position) + view_matrix._41;

	int screenW = g_ScreenW > 0 ? g_ScreenW : GetSystemMetrics(SM_CXSCREEN);
	int screenH = g_ScreenH > 0 ? g_ScreenH : GetSystemMetrics(SM_CYSCREEN);

	Vector2 Screen_position = Vector2((screenW / 2.f) * (1.f + x / w), (screenH / 2.f) * (1.f - y / w));
	return Screen_position;
}



enum BoneList : int
{
	pelvis = 0,
	l_hip = 1,
	l_hip_twist_02 = 2,
	l_knee = 3,
	l_foot = 4,
	l_toe = 5,
	l_knee_twist_02 = 6,
	r_hip = 14,
	r_hip_twist_02 = 15,
	r_knee = 16,
	r_foot = 17,
	r_toe = 18,
	r_knee_twist_02 = 19,
	spine1 = 20,
	spine2 = 21,
	spine3 = 22,
	spine4 = 23,
	l_clavicle = 24,
	l_upperarm = 25,
	l_forearm = 26,
	l_forearm_twist_01 = 27,
	l_forearm_twist_02 = 28,
	l_hand = 29,
	l_index_meta = 30,
	l_index1 = 31,
	l_index2 = 32,
	l_index3 = 33,
	l_middle_meta = 34,
	l_middle1 = 35,
	l_middle2 = 36,
	l_middle3 = 37,
	l_pinky_meta = 38,
	l_little1 = 39,
	l_little2 = 40,
	l_little3 = 41,
	l_prop = 42,
	l_ring_meta = 43,
	l_ring1 = 44,
	l_ring2 = 45,
	l_ring3 = 46,
	l_thumb1 = 47,
	l_thumb2 = 48,
	l_thumb3 = 49,
	l_upperarm_twist_01 = 50,
	l_upperarm_twist_02 = 51,
	neck = 52,
	head = 53,
	eyetransform = 54,
	jaw = 55,
	l_eye = 56,
	l_eyelid = 57,
	r_eye = 58,
	r_eyelid = 59,
	r_clavicle = 60,
	r_upperarm = 61,
	r_forearm = 62,
	r_forearm_twist_01 = 63,
	r_forearm_twist_02 = 64,
	r_hand = 65,
	r_index_meta = 66,
	r_index1 = 67,
	r_index2 = 68,
	r_index3 = 69,
	r_middle_meta = 70,
	r_middle1 = 71,
	r_middle2 = 72,
	r_middle3 = 73,
	r_pinky_meta = 74,
	r_little1 = 75,
	r_little2 = 76,
	r_little3 = 77,
	r_prop = 78,
	r_ring_meta = 79,
	r_ring1 = 80,
	r_ring2 = 81,
	r_ring3 = 82,
	r_thumb1 = 83,
	r_thumb2 = 84,
	r_thumb3 = 85,
	r_upperarm_twist_01 = 86,
	r_upperarm_twist_02 = 87,
};

#pragma region BPFlags

enum class BCameraMode {
	FirstPerson = 0,
	ThirdPerson = 1,
	Eyes = 2,
	FirstPersonWithArms = 3,
	Last = 3
};

enum class BPlayerFlags {
	Unused1 = 1,
	Unused2 = 2,
	IsAdmin = 4,
	ReceivingSnapshot = 8,
	Sleeping = 16,
	Spectating = 32,
	Wounded = 64,
	IsDeveloper = 128,
	Connected = 256,
	ThirdPersonViewmode = 1024,
	EyesViewmode = 2048,
	ChatMute = 4096,
	NoSprint = 8192,
	Aiming = 16384,
	DisplaySash = 32768,
	Relaxed = 65536,
	SafeZone = 131072,
	ServerFall = 262144,
	Workbench1 = 1048576,
	Workbench2 = 2097152,
	Workbench3 = 4194304,
	DebugCamera = 260
};


enum class BMapNoteType {
	Death = 0,
	PointOfInterest = 1
};

enum class BTimeCategory {
	Wilderness = 1,
	Monument = 2,
	Base = 4,
	Flying = 8,
	Boating = 16,
	Swimming = 32,
	Driving = 64
};
enum class MStateFlags {
	Ducked = 1,
	Jumped = 2,
	OnGround = 4,
	Sleeping = 8,
	Sprinting = 16,
	OnLadder = 32,
	Flying = 64,
	Aiming = 128,
	Prone = 256,
	Mounted = 512,
	Relaxed = 1024,
};
enum class BaseEntityFlag
{
	stash = 2048,
};
inline const double ToRad(double degree)
{
	double pi = 3.14159265359;
	return (degree * (pi / 180));
}
inline Vector4 ToQuat(Vector3 Euler) {
	float c1 = cos(ToRad(Euler.x) / 2);
	float s1 = sin(ToRad(Euler.x) / 2);
	float c2 = cos(ToRad(Euler.y) / 2);
	float s2 = sin(ToRad(Euler.y) / 2);
	float c3 = cos(ToRad(Euler.z) / 2);
	float s3 = sin(ToRad(Euler.z) / 2);
	float c1c2 = c1 * c2;
	float s1s2 = s1 * s2;
	float c1s2 = c1 * s2;
	float s1c2 = s1 * c2;
	Vector4 Quat = {
		s1c2 * c3 + c1s2 * s3,
		c1s2 * c3 - s1c2 * s3,
		c1c2 * s3 - s1s2 * c3,
		c1c2 * c3 + s1s2 * s3
	};
	return Quat;
}
inline float Calc3D_Dist(const Vector3& Src, const Vector3& Dst) {
	return Vector3::sqrtf(sqr(Src.x - Dst.x) + sqr(Src.y - Dst.y) + sqr(Src.z - Dst.z));
}

class m_matrix
{
public:
	union
	{
		struct
		{
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
		float m[4][4];
	};

	m_matrix()
		: m{ { 0, 0, 0, 0 },
			 { 0, 0, 0, 0 },
			 { 0, 0, 0, 0 },
			 { 0, 0, 0, 0 } }
	{
	}

	m_matrix(const m_matrix&) = default;

	auto transpose() -> m_matrix
	{
		m_matrix m;

		for (int i = 0; i < 4; ++i)
		{
			for (int x = 0u; x < 4; ++x)
			{
				m.m[i][x] = this->m[x][i];
			}
		}

		return m;
	}

	auto multiply_point_3(Vector3& input, Vector3& output) -> bool
	{
		float w = input.x * m[3][0] + input.y * m[3][1] + input.z * m[3][2] + m[3][3];
		if (std::abs(w) <= 0.0000001f)
		{
			output = Vector3(0, 0, 0);
			return false;
		}

		output.x = (input.x * m[0][0] + input.y * m[0][1] + input.z * m[0][2] + m[0][3]) / w;
		output.y = (input.x * m[1][0] + input.y * m[1][1] + input.z * m[1][2] + m[1][3]) / w;
		output.z = (input.x * m[2][0] + input.y * m[2][1] + input.z * m[2][2] + m[2][3]) / w;

		return true;
	}

	float* operator[](std::size_t idx) { return m[idx]; }
	const float* operator[](std::size_t idx) const { return m[idx]; }
};
