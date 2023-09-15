#pragma once

#include <iostream>
#include <directxmath.h>
#include <vector>

// FBX includes
#include <fbxsdk.h>
#include "MeshUtils.h"
#include <string>

FbxManager* gSdkManager;
float scale = 1.0f / 40.0f;

// funtime random normal
//#define RAND_NORMAL XMFLOAT3(rand()/float(RAND_MAX),rand()/float(RAND_MAX),rand()/float(RAND_MAX))

// Add FBX mesh process function declaration here
void ProcessFBXMesh(FbxNode* Node, SimpleMesh<SimpleVertex>& simpleMesh, std::string& textureFilename);

void InitFBX()
{
	gSdkManager = FbxManager::Create();

	// create an IOSettings object
	FbxIOSettings* ios = FbxIOSettings::Create(gSdkManager, IOSROOT);
	gSdkManager->SetIOSettings(ios);
}

void LoadFBX(const std::string& filename, SimpleMesh<SimpleVertex> &simpleMesh, std::string& textureFilename)
{
	const char* ImportFileName = filename.c_str(); 

	// Create a scene
	FbxScene* lScene = FbxScene::Create(gSdkManager, "");

	FbxImporter* lImporter = FbxImporter::Create(gSdkManager, "");

	// Initialize the importer by providing a filename.
	if (!lImporter->Initialize(ImportFileName, -1, gSdkManager->GetIOSettings())) {
		printf("Call to FbxImporter::Initialize() failed.\n");
		printf("Error returned: %s\n\n", lImporter->GetStatus().GetErrorString());
		//exit(-1);
	}

	// Import the scene.
	bool lStatus = lImporter->Import(lScene);

	// Destroy the importer
	lImporter->Destroy();

	// Process the scene and build DirectX Arrays
	ProcessFBXMesh(lScene->GetRootNode(), simpleMesh, textureFilename);

	// Optimize the mesh
	MeshUtils::Compactify(simpleMesh);

	// Destroy the (no longer needed) scene
	lScene->Destroy();
}

string getFileName(const string& s)
{
	// look for '\\' first
	char sep = '/';

	size_t i = s.rfind(sep, s.length());
	if (i != string::npos) {
		return(s.substr(i + 1, s.length() - i));
	}
	else // try '/'
	{
		sep = '\\';
		i = s.rfind(sep, s.length());
		if (i != string::npos) {
			return(s.substr(i + 1, s.length() - i));
		}
	}
	return("");
}

// from C++ Cookbook by D. Ryan Stephens, Christopher Diggins, Jonathan Turkanis, Jeff Cogswell
// https://www.oreilly.com/library/view/c-cookbook/0596007612/ch10s17.html
void replaceExt(string& s, const string& newExt) {

	string::size_type i = s.rfind('.', s.length());

	if (i != string::npos) {
		s.replace(i + 1, newExt.length(), newExt);
	}
}

void ProcessFBXMesh(FbxNode* Node, SimpleMesh<SimpleVertex>& simpleMesh, std::string& textureFilename)
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
				simpleMesh.vertexList[j].Pos.x = (float)vert.mData[0] * scale;
				simpleMesh.vertexList[j].Pos.y = (float)vert.mData[1] * scale;
				simpleMesh.vertexList[j].Pos.z = (float)vert.mData[2] * scale;
				// Generate random normal for first attempt at getting to render
				//simpleMesh.vertexList[j].Normal = RAND_NORMAL;
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

			//get all UV set names
			FbxStringList lUVSetNameList;
			mesh->GetUVSetNames(lUVSetNameList);
			const char* lUVSetName = lUVSetNameList.GetStringAt(0);
			const FbxGeometryElementUV* lUVElement = mesh->GetElementUV(lUVSetName);
			
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

				if (lUVElement->GetReferenceMode() == FbxLayerElement::eDirect)
				{
					FbxVector2 lUVValue = lUVElement->GetDirectArray().GetAt(indices[j]);

					vertexListExpanded[j].Tex.x = (float)lUVValue[0];
					vertexListExpanded[j].Tex.y = 1.0f - (float)lUVValue[1];
				}
				else if (lUVElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
				{
					auto& index_array = lUVElement->GetIndexArray();

					FbxVector2 lUVValue = lUVElement->GetDirectArray().GetAt(index_array[j]);

					vertexListExpanded[j].Tex.x = (float)lUVValue[0];
					vertexListExpanded[j].Tex.y = 1.0f - (float)lUVValue[1];
				}
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

		//================= Texture ========================================

		int materialCount = childNode->GetSrcObjectCount<FbxSurfaceMaterial>();
		//cout << "\nmaterial count: " << materialCount << std::endl;

		for (int index = 0; index < materialCount; index++)
		{
			FbxSurfaceMaterial* material = (FbxSurfaceMaterial*)childNode->GetSrcObject<FbxSurfaceMaterial>(index);
			//cout << "\nmaterial: " << material << std::endl;

			if (material != NULL)
			{
				//cout << "\nmaterial: " << material->GetName() << std::endl;
				// This only gets the material of type sDiffuse, you probably need to traverse all Standard Material Property by its name to get all possible textures.
				FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);

				// Check if it's layeredtextures
				int layeredTextureCount = prop.GetSrcObjectCount<FbxLayeredTexture>();

				if (layeredTextureCount > 0)
				{
					for (int j = 0; j < layeredTextureCount; j++)
					{
						FbxLayeredTexture* layered_texture = FbxCast<FbxLayeredTexture>(prop.GetSrcObject<FbxLayeredTexture>(j));
						int lcount = layered_texture->GetSrcObjectCount<FbxTexture>();

						for (int k = 0; k < lcount; k++)
						{
							FbxFileTexture* texture = FbxCast<FbxFileTexture>(layered_texture->GetSrcObject<FbxTexture>(k));
							// Then, you can get all the properties of the texture, include its name
							const char* textureName = texture->GetFileName();
							//cout << textureName;
						}
					}
				}
				else
				{
					// Directly get textures
					int textureCount = prop.GetSrcObjectCount<FbxTexture>();
					for (int j = 0; j < textureCount; j++)
					{
						FbxFileTexture* texture = FbxCast<FbxFileTexture>(prop.GetSrcObject<FbxTexture>(j));
						// Then, you can get all the properties of the texture, include its name
						const char* textureName = texture->GetFileName();
						//cout << "\nTexture Filename " << textureName;
						textureFilename = textureName;
						FbxProperty p = texture->RootProperty.Find("Filename");
						//cout << p.Get<FbxString>() << std::endl;

					}
				}

				// strip out the path and change the file extension
				textureFilename = getFileName(textureFilename);
				replaceExt(textureFilename, "dds");
				//cout << "\nTexture Filename " << textureFilename << endl;

			}
		}
	}
	// did not find a mesh here so recurse
		else
		ProcessFBXMesh(childNode, simpleMesh, textureFilename);
	}
}