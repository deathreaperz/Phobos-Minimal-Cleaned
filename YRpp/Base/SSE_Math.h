#pragma once

#include <Base/Always.h>
#include <xmmintrin.h>
#include <limits>

#pragma warning( push )
#pragma warning (disable : 4244)

const f32 gf_PI = f32(3.14159265358979323846264338327950288419716939937510);
const f64 g_PI = 3.14159265358979323846264338327950288419716939937510; // pi

const f32 gf_PI2 = f32(3.14159265358979323846264338327950288419716939937510 * 2.0);
const f64 g_PI2 = 3.14159265358979323846264338327950288419716939937510 * 2.0; // 2*pi

const f64 sqrt2 = 1.4142135623730950488016887242097;
const f64 sqrt3 = 1.7320508075688772935274463415059;

const f32 gf_halfPI = f32(1.57079632679489661923132169163975144209858469968755);

#define VEC_EPSILON (0.05f)
#define RAD_EPSILON (0.01f)
#define DEG2RAD(a) ((a) * (gf_PI / 180.0f))
#define RAD2DEG(a) ((a) * (180.0f / gf_PI))
#define DEG2COS(a) (cos_tpl((a) * (gf_PI / 180.0f)))
#define COS2DEG(a) (acos_tpl(a) * (180.0f / gf_PI))
#define RAD2HCOS(a) (cos_tpl((a * 0.5f)))
#define HCOS2RAD(a) (acos_tpl(a) * 2.0f)
#define DEG2HCOS(a) (cos_tpl((a * 0.5f) * (gf_PI / 180.0f)))
#define DEG2HSIN(a) (sin_tpl((a * 0.5f) * (gf_PI / 180.0f)))
#define HCOS2DEG(a) (acos_tpl(a) * 2.0f * (180.0f / gf_PI))
#define SIGN_MASK(x) ((intptr_t)(x) >> ((sizeof(size_t) * 8) - 1))
#define TANGENT30 0.57735026918962576450914878050196f // tan(30)
#define TANGENT30_2 0.57735026918962576450914878050196f * 2 // 2*tan(30)

// Bring min and max from std namespace to global scope.
template <class T>
ILINE T min(const T& a, const T& b) { return b < a ? b : a; }
template <class T>
ILINE T max(const T& a, const T& b) { return a < b ? b : a; }
template <class T, class _Compare>
ILINE const T& min(const T& a, const T& b, _Compare comp) { return comp(b, a) ? b : a; }
template <class T, class _Compare>
ILINE const T& max(const T& a, const T& b, _Compare comp) { return comp(a, b) ? b : a; }
ILINE int min_branchless(int a, int b) { int diff = a - b; int mask = diff >> 31; return (b & (~mask)) | (a & mask); }

template<class T>
ILINE T clamp_tpl(T X, T Min, T Max) { return X < Min ? Min : X < Max ? X : Max; }
template<class T>
ILINE void Limit(T& val, const T& min, const T& max)
{
	if (val < min)
	{
		val = min;
	}
	else if (val > max)
	{
		val = max;
	}
}
template<class T>
ILINE T Lerp(const T& a, const T& b, float s) { return T(a + (b - a) * s); }

ILINE f32 fsel(const f32 _a, const f32 _b, const f32 _c) { return (_a < 0.0f) ? _c : _b; }
ILINE f64 fsel(const f64 _a, const f64 _b, const f64 _c) { return (_a < 0.0f) ? _c : _b; }
ILINE f32 fres(const f32 _a) { return 1.f / _a; }
template<class T>
ILINE T isel(int c, T a, T b) { return (c < 0) ? b : a; }
template<class T>
ILINE T isel(int64 c, T a, T b) { return (c < 0) ? b : a; }
template<class T>
ILINE T iselnz(int c, T a, T b) { return c ? a : b; }
template<class T>
ILINE T iselnz(uint32 c, T a, T b) { return c ? a : b; }
template<class T>
ILINE T iselnz(int64 c, T a, T b) { return c ? a : b; }

