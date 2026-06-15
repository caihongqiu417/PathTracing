#pragma once
#include<cmath>
//====================================================
// 复数类 a+bi c+di
//====================================================
struct Complex
{
	//a+bi c+di
	union {
		float a;
		float c;
	};

	union {
		float b;
		float d;
	};

	Complex(float a, float b)
		:a(a), b(b)
	{
	}

	Complex(float a)
		:a(a), b(0)
	{
		
	}
};

inline Complex operator+(Complex lhs, Complex rhs)
{
	return{ lhs.a + rhs.c,lhs.b + rhs.d };
}


inline Complex operator-(Complex lhs, Complex rhs)
{
	return{ lhs.a - rhs.c,lhs.b - rhs.d };
}

inline Complex operator*(Complex lhs, Complex rhs)
{
	//(a+bi)(c+di)=(ac-bd)+(ad+bc)i
	return{ lhs.a * rhs.c-lhs.b*rhs.d,lhs.a*rhs.d+lhs.b*rhs.c};
}

inline Complex operator/(Complex lhs, Complex rhs)
{
	// (a+bi)/(c+di) = (a+bi)(c-di) / (c²+d²)
	float denom = rhs.c * rhs.c + rhs.d * rhs.d;
	return { (lhs.a * rhs.c + lhs.b * rhs.d) / denom,
	         (lhs.b * rhs.c - lhs.a * rhs.d) / denom };
}

// 模的平方 |z|² = a²+b²（注意：不带根号，和 std::norm 语义一致）
inline float norm(Complex rhs)
{
	return rhs.a * rhs.a + rhs.b * rhs.b;
};

// 复数平方根（主值）
// 推导：设 z = a+bi，r = |z|
//   Re(√z) = √((r+a)/2)
//   Im(√z) = sign(b) * √((r-a)/2)
inline Complex sqrt(Complex rhs)
{
	float r = std::sqrt(rhs.a * rhs.a + rhs.b * rhs.b);
	float re = std::sqrt((r + rhs.a) * 0.5f);
	float im = std::sqrt((r - rhs.a) * 0.5f);
	if (rhs.b < 0.f) im = -im;  
	return { re, im };
};

