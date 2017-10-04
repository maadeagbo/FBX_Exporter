#include <cstdlib>
#include <cstdio>
#include <string>

#include "FBX_Utility.h"
#include "FBX_MeshFuncs.h"

#include <fbxsdk.h>

enum class FbxExport
{
	NONE = 0x0,
	MESH = 0x1,
	ANIMATION = 0x2,
	SKELETON = 0x4,
	VICON = 0x8
};
template<>
struct EnableBitMaskOperators<FbxExport> { static const bool enable = true; };

int main(const int argc, const char** argv)
{
	const char* help = "Arguments for export: "
		"\n\t-f\tfile to read"
		"\n\t-m\tmesh"
		"\n\t-a\tanimation"
		"\n\t-s\tskeleton"
		"\n\t-v\tvicon\n";
	cbuff<512> fbx_to_read;
	FbxExport exportFlags = FbxExport::NONE;

	if( argc == 1 ) {
		printf("%s\n", help);
		return 0;
	}
	else {
		for (unsigned i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-f") == 0) {			// grab fbx file
				fbx_to_read.set(argv[i + 1]);
			}
			else if(strcmp(argv[i], "-m") == 0) {		// mesh flag
				exportFlags |= FbxExport::MESH;
			}
			else if(strcmp(argv[i], "-a") == 0) {		// animation flag
				exportFlags |= FbxExport::ANIMATION;
			}
			else if(strcmp(argv[i], "-s") == 0) {		// skeleton flag
				exportFlags |= FbxExport::SKELETON;
			}
			else if(strcmp(argv[i], "-vicon") == 0) {	// vicon flag
				exportFlags |= FbxExport::VICON;
			}
		}
	}
	// create fbx manager object
	FbxManager* sdkManager = FbxManager::Create();

	// initialize settings
	FbxIOSettings *_IOSettings = FbxIOSettings::Create(sdkManager, IOSROOT);
	sdkManager->SetIOSettings(_IOSettings);

	// process input
	for( size_t i = 1; i < (size_t)argc; i++ ) {
		std::string fileProvided = fbx_to_read.str();
		size_t fileExtIndex = fileProvided.find_last_of('.');
		if( fileExtIndex <= 0 || fileProvided == "") {
			printf("Provide valid FBX file\n");
			exit(-1);
		}
		std::string fileExt = fileProvided.substr(fileExtIndex + 1);
		if( fileExt == "FBX" || fileExt == "fbx") {
			printf("Parsing %s\n\n", argv[i]);
		}
		else {
			printf("Invalid FBX file\n");
			exit(-1);
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