//provides fast way of checking against 0 (saves fcmp)
ILINE bool fzero(const float& val) { return val == 0.0f; }

ILINE bool fzero(float* pVal) { return *pVal == 0.0f; }
ILINE f32 fabs_tpl(f32 op) { return op < 0.0f ? -op : op; }
ILINE f64 fabs_tpl(f64 op) { return fabs(op); }
ILINE int32 fabs_tpl(int32 op) { int32 mask = op >> 31; return op + mask ^ mask; }

ILINE f32 floor_tpl(f32 op) { return floorf(op); }
ILINE f64 floor_tpl(f64 op) { return floor(op); }
ILINE int32 floor_tpl(int32 op) { return op; }

ILINE f32 ceil_tpl(f32 op) { return ceilf(op); }
ILINE f64 ceil_tpl(f64 op) { return ceil(op); }
ILINE int32 ceil_tpl(int32 op) { return op; }

ILINE f32 fmod_tpl(f32 x, f32 y) { return (f32)fmodf(x, y); }
ILINE f64 fmod_tpl(f64 x, f64 y) { return (f32)fmod(x, y); }

ILINE void sincos_tpl(f32 angle, f32* pSin, f32* pCos) { *pSin = f32(sin(angle));    *pCos = f32(cos(angle)); }
ILINE void sincos_tpl(f64 angle, f64* pSin, f64* pCos) { *pSin = f64(sin(angle));  *pCos = f64(cos(angle)); }

ILINE f32 cos_tpl(f32 op) { return cosf(op); }
ILINE f64 cos_tpl(f64 op) { return cos(op); }

ILINE f32 sin_tpl(f32 op) { return sinf(op); }
ILINE f64 sin_tpl(f64 op) { return sin(op); }

ILINE f32 acos_tpl(f32 op) { return acosf(clamp_tpl(op, -1.0f, +1.0f)); }
ILINE f64 acos_tpl(f64 op) { return acos(clamp_tpl(op, -1.0, +1.0)); }

ILINE f32 asin_tpl(f32 op) { return asinf(clamp_tpl(op, -1.0f, +1.0f)); }
ILINE f64 asin_tpl(f64 op) { return asin(clamp_tpl(op, -1.0, +1.0)); }

ILINE f32 atan_tpl(f32 op) { return atanf(op); }
ILINE f64 atan_tpl(f64 op) { return atan(op); }

ILINE f32 atan2_tpl(f32 op1, f32 op2) { return atan2f(op1, op2); }
ILINE f64 atan2_tpl(f64 op1, f64 op2) { return atan2(op1, op2); }

ILINE f32 tan_tpl(f32 op) { return tanf(op); }
ILINE f64 tan_tpl(f64 op) { return tan(op); }

ILINE f32 exp_tpl(f32 op) { return expf(op); }
ILINE f64 exp_tpl(f64 op) { return exp(op); }

ILINE f32 log_tpl(f32 op) { return logf(op); }
ILINE f64 log_tpl(f64 op) { return log(op); }

ILINE f32 pow_tpl(f32 x, f32 y) { return (f32)pow((f64)x, (f64)y); }
ILINE f64 pow_tpl(f64 x, f64 y) { return pow(x, y); }

ILINE f32 sqrt_tpl(f32 op)
{
	__m128 s = _mm_sqrt_ss(_mm_set_ss(op));
	float r;
	_mm_store_ss(&r, s);
	return r;
}
ILINE f64 sqrt_tpl(f64 op)
{
	return sqrt(op);
}

ILINE f32 sqrt_fast_tpl(f32 op)
{
	return sqrt_tpl(op);
}
ILINE f64 sqrt_fast_tpl(f64 op)
{
	return sqrt_tpl(op);
}

