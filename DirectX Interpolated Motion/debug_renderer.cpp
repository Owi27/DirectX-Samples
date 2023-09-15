#include "debug_renderer.h"
#include <array>
#include <cassert>

// Anonymous namespace
namespace
{
	// Declarations in an anonymous namespace are global BUT only have internal linkage.
	// In other words, these variables are global but are only visible in this source file.

	// Maximum number of debug lines at one time (i.e: Capacity)
	constexpr size_t MAX_LINE_VERTS = 8192;

	// CPU-side buffer of debug-line verts
	// Copied to the GPU and reset every frame.
	size_t line_vert_count = 0;
	std::array< end::colored_vertex, MAX_LINE_VERTS> line_verts;
}

namespace end
{
	namespace debug_renderer
	{
		void add_line(float3 point_a, float3 point_b, float4 color_a, float4 color_b)
		{
			// Add points to debug_verts, increments debug_vert_count

			assert(line_vert_count + 2 < MAX_LINE_VERTS);

			line_verts[line_vert_count++] = colored_vertex{ point_a, color_a };
			line_verts[line_vert_count++] = colored_vertex{ point_b, color_b };
		}

		void add_transform(const float4x4& transform)
		{
			const float3 start = transform[3].xyz;

			for (int i = 0; i < 3; ++i)
			{
				float4 color{ 0.0f, 0.0f, 0.0f, 1.0f };
				color[i] = 1.0f;

				float3 end = start + transform[i].xyz;

				add_line(start, end, color);
			}
		}

		void clear_lines()
		{
			// Resets debug_vert_count

			line_vert_count = 0;
		}

		const colored_vertex* get_line_verts()
		{ 
			// Does just what it says in the name

			return line_verts.data();
		}

		size_t get_line_vert_count() 
		{ 
			// Does just what it says in the name
			return line_vert_count;
		}

		size_t get_line_vert_capacity()
		{
			// Does just what it says in the name
			return MAX_LINE_VERTS;
		}
	}
}