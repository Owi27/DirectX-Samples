#pragma once
#include <directxmath.h>
#include <vector>

using namespace std;
using namespace DirectX;

struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 Tex;
};

template <typename T>
struct SimpleMesh
{
	vector<T> vertexList;
	vector<int> indicesList;
};

namespace MeshUtils
{
	void Compactify(SimpleMesh<SimpleVertex>& simpleMesh)
	{
		// Using vectors because we don't know what size we are
		// going to need until the end
		vector<SimpleVertex> compactedVertexList;
		vector<int> indicesList;

		// initialize running index
		int compactedIndex = 0;

		// for each vertex in the expanded array
		// compare to the compacted array for a matching
		// vertex, if found, skip adding and set the index
		for (SimpleVertex vertSimpleMesh : simpleMesh.vertexList)
		{
			bool found = false;
			int foundIndex = 0;
			// search for match with the rest in the array
			for (SimpleVertex vertCompactedList : compactedVertexList)
			{
				if (vertSimpleMesh.Pos.x == vertCompactedList.Pos.x &&
					vertSimpleMesh.Pos.y == vertCompactedList.Pos.y &&
					vertSimpleMesh.Pos.z == vertCompactedList.Pos.z &&
					vertSimpleMesh.Normal.x == vertCompactedList.Normal.x &&
					vertSimpleMesh.Normal.y == vertCompactedList.Normal.y &&
					vertSimpleMesh.Normal.z == vertCompactedList.Normal.z &&
					vertSimpleMesh.Tex.x == vertCompactedList.Tex.x &&
					vertSimpleMesh.Tex.y == vertCompactedList.Tex.y
					)
				{
					//cout << "Match at " << i << "-";
					indicesList.push_back(foundIndex);
					found = true;
					break;
				}
				foundIndex++;
			}
			// didn't find a duplicate so keep (push back) the current vertex
			// and increment the index count and push back that index as well
			if (!found)
			{
				compactedVertexList.push_back(vertSimpleMesh);
				indicesList.push_back(compactedIndex);
				compactedIndex++;
			}
		}

		int numIndices = (int)simpleMesh.indicesList.size();
		int numVertices = (int)simpleMesh.vertexList.size();

		// print out some stats
		cout << "index count BEFORE/AFTER compaction " << numIndices << endl;
		cout << "vertex count UNOPTIMIZED (SimpleMesh In): " << numVertices << endl;
		cout << "vertex count AFTER compaction (SimpleMesh Out): " << compactedVertexList.size() << endl;
		cout << "Size reduction: " << ((numVertices - compactedVertexList.size()) / (float)numVertices) * 100.00f << "%" << endl;
		cout << "or " << (compactedVertexList.size() / (float)numVertices) << " of the expanded size" << endl;

		// copy working data to the global SimpleMesh
		simpleMesh.indicesList = indicesList;
		simpleMesh.vertexList = compactedVertexList;
	}

	// create a simple cube with normals and texture coordinates
	void makeCubePNT(SimpleMesh<SimpleVertex>& mesh)
	{
		// create vertices
		mesh.vertexList =
		{
			{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(-1.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f),  XMFLOAT2(-1.0f, 1.0f) },

			{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(-1.0f, 0.0f) },
			{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(-1.0f, 1.0f) },
			{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },

			{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(-1.0f, 1.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(-1.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },

			{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(-1.0f, 1.0f) },
			{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(-1.0f, 0.0f)  },

			{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(-1.0f, 1.0f) },
			{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(-1.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },

			{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(-1.0f, 1.0f) },
			{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(-1.0f, 0.0f) },
		};

		// Create indices
		mesh.indicesList =
		{
			3,1,0,
			2,1,3,

			6,4,5,
			7,4,6,

			11,9,8,
			10,9,11,

			14,12,13,
			15,12,14,

			19,17,16,
			18,17,19,

			22,20,21,
			23,20,22
		};
	}

	// create a simple cube with normals and texture coordinates
	void makeGroundPNT(SimpleMesh<SimpleVertex>& mesh)
	{
		float scale = 15.0f;
		// create vertices
		mesh.vertexList =
		{
			{ XMFLOAT3(-scale, 0.0f, -scale), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(-3.0f, 0.0f) },
			{ XMFLOAT3(scale, 0.0f, -scale), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(scale, 0.0f, scale), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 3.0f) },
			{ XMFLOAT3(-scale, 0.0f, scale), XMFLOAT3(0.0f, 1.0f, 0.0f),  XMFLOAT2(-3.0f, 3.0f) }
		};

		// Create indices
		mesh.indicesList =
		{
			3,1,0,
			2,1,3
		};
	}

	// create a simple cube with normals and texture coordinates
	void makeCrossHatchPNT(SimpleMesh<SimpleVertex>& mesh, float scale)
	{
		float darkNormal = 0.4f;
		// create vertices
		mesh.vertexList =
		{
			{ XMFLOAT3(-scale, 0.0f, -scale), XMFLOAT3(0.0f, darkNormal, 0.0f), XMFLOAT2(1.0f, 0.0f) },
			{ XMFLOAT3(scale, 0.0f, scale), XMFLOAT3(0.0f, darkNormal, 0.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(scale, scale * 2.0f, scale), XMFLOAT3(0.0f, 1.0f, -0.2f), XMFLOAT2(0.0f, -1.0f) },
			{ XMFLOAT3(-scale, scale * 2.0f, -scale), XMFLOAT3(0.0f, 1.0f, -0.2f),  XMFLOAT2(1.0f, -1.0f) },

			{ XMFLOAT3(scale, 0.0f, -scale), XMFLOAT3(0.0f, darkNormal, 0.0f), XMFLOAT2(-1.0f, 0.0f) },
			{ XMFLOAT3(-scale, 0.0f, scale), XMFLOAT3(0.0f, darkNormal, 0.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(-scale, scale * 2.0f, scale), XMFLOAT3(0.0f, 1.0f, -0.2f), XMFLOAT2(0.0f, -1.0f) },
			{ XMFLOAT3(scale, scale * 2.0f, -scale), XMFLOAT3(0.0f, 1.0f, -0.2f),  XMFLOAT2(-1.0f, -1.0f) }
		};

		// Create indices
		mesh.indicesList =
		{
			3,1,0,
			2,1,3,

			6,4,5,
			7,4,6
		};
	}

	// create a simple cube with normals and texture coordinates
	void makeCrossHatchPNT(SimpleMesh<SimpleVertex>& mesh)
	{
		float scale = 0.5f;
		makeCrossHatchPNT(mesh, scale);
	}


}