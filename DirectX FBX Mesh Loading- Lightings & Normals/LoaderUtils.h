#pragma once

#include <iostream>
#include <directxmath.h>
#include <vector>

// FBX includes
#include <fbxsdk.h>
#include "MeshUtils.h"
#include <string>

FbxManager* gSdkManager;
float scale = 50.0f;

// funtime random normal
#define RAND_NORMAL XMFLOAT3(rand()/float(RAND_MAX),rand()/float(RAND_MAX),rand()/float(RAND_MAX))
void ProcessFBXMesh(FbxNode* Node, SimpleMesh<SimpleVertex>& simpleMesh);



void InitFBX()
{
	gSdkManager = FbxManager::Create();

	// create an IOSettings object
	FbxIOSettings* ios = FbxIOSettings::Create(gSdkManager, IOSROOT);
	gSdkManager->SetIOSettings(ios);
}

void LoadFBX(const std::string& filename, SimpleMesh<SimpleVertex>& simpleMesh, std::string& textureFilename)
{
	const char* ImportFileName = filename.c_str();

	// Create a scene
	FbxScene* lScene = FbxScene::Create(gSdkManager, "");

	FbxImporter* lImporter = FbxImporter::Create(gSdkManager, "");

	// Initialize the importer by providing a filename.
	if (!lImporter->Initialize(ImportFileName, -1, gSdkManager->GetIOSettings())) {
		printf("Call to FbxImporter::Initialize() failed.\n");
		printf("Error returned: %s\n\n", lImporter->GetStatus().GetErrorString());
		exit(-1);
	}

	// Import the scene.
	bool lStatus = lImporter->Import(lScene);

	// Destroy the importer
	lImporter->Destroy();

	// Process the scene and build DirectX Arrays
	ProcessFBXMesh(lScene->GetRootNode(), simpleMesh);

	// Optimize the mesh
	MeshUtils::Compactify(simpleMesh);

	// Destroy the (no longer needed) scene
	lScene->Destroy();
}

void ProcessFBXMesh(FbxNode* Node, SimpleMesh<SimpleVertex>& simpleMesh)
{
	int childrenCount = Node->GetChildCount();

	cout << "\nName:" << Node->GetName();
	// check each child node for a FbxMesh
	for (int i = 0; i < childrenCount; i++)
	{
		FbxNode* childNode = Node->GetChild(i);
		FbxMesh* mesh = childNode->GetMesh();

		// Found a mesh on this node
		if (mesh != NULL)
		{
			cout << "\nMesh:" << childNode->GetName();

			// Get index count from mesh
			int numVertices = mesh->GetControlPointsCount();
			cout << "\nVertex Count:" << numVertices;

			// Resize the vertex vector to size of this mesh
			simpleMesh.vertexList.resize(numVertices);

			//================= Process Vertices ===============
			for (int j = 0; j < numVertices; j++)
			{
				FbxVector4 vert = mesh->GetControlPointAt(j);
				simpleMesh.vertexList[j].Pos.x = (float)vert.mData[0] / scale;
				simpleMesh.vertexList[j].Pos.y = (float)vert.mData[1] / scale;
				simpleMesh.vertexList[j].Pos.z = (float)vert.mData[2] / scale;
				// Generate random normal for first attempt at getting to render
				simpleMesh.vertexList[j].Normal = RAND_NORMAL;
			}

			int numIndices = mesh->GetPolygonVertexCount();
			cout << "\nIndice Count:" << numIndices;

			// No need to allocate int array, FBX does for us
			int* indices = mesh->GetPolygonVertices();

			// Fill indiceList
			simpleMesh.indicesList.resize(numIndices);
			memcpy(simpleMesh.indicesList.data(), indices, numIndices * sizeof(int));

			// Get the Normals array from the mesh
			FbxArray<FbxVector4> normalsVec;
			mesh->GetPolygonVertexNormals(normalsVec);
			cout << "\nNormalVec Count:" << normalsVec.Size();

			// Declare a new vector for the expanded vertex data
			// Note the size is numIndices not numVertices
			vector<SimpleVertex> vertexListExpanded;
			vertexListExpanded.resize(numIndices);

			// align (expand) vertex array and set the normals
			for (int j = 0; j < numIndices; j++)
			{
				// copy the original vertex position to the new vector
				// by using the index to look up the correct vertex
				// this is the "unindexing" step
				vertexListExpanded[j].Pos.x = simpleMesh.vertexList[indices[j]].Pos.x;
				vertexListExpanded[j].Pos.y = simpleMesh.vertexList[indices[j]].Pos.y;
				vertexListExpanded[j].Pos.z = simpleMesh.vertexList[indices[j]].Pos.z;
				// copy normal data directly, no need to unindex
				vertexListExpanded[j].Normal.x = (float)normalsVec.GetAt(j)[0];
				vertexListExpanded[j].Normal.y = (float)normalsVec.GetAt(j)[1];
				vertexListExpanded[j].Normal.z = (float)normalsVec.GetAt(j)[2];
			}

			// make new indices to match the new vertexListExpanded
			vector<int> indicesList;
			indicesList.resize(numIndices);
			for (int j = 0; j < numIndices; j++)
			{
				indicesList[j] = j; //literally the index is the count
			}

			// copy working data to the global SimpleMesh
			simpleMesh.indicesList = indicesList;
			simpleMesh.vertexList = vertexListExpanded;

		}
		// did not find a mesh here so recurse
		else
			ProcessFBXMesh(childNode, simpleMesh);
	}
}