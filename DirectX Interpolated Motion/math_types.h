#pragma once

#include <array>
#include <cstdlib>
#include <cmath>
#include <stdint.h>

namespace end
{
	struct float2
	{
		float x;
		float y;

		inline float& operator[](int i) { return (&x)[i]; }
		inline float operator[](int i)const { return (&x)[i]; }

	};

	struct float3
	{
		union
		{
			struct
			{
				float x;
				float y;
				float z;
			};

			float2 xy;
		};


		inline float& operator[](int i) { return (&x)[i]; }
		inline float operator[](int i)const { return (&x)[i]; }

		inline friend float3 operator+(float3 lhs, float3 rhs)
		{
			return { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
		}

		inline friend float3 operator-(float3 lhs, float3 rhs)
		{
			return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
		}

		inline friend float3 operator*(float3 lhs, float3 rhs)
		{
			return { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z };
		}

		inline friend float3 operator*(float3 lhs, float rhs)
		{
			return { lhs.x * rhs, lhs.y * rhs, lhs.z * rhs };
		}

		inline friend float3 operator*(float lhs, float3 rhs)
		{
			return { rhs.x * lhs, rhs.y * lhs, rhs.z * lhs };
		}

		inline friend float3 operator/(float3 lhs, float3 rhs)
		{
			return { lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z };
		}

		inline friend float dot(float3 lhs, float3 rhs)
		{
			return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
		}

		inline friend float3 cross(float3 lhs, float3 rhs)
		{
			return
			{
				lhs.y * rhs.z - lhs.z * rhs.y,
				lhs.z * rhs.x - lhs.x * rhs.z,
				lhs.x * rhs.y - lhs.y * rhs.x
			};
		}

		inline friend float3 normalize(float3 v)
		{
			return v * (1.0f / sqrt(dot(v, v)));
		}

		inline float3& operator+=(float3 rhs)
		{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;

			return *this;
		}

		inline float3& operator-=(float3 rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;

			return *this;
		}

		inline float3& operator*=(float3 rhs)
		{
			x *= rhs.x;
			y *= rhs.y;
			z *= rhs.z;

			return *this;
		}

		inline float3& operator/=(float3 rhs)
		{
			x /= rhs.x;
			y /= rhs.y;
			z /= rhs.z;

			return *this;
		}

		inline float3& operator*=(float rhs)
		{
			x *= rhs;
			y *= rhs;
			z *= rhs;

			return *this;
		}

		inline float3& operator/=(float rhs)
		{
			x /= rhs;
			y /= rhs;
			z /= rhs;

			return *this;
		}

		inline friend float3 abs(float3 v)
		{
			return { std::abs(v.x), std::abs(v.y), std::abs(v.z) };
		}

		inline friend float accumulate(float3 v)
		{
			return v.x + v.y + v.z;
		}
	};

	struct float4
	{
		union
		{
			struct
			{
				float x;
				float y;
				float z;
				float w;
			};

			float3 xyz;
		};

		inline float& operator[](int i) { return (&x)[i]; }
		inline float operator[](int i)const { return (&x)[i]; }
	};

	struct alignas(8) float2_a : float2 {};

	struct alignas(16) float3_a : float3 {};

	struct alignas(16) float4_a : float4
	{
		using float4::float4;

		float4_a() = default;
		inline constexpr float4_a(float x, float y, float z, float w) : float4{ x,y,z,w } {}
	};

	using float4x4 = std::array< float4, 4 >;

	class alignas(16) float4x4_a : public float4x4 {};
}


namespace end
{
	struct colored_vertex
	{
		float3 pos = { 0.0f, 0.0f, 0.0f };
		float4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

		colored_vertex() = default;
		colored_vertex(const colored_vertex&) = default;

		inline colored_vertex(const float3& p, const float4& c) : pos{ p }, color{ c } {}
		inline colored_vertex(const float3& p, const float3& c) : pos{ p }, color{ c.x, c.y, c.z, 1.0f } {}
		inline colored_vertex(const float3& p, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : pos{ p }, color{ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f } {}
	};
}
