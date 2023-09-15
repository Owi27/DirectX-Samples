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

	bool inline operator==(const SimpleVertex& rhs)
	{
		return (Pos.x == rhs.Pos.x &&
			Pos.y == rhs.Pos.y &&
			Pos.z == rhs.Pos.z &&
			Normal.x == rhs.Normal.x &&
			Normal.y == rhs.Normal.y &&
			Normal.z == rhs.Normal.z &&
			Tex.x == rhs.Tex.x &&
			Tex.y == rhs.Tex.y
			);
	}
};

struct SkinnedVertex
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 Tex;
	XMFLOAT4 weights = { 0.0f, 0.0f, 0.0f, 0.0f };
	XMINT4 indices = { 0,0,0,0 };

	bool inline operator==(const SkinnedVertex& rhs)
	{
		return (Pos.x == rhs.Pos.x &&
			Pos.y == rhs.Pos.y &&
			Pos.z == rhs.Pos.z &&
			Normal.x == rhs.Normal.x &&
			Normal.y == rhs.Normal.y &&
			Normal.z == rhs.Normal.z &&
			Tex.x == rhs.Tex.x &&
			Tex.y == rhs.Tex.y &&
			weights.x == rhs.weights.x &&
			weights.y == rhs.weights.y &&
			weights.z == rhs.weights.z &&
			weights.w == rhs.weights.w &&
			indices.x == rhs.indices.x &&
			indices.y == rhs.indices.y &&
			indices.z == rhs.indices.z &&
			indices.w == rhs.indices.w
			);
	}
};

template <typename T>
struct SimpleMesh
{
	vector<T> vertexList;
	vector<int> indicesList;
};

namespace MeshUtils
{
	template <typename T>
	void Compactify(SimpleMesh<T>& simpleMesh)
	{
		// Using vectors because we don't know what size we are
		// going to need until the end
		vector<T> compactedVertexList;
		vector<int> indicesList;

		// initialize running index
		int compactedIndex = 0;

		// for each vertex in the expanded array
		// compare to the compacted array for a matching
		// vertex, if found, skip adding and set the index
		for (T vertSimpleMesh : simpleMesh.vertexList)
		{
			bool found = false;
			int foundIndex = 0;
			// search for match with the rest in the array
			for (T vertCompactedList : compactedVertexList)
			{
				//if (vertSimpleMesh.Pos.x == vertCompactedList.Pos.x &&
				//	vertSimpleMesh.Pos.y == vertCompactedList.Pos.y &&
				//	vertSimpleMesh.Pos.z == vertCompactedList.Pos.z &&
				//	vertSimpleMesh.Normal.x == vertCompactedList.Normal.x &&
				//	vertSimpleMesh.Normal.y == vertCompactedList.Normal.y &&
				//	vertSimpleMesh.Normal.z == vertCompactedList.Normal.z &&
				//	vertSimpleMesh.Tex.x == vertCompactedList.Tex.x &&
				//	vertSimpleMesh.Tex.y == vertCompactedList.Tex.y
				//	)
				if (vertSimpleMesh == vertCompactedList)
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

	template <typename T>
	void rh_to_lh_coord(SimpleMesh<T>& simpleMesh)
	{
		for (auto& v : simpleMesh.vertexList)
		{
			v.Pos.x = -v.Pos.x;
			v.Normal.x = -v.Normal.x;
			//v.Tex.y = -v.Tex.y;
		}

		int tri_count = (int)(simpleMesh.indicesList.size() / 3);

		for (int i = 0; i < tri_count; ++i)
		{
			auto tri = simpleMesh.indicesList.data() + i * 3;

			int temp = tri[0];
			tri[0] = tri[2];
			tri[2] = temp;
		}
	}

	void copy(SimpleMesh<SkinnedVertex>& skinnedMesh, SimpleMesh<SimpleVertex>& simpleMesh)
	{
		skinnedMesh.vertexList.resize(simpleMesh.vertexList.size());
		for (int i = 0; i < skinnedMesh.vertexList.size(); i++)
		{
			skinnedMesh.vertexList[i].Pos = simpleMesh.vertexList[i].Pos;
			skinnedMesh.vertexList[i].Normal = simpleMesh.vertexList[i].Normal;
			skinnedMesh.vertexList[i].Tex = simpleMesh.vertexList[i].Tex;
		}
		skinnedMesh.indicesList = simpleMesh.indicesList;
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