ILINE f32 isqrt_tpl(f32 op)
{
	__m128 value = _mm_set_ss(op);
	__m128 oneHalf = _mm_set_ss(0.5f);
	__m128 threeHalfs = _mm_set_ss(1.5f);
	__m128 simdRecipSqrt = _mm_rsqrt_ss(value);
	__m128 inverseMult = _mm_mul_ps(_mm_mul_ss(_mm_mul_ss(value, simdRecipSqrt), simdRecipSqrt), oneHalf);
	__m128 inverseInner = _mm_sub_ps(threeHalfs, inverseMult);
	__m128 newtonIteration1 = _mm_mul_ss(simdRecipSqrt, inverseInner);
	float r;
	_mm_store_ss(&r, newtonIteration1);
	return r;
}
ILINE f64 isqrt_tpl(f64 op)
{
	return 1.0 / sqrt(op);
}

ILINE f32 isqrt_fast_tpl(f32 op)
{
	return isqrt_tpl(op);
}
ILINE f64 isqrt_fast_tpl(f64 op)
{
	return isqrt_tpl(op);
}

ILINE f32 isqrt_safe_tpl(f32 value)
{
	return isqrt_tpl(value + (std::numeric_limits<f32>::min)());
}

ILINE f64 isqrt_safe_tpl(f64 value)
{
	return isqrt_tpl(value + (std::numeric_limits<f64>::min)());
}

ILINE int32 int_round(f32 f) { return f < 0.f ? int32(f - 0.5f) : int32(f + 0.5f); }
ILINE int32 pos_round(f32 f) { return int32(f + 0.5f); }
ILINE int64 int_round(f64 f) { return f < 0.0 ? int64(f - 0.5) : int64(f + 0.5); }
ILINE int64 pos_round(f64 f) { return int64(f + 0.5); }

ILINE int32 int_ceil(f32 f) { int32 i = int32(f); return (f > f32(i)) ? i + 1 : i; }
ILINE int64 int_ceil(f64 f) { int64 i = int64(f); return (f > f64(i)) ? i + 1 : i; }

template<class F>
ILINE F sqr(const F& op) { return op * op; }
template<class F>
ILINE F sqr_signed(const F& op) { return op * fabs_tpl(op); }
template<class F>
ILINE F cube(const F& op) { return op * op * op; }

template<class F>
ILINE F square(F fOp) { return(fOp * fOp); }
ILINE float div_min(float n, float d, float m) { return n * d < m* d* d ? n / d : m; }

// Utility functions for returning -1 if input is negative and non-zero, returns positive 1 otherwise
// Uses extensive bit shifting for performance reasons.
ILINE int32 sgnnz(f64 x)
{
	union
	{
		f32 f;
		int32 i;
	} u;
	u.f = (f32)x;
	return ((u.i >> 31) << 1) + 1;
}

ILINE int32 sgnnz(f32 x)
{
	union
	{
		f32 f;
		int32 i;
	} u;
	u.f = x;
	return ((u.i >> 31) << 1) + 1;
}

ILINE int32 sgnnz(int32 x) { return ((x >> 31) << 1) + 1; }

ILINE f32   fsgnnz(f32 x)
{
	union
	{
		f32 f;
		int32 i;
	} u;
	u.f = x;
	u.i = (u.i & 0x80000000) | 0x3f800000;
	return u.f;
}

ILINE int32 isneg(f32 x)
{
	union
	{
		f32 f;
		uint32 i;
	} u;
	u.f = x;
	return (int32)(u.i >> 31);
}

ILINE int32 isneg(f64 x)
{
	union
	{
		f32 f;
		uint32 i;
	} u;
	u.f = (f32)x;
	return (int32)(u.i >> 31);
}

ILINE int32 isneg(int32 x) { return (int32)((uint32)x >> 31); }

ILINE int32 sgn(f64 x)
{
	union
	{
		f32 f;
		int32 i;
	} u;
	u.f = (f32)x;
	return (u.i >> 31) + ((u.i - 1) >> 31) + 1;
}
ILINE int32 sgn(f32 x)
{
	union
	{
		f32 f;
		int32 i;
	} u;
	u.f = x;
	return (u.i >> 31) + ((u.i - 1) >> 31) + 1;
}

ILINE int32 sgn(int32 x) { return (x >> 31) + ((x - 1) >> 31) + 1; }

ILINE f32   fsgnf(f32 x) { return f32(sgn(x)); }

ILINE int32 isnonneg(f32 x)
{
	union
	{
		f32 f;
		uint32 i;
	} u;
	u.f = x;
	return (int32)(u.i >> 31 ^ 1);
}
ILINE int32 isnonneg(f64 x)
{
	union
	{
		f32 f;
		uint32 i;
	} u;
	u.f = (f32)x;
	return (int32)(u.i >> 31 ^ 1);
}

ILINE int32 isnonneg(int32 x) { return (int32)((uint32)x >> 31 ^ 1); }

ILINE int32 getexp(f32 x) { return (int32)(*(uint32*)&x >> 23 & 0x0FF) - 127; }
ILINE int32 getexp(f64 x) { return (int32)(*((uint32*)&x + 1) >> 20 & 0x7FF) - 1023; }
ILINE f32& setexp(f32& x, int32 iexp) { (*(uint32*)&x &= ~(0x0FF << 23)) |= (iexp + 127) << 23;   return x; }
ILINE f64& setexp(f64& x, int32 iexp) { (*((uint32*)&x + 1) &= ~(0x7FF << 20)) |= (iexp + 1023) << 20;  return x; }

ILINE int32 iszero(f32 x)
{
	union
	{
		f32 f;
		int32 i;
	} u;
	u.f = x;
	u.i &= 0x7FFFFFFF;
	return -(u.i >> 31 ^ (u.i - 1) >> 31);
}

ILINE int32 iszero(f64 x)
{
	union
	{
		f32 f;
		int32 i;
	} u;
	u.f = (f32)x;
	u.i &= 0x7FFFFFFF;
	return -((u.i >> 31) ^ (u.i - 1) >> 31);
}

ILINE float if_neg_else(float test, float val_neg, float val_nonneg) { return (float)fsel(test, val_nonneg, val_neg); }
template<class F>
ILINE int32 inrange(F x, F end1, F end2) { return isneg(fabs_tpl(end1 + end2 - x * (F)2) - fabs_tpl(end1 - end2)); }

template<class F>
ILINE int32 idxmax3(const F* pdata)
{
	int32 imax = isneg(pdata[0] - pdata[1]);
	imax |= isneg(pdata[imax] - pdata[2]) << 1;
	return imax & (2 | (imax >> 1 ^ 1));
}

ILINE float Linear(const float a, const float b, float t)
{
	return a * t + (1 - t) * b;
}

ILINE float Cosine(const float a, const float b, float t)
{
	float ft = t * gf_PI;
	t = (1 - cos(ft)) * 0.5;
	return Linear(a, b, t);
}

ILINE float Cubic(float v0, float v1, float v2, float v3, float t)
{
	return v0 + 0.5 * t * (v2 - v0 + t * (2.0 * v0 - 5.0 * v1 + 4.0 * v2 - v3 + t * (3.0 * (v1 - v2) + v3 - v0)));
}

ILINE float Hermite(float v0, float m0, float v1, float m1, float t)
{
	return v0 + t * (m0 + t * (-3 * v0 - 2 * m0 - m1 + 3 * v1 + t * (2 * v0 + m0 + m1 - 2 * v1)));
}

ILINE float roundoff(float value, unsigned char prec)
{
	float pow_10 = pow(10.0f, (float)prec);
	return round(value * pow_10) / pow_10;
}
#pragma warning( pop )