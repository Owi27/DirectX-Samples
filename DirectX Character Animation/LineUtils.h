#pragma once

#include <directxmath.h>
#include <vector>

using namespace DirectX;
using namespace std;

struct ColorVertex
{
	XMFLOAT3 pos1;
	XMFLOAT4 color;
};

struct DebugLines
{
	vector<ColorVertex> vertexList;

	void AddLine(XMFLOAT3 pos1, XMFLOAT3 pos2, XMFLOAT4 color)
	{
		vertexList.push_back({ pos1, color });
		vertexList.push_back({ pos2, color });
	}
};

namespace LineUtils
{
	void MakeGrid(DebugLines& lines)
	{
		float size = 10.0f;
		float spacing = 0.5f;
		int lineCount = (int)(size / spacing);

		float x = -size / 2.0f;
		float y = -size / 2.0f;
		float xS = spacing, yS = spacing;

		y = -size / 2.0f;
		for (int i = 0; i <= lineCount; i++)
		{
			if (i != lineCount / 2)
				lines.AddLine({ x,0,y }, { x + size,0,y }, { .5,.5,.5,1 });
			else
				lines.AddLine({ x,0,y }, { x + size,0,y }, { 1,1,1,1 });
			y += yS;
		}
		y = -size / 2.0f;
		x = -size / 2.0f;
		for (int i = 0; i <= lineCount; i++)
		{
			if (i != lineCount / 2)
				lines.AddLine({ x,0,y }, { x ,0,y + size }, { .5,.5,.5,1 });
			else
				lines.AddLine({ x,0,y }, { x ,0,y + size }, { 1,1,1,1 });
			x += xS;
		}
	}
}