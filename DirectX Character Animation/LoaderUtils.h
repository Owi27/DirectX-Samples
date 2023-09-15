#pragma once

#include <iostream>
#include <directxmath.h>
#include <vector>

// FBX includes
#include <fbxsdk.h>
#include "MeshUtils.h"
#include <string>

#include "dev5_anim.h"

FbxManager* gSdkManager;
float scale = 0.75f;

using namespace dev5;

struct fbx_joint
{
	FbxNode* node;
	int parent;
};

using fbx_joint_set = std::vector<fbx_joint>;

FbxSkin* mesh_skin(FbxMesh& mesh)
{
	for (int i = 0; i < mesh.GetDeformerCount(); ++i)
	{
		auto deformer = mesh.GetDeformer(i);

		if (deformer->Is<FbxSkin>())
			return (FbxSkin*)deformer;
	}

	return nullptr;
}

fbx_joint_set get_bindpose(FbxMesh& mesh)
{
	FbxSkin* skin = mesh_skin(mesh);

	FbxNode* link = skin->GetCluster(0)->GetLink();

	FbxSkeleton* skeleton_root = link->GetSkeleton();

	assert(skeleton_root);

	while (!skeleton_root->IsSkeletonRoot())
	{
		link = link->GetParent();
		skeleton_root = link->GetSkeleton();
	}

	fbx_joint_set joints;

	joints.push_back(fbx_joint{ link, -1 });

	for (int i = 0; i < joints.size(); ++i)
	{
		int child_count = joints[i].node->GetChildCount();

		for (int c = 0; c < child_count; ++c)
		{
			FbxNode* child = joints[i].node->GetChild(c);

			if (child->GetSkeleton())
				joints.push_back(fbx_joint{ child, i });
		}
	}

	return std::move(joints);

}

dev5::anim_clip_t LoadAnimationClip(FbxScene* lScene, FbxMesh* mesh)
{
	dev5::anim_clip_t anim_clip;
	//Load animation data
	fbx_joint_set bind_pose = get_bindpose(*mesh);

	int joint_count = (int)bind_pose.size();

	auto anim_stack = lScene->GetCurrentAnimationStack();
	FbxTimeSpan local_time_span = anim_stack->GetLocalTimeSpan();
	FbxTime timer = local_time_span.GetDuration();
	float duration = (float)timer.GetSecondDouble();
	int frame_count = (int)timer.GetFrameCount(FbxTime::eFrames24);
	anim_clip.duration = duration;

	for (int frame = 0; frame < frame_count; ++frame)
	{
		dev5::keyframe_t key_frame;

		key_frame.joints.resize(bind_pose.size());

		timer.SetFrame(frame, FbxTime::eFrames24);
		key_frame.time = (float)timer.GetSecondDouble();

		for (int j = 0; j < bind_pose.size(); ++j)
		{
			dev5::joint_t& joint = key_frame.joints[j];
			joint.parent = bind_pose[j].parent;
			FbxAMatrix mat = bind_pose[j].node->EvaluateGlobalTransform(timer);

			for (int r = 0; r < 4; ++r)
			{
				for (int c = 0; c < 4; ++c)
				{
					joint.transform[r][c] = (float)mat[r][c];
				}
			}
			{
				joint.transform[0].y = -joint.transform[0].y;
				joint.transform[0].z = -joint.transform[0].z;

				joint.transform[1].x = -joint.transform[1].x;
				joint.transform[2].x = -joint.transform[2].x;
				joint.transform[3].x = -joint.transform[3].x;
			}

		}
		anim_clip.keyframes.push_back(std::move(key_frame));
	}
	return move(anim_clip);
}

// Add FBX mesh process function declaration here
FbxMesh* ProcessFBXMesh(FbxNode* Node, SimpleMesh<SimpleVertex>& simpleMesh, std::string& textureFilename);

void InitFBX()
{
	gSdkManager = FbxManager::Create();

	// create an IOSettings object
	FbxIOSettings* ios = FbxIOSettings::Create(gSdkManager, IOSROOT);
	gSdkManager->SetIOSettings(ios);
}

FbxScene* LoadFBXScene(const char* ImportFileName)
{
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

	return lScene;
}

void LoadFBX(const std::string& filename, SimpleMesh<SimpleVertex>& simpleMesh, std::string& textureFilename)
{
	// Create a scene
	FbxScene* lScene = LoadFBXScene(filename.c_str());

	// Process the scene and build DirectX Arrays
	FbxMesh* mesh = ProcessFBXMesh(lScene->GetRootNode(), simpleMesh, textureFilename);

	// Optimize the mesh
	MeshUtils::Compactify(simpleMesh);

	// Convert vertex data from right-hand to left-hand coordinates
	MeshUtils::rh_to_lh_coord(simpleMesh);

	// Destroy the (no longer needed) scene
	lScene->Destroy();
}

struct influence_t
{
	float weight = 0.0f;
	int index = 0;
};

using influence_set_t = std::array< influence_t, 4 >;

using influence_buffer_t = std::vector<influence_set_t>;

influence_buffer_t get_influence_buffer(FbxMesh* mesh)
{
	fbx_joint_set bind_pose = get_bindpose(*mesh);

	assert(!bind_pose.empty());

	FbxSkin* skin = mesh_skin(*mesh);

	assert(skin);

	std::vector<influence_set_t> per_ctrl_pt;

	per_ctrl_pt.resize(mesh->GetControlPointsCount());

	int cluster_count = skin->GetClusterCount();

	for (int c = 0; c < cluster_count; ++c)
	{
		auto cluster = skin->GetCluster(c);

		int joint_index = -1;

		for (int j = 0; j < bind_pose.size(); ++j)
		{
			if (bind_pose[j].node == cluster->GetLink())
			{
				joint_index = j;
				break;
			}
		}

		assert(joint_index != -1);

		int cpi_count = cluster->GetControlPointIndicesCount();

		auto cpi = cluster->GetControlPointIndices();

		auto weights = cluster->GetControlPointWeights();

		for (int i = 0; i < cpi_count; ++i)
		{
			int index = cpi[i];

			influence_set_t& influence_set = per_ctrl_pt[index];

			influence_t temp{ (float)weights[i], joint_index };

			for (auto& inf : influence_set)
			{
				if (inf.weight < temp.weight)
					std::swap(temp, inf);
			}
		}
	}

	int poly_vert_count = mesh->GetPolygonVertexCount();
	int* polygon_verts = mesh->GetPolygonVertices();

	influence_buffer_t result;
	result.resize(poly_vert_count);

	for (int i = 0; i < poly_vert_count; ++i)
	{
		int point_index = polygon_verts[i];

		result[i] = per_ctrl_pt[point_index];
	}

	return std::move(result);
}

void LoadFBXAnimation(const std::string& filename, SimpleMesh<SkinnedVertex>& skinnedMesh, std::string& textureFilename, anim_clip_t& anim_clip)
{
	// Create a scene
	FbxScene* lScene = LoadFBXScene(filename.c_str());

	SimpleMesh<SimpleVertex> simpleMesh;
	// Process the scene and build DirectX Arrays
	FbxMesh* mesh = ProcessFBXMesh(lScene->GetRootNode(), simpleMesh, textureFilename);
	auto inf_buffer = get_influence_buffer(mesh);

	MeshUtils::copy(skinnedMesh, simpleMesh);

	for (int i = 0; i < skinnedMesh.vertexList.size(); i++)
	{
		skinnedMesh.vertexList[i].indices = { inf_buffer[i][0].index, inf_buffer[i][1].index, inf_buffer[i][2].index, inf_buffer[i][3].index };
		skinnedMesh.vertexList[i].weights = { inf_buffer[i][0].weight, inf_buffer[i][1].weight, inf_buffer[i][2].weight, inf_buffer[i][3].weight };
	}

	// Optimize the mesh
	MeshUtils::Compactify(skinnedMesh);

	// Convert vertex data from right-hand to left-hand coordinates
	MeshUtils::rh_to_lh_coord(skinnedMesh);

	//Load animation data
	anim_clip = LoadAnimationClip(lScene, mesh);

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

FbxMesh* ProcessFBXMesh(FbxNode* Node, SimpleMesh<SimpleVertex>& simpleMesh, std::string& textureFilename)
{
	int childrenCount = Node->GetChildCount();
	FbxMesh* mesh = nullptr;

	cout << "\nName:" << Node->GetName();
	// check each child node for a FbxMesh
	for (int i = 0; i < childrenCount; i++)
	{
		FbxNode* childNode = Node->GetChild(i);
		mesh = childNode->GetMesh();

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
					cout << "\nTexture Filename " << textureFilename << endl;

				}
			}
		}
		else
			// did not find a mesh here so recurse
			mesh = ProcessFBXMesh(childNode, simpleMesh, textureFilename);
	}
	return mesh;
}