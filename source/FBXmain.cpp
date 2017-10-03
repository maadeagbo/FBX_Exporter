#include <cstdlib>
#include <cstdio>
#include <string>

#include "FBX_Utility.h"
#include "FBX_MeshFuncs.h"

#include <fbxsdk.h>

int main(const int argc, const char** argv)
{
	if( argc == 1 ) {
		printf("Provide file to analyze for FBX generation\n");
		return 0;
	}
	// create fbx manager object
	FbxManager* sdkManager = FbxManager::Create();

	// initialize settings
	FbxIOSettings *_IOSettings = FbxIOSettings::Create(sdkManager, IOSROOT);
	sdkManager->SetIOSettings(_IOSettings);

	// process input
	for( size_t i = 1; i < (size_t)argc; i++ ) {
		std::string fileProvided = argv[i];
		size_t fileExtIndex = fileProvided.find_last_of('.');
		if( fileExtIndex <= 0 ) {
			printf("Provide valid FBX file\n");
			return 0;
		}
		std::string fileExt = fileProvided.substr(fileExtIndex + 1);
		if( fileExt == "FBX" || fileExt == "fbx") {
			printf("Parsing %s\n\n", argv[i]);
		}
		else {
			printf("Invalid file format\n");
		}

		// Create importer
		FbxImporter* _importer = FbxImporter::Create(sdkManager, "");

		// initialize fbx object
		if( !_importer->Initialize(fileProvided.c_str(), -1,
								   sdkManager->GetIOSettings()) ) {
			printf("Call to FBX::Initialize() failed. \nError: %s\n\n",
				   _importer->GetStatus().GetErrorString());
			exit(-1);
		}

		// create scene for FBX file
		std::string sceneName = fileProvided.substr(0, fileExtIndex);
		FbxScene* fbx_scene = FbxScene::Create(sdkManager, sceneName.c_str());
		// destroy importer after importing scene
		_importer->Import(fbx_scene);
		_importer->Destroy();
		// get global time info
		FbxTime::EMode g_timemode = fbx_scene->GetGlobalSettings().GetTimeMode();
		const float fr_rate = FbxTime::GetFrameRate(g_timemode);
		printf("Scene frame rate: %.3f(s)", fr_rate);

		// Triangulate all meshes (buggy)
		FbxGeometryConverter lGeomConverter(sdkManager);
        lGeomConverter.Triangulate(fbx_scene, true);

		// recursively walk thru scene and get asset information
		FbxNode* rootNode = fbx_scene->GetRootNode();
		if( rootNode ) {
			// create asset
			AssetFBX asset;
			printf("\n\n---------\nSkeleton\n---------\n\n");
			for( int i = 0; i < rootNode->GetChildCount(); i++ ) {
				FbxNode *_node = rootNode->GetChild(i);
				FbxNodeAttribute* attrib = _node->GetNodeAttribute();
				if( attrib ) {
					FbxNodeAttribute::EType type = attrib->GetAttributeType();
					if( type == fbxsdk::FbxNodeAttribute::eSkeleton ) {
						processSkeletonAsset(_node, 0, asset);
					}
				}
			}
			printf("\n\n---------\nAnimation\n---------\n\n");
			for (int i = 0; i < fbx_scene->GetSrcObjectCount<FbxAnimStack>(); i++) {
		        FbxAnimStack* lAnimStack = fbx_scene->GetSrcObject<FbxAnimStack>(i);
				FbxString lOutputString = lAnimStack->GetName();
				printf("Animation Stack Name: %s\n", lOutputString.Buffer());

				processAnimation(rootNode, lAnimStack, asset, fr_rate, 
								 lOutputString.Buffer());
				// export DDA (animations)
				asset.exportAnimation();
			}
			printf("\n\n----\nMesh\n----\n\n");
			for( int i = 0; i < rootNode->GetChildCount(); i++ ) {
				FbxNode *_node = rootNode->GetChild(i);
				FbxNodeAttribute* attrib = _node->GetNodeAttribute();
				if( attrib ) {
					FbxNodeAttribute::EType type = attrib->GetAttributeType();
					if( type == fbxsdk::FbxNodeAttribute::eMesh ) {
						processAsset(_node, asset);
						// export DDM (mesh)
						asset.exportMesh();
						// export DDB (skeleton)
						asset.exportSkeleton();
					}
				}
			}
		}
	}

	// destroy sdkManager when done
	sdkManager->Destroy();

	return 0;
}
