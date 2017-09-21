#include "FBX_MeshFuncs.h"
#include <vector>

dd_array<vec2_f> DisplayCurve(FbxAnimCurve* pCurve);

/// \brief Process asset and export file w/ mesh and animation data
/// \param node FbxNode with mesh and anim information
void processAsset(FbxNode* node, AssetFBX &_asset)
{
	const char* nodeName = node->GetName();

	// create new mesh w/ id
	MeshFBX mesh(nodeName);

	FbxMesh* currmesh = (FbxMesh*)node->GetMesh();
	mesh.m_ctrlpnts.resize(currmesh->GetControlPointsCount());

	// get vertex positions
	for( size_t i = 0; i < mesh.m_ctrlpnts.size(); i++ ) {
		CtrlPnt &cp = mesh.m_ctrlpnts[i];
		cp.m_pos[0] = static_cast<float>(
			currmesh->GetControlPointAt((int)i).mData[0]);
		cp.m_pos[1] = static_cast<float>(
			currmesh->GetControlPointAt((int)i).mData[1]);
		cp.m_pos[2] = static_cast<float>(
			currmesh->GetControlPointAt((int)i).mData[2]);
	}

	// get skeleton blend information
	processSkeleton(currmesh, mesh, _asset.m_skeleton);
	// get mesh buffers
	processMesh(node, mesh);
	// get all materials
	_asset.m_matbin = processMats(node);
	// tag and construct ebo buffers
	dd_array<size_t> ebos = connectMatToMesh(node, mesh,
		(uint8_t)_asset.m_matbin.size());
	// finalize asset
	_asset.addMesh(mesh, ebos);

	printf("Mesh name ='%s'(%lu)\n",
		   mesh.m_id._str(), mesh.m_id.gethash());

	// export DDM (mesh), DDA (animations), and DDB (skeleton) files
	_asset.exportMesh();
	_asset.exportSkeleton();
}

/// \brief Process node to get skeleton heirarchy
/// \param _geom FbxNode with skeletal heirarchy information
void processSkeletonAsset(FbxNode *node, const size_t index, AssetFBX &_asset)
{
	bool recordbone = false;
	for( int i = 0; i < node->GetNodeAttributeCount(); i++ ) {
		FbxNodeAttribute* attrib = node->GetNodeAttributeByIndex(i);
		if (attrib->GetAttributeType() == fbxsdk::FbxNodeAttribute::eSkeleton) {
			recordbone = true;
		}
	}
	// skeleton joint/limb information
	SkelFbx &_sk = _asset.m_skeleton;
	size_t next_parent = _sk.m_numJoints - 1;
	if (recordbone) {
		JointFBX &this_j = _sk.m_joints[_sk.m_numJoints];
		this_j.m_name.set((char *)node->GetName());
		this_j.m_idx = _sk.m_numJoints;
		this_j.m_parent = index;
		printf("Skeleton Name: %s (%u : %u)\n",
				this_j.m_name._str(),
				this_j.m_idx,
				this_j.m_parent);
		// increment joint counter and index of next parent
		_sk.m_numJoints += 1;
		next_parent += 1;
	}

	// loop thru children nodes for skeleton
	for( int i = 0; i < node->GetChildCount(); i++ ) {
		processSkeletonAsset(node->GetChild(i), next_parent, _asset);
	}
}

/// \brief Get animation curve data from fbx
/// \param node FbxNode with animation information
/// \param animstack FbxAnimLayer with animation information
/// \param _asset AssetFBX that holds to be exported data
void getCurveInfo(
	FbxNode* node,
	FbxAnimLayer *animlayer,
	AssetFBX &_asset,
	const unsigned jnt_idx)
{
	FbxAnimCurve* lAnimCurve = NULL;
	dd_array<vec2_f> frames;

	// general curves
	lAnimCurve = node->LclTranslation.GetCurve(animlayer,
											   FBXSDK_CURVENODE_COMPONENT_X);
    if (lAnimCurve)
    {
        //printf("        TX\n");
        //DisplayCurve(lAnimCurve);
    }
    lAnimCurve = node->LclTranslation.GetCurve(animlayer,
											   FBXSDK_CURVENODE_COMPONENT_Y);
    if (lAnimCurve)
    {
        //printf("        TY\n");
        //DisplayCurve(lAnimCurve);
    }
    lAnimCurve = node->LclTranslation.GetCurve(animlayer,
											   FBXSDK_CURVENODE_COMPONENT_Z);
    if (lAnimCurve)
    {
        //printf("        TZ\n");
        //DisplayCurve(lAnimCurve);
    }

    lAnimCurve = node->LclRotation.GetCurve(animlayer,
											FBXSDK_CURVENODE_COMPONENT_X);
    if (lAnimCurve)
    {
        printf("        RX\n");
        DisplayCurve(lAnimCurve);
    }
    lAnimCurve = node->LclRotation.GetCurve(animlayer,
											FBXSDK_CURVENODE_COMPONENT_Y);
    if (lAnimCurve)
    {
        printf("        RY\n");
		frames = std::move(DisplayCurve(lAnimCurve));
		for (unsigned i = 0; i < frames.size(); i++) {
			printf("y%u-> %u:%.3f\n", 
					i, 
					(unsigned)frames[i].data[0], 
					frames[i].data[1]
			);
		}
    }
    lAnimCurve = node->LclRotation.GetCurve(animlayer,
											FBXSDK_CURVENODE_COMPONENT_Z);
    if (lAnimCurve)
    {
        printf("        RZ\n");
        DisplayCurve(lAnimCurve);
    }

    lAnimCurve = node->LclScaling.GetCurve(animlayer,
										   FBXSDK_CURVENODE_COMPONENT_X);
    if (lAnimCurve)
    {
        //printf("        SX\n");
        //DisplayCurve(lAnimCurve);
    }
    lAnimCurve = node->LclScaling.GetCurve(animlayer,
										   FBXSDK_CURVENODE_COMPONENT_Y);
    if (lAnimCurve)
    {
        //printf("        SY\n");
        //DisplayCurve(lAnimCurve);
    }
    lAnimCurve = node->LclScaling.GetCurve(animlayer,
										   FBXSDK_CURVENODE_COMPONENT_Z);
    if (lAnimCurve)
    {
        //printf("        SZ\n");
        //DisplayCurve(lAnimCurve);
    }
}

/// \brief Get animation data from fbx
/// \param node FbxNode with animation information
/// \param animstack FbxAnimLayer with animation information
/// \param _asset AssetFBX that holds to be exported data
void processAnimLayer(
	FbxNode* node, 
	FbxAnimLayer *animlayer, 
	AssetFBX &_asset,
	AnimClipFBX &clip )
{
	FbxString lOutputString;
    lOutputString = "     Node found: ";
	lOutputString += node->GetName();

	cbuff<32> node_name;
	node_name.set(node->GetName());
	bool bone_found = false;
	// only save animations from skeleton 
	for(unsigned i = 0; i < _asset.m_skeleton.m_numJoints && !bone_found; i++) {
		if( _asset.m_skeleton.m_joints[i].m_name == node_name) {
			bone_found = true;
			printf("%s\n", lOutputString.Buffer());
			getCurveInfo(node, animlayer, _asset, i);
		}
	}

	for (int i = 0; i < node->GetChildCount(); i++) {
		processAnimLayer(node->GetChild(i), animlayer, _asset, clip);
	}
}

/// \brief Get animation data from fbx
/// \param node FbxNode with animation information
/// \param animstack FbxAnimStack with animation information
/// \param _asset AssetFBX that holds to be exported data
void processAnimation(FbxNode* node, FbxAnimStack *animstack, AssetFBX &_asset)
{
	int nbAnimLayers = animstack->GetMemberCount<FbxAnimLayer>();
	FbxString lOutputString;
	_asset.m_clips.resize(nbAnimLayers);

    lOutputString = "Animation stack contains ";
    lOutputString += nbAnimLayers;
    lOutputString += " Animation Layer(s)\n";
	printf("%s\n", lOutputString.Buffer());

    for (int i = 0; i < nbAnimLayers; i++)
    {
        FbxAnimLayer* lAnimLayer = animstack->GetMember<FbxAnimLayer>(i);

        lOutputString = "AnimLayer ";
        lOutputString += i;
        lOutputString += "\n";
        printf("%s\n", lOutputString.Buffer());

        processAnimLayer(node, lAnimLayer, _asset, _asset.m_clips[i]);
    }
}

/// \brief Process node to get skeleton structure with tranforms
/// \param _geom FbxNode with geometry and cluster information
/// \param mesh MeshFBX mesh for modifing CtrlPnt data
/// \param _sk SkelFBX skeleton for indexing joints
void processSkeleton(FbxGeometry *_geom, MeshFBX &mesh, SkelFbx& _sk)
{
	int lSkinCount=0;
    int lClusterCount=0;
    FbxCluster* lCluster;
	bool joint_to_world = true;

	auto matrixToStr = [] (FbxAMatrix  &lMatrix)
	{
		std::string out;
		for (size_t i = 0; i < 4; i++) {
			FbxVector4 lRow = lMatrix.GetRow(i);
			char lRowValue[1024];

			snprintf(lRowValue, 1024, "%9.4f %9.4f %9.4f %9.4f\n",
				lRow[0], lRow[1], lRow[2], lRow[3]);
			out += std::string("        ") + std::string(lRowValue);
		}
		return out;
	};

    lSkinCount = _geom->GetDeformerCount(FbxDeformer::eSkin);
    for(size_t i = 0; i != (size_t)lSkinCount; i++) {
		lClusterCount = ((FbxSkin *)_geom->GetDeformer(i, FbxDeformer::eSkin))->
			GetClusterCount();
		for (size_t j = 0; j != (size_t)lClusterCount; j++) {
			printf("    Cluster %lu\n", i);
			lCluster = ((FbxSkin *)_geom->GetDeformer(i, FbxDeformer::eSkin))->
				GetCluster(j);
			// check blending mode
			const char* lClusterModes[] = { "Normalize", "Additive", "Total1" };
			printf("    Mode: %s\n", lClusterModes[lCluster->GetLinkMode()]);

			// get name (and joint index)
			std::string clustername = (char *)lCluster->GetLink()->GetName();
			int j_idx = -1;
			for (size_t k = 0; k < _sk.m_numJoints; k++) {
				std::string id = _sk.m_joints[k].m_name._str();
				if (id == clustername) {
					j_idx = _sk.m_joints[k].m_idx;
					break;
				}
			}
			if(lCluster->GetLink() != NULL) {
                printf("        Name: %s (%d)\n", clustername.c_str(), j_idx);
            }

			// get matrices
			FbxAMatrix lMatrix;
			if (joint_to_world) {
				lMatrix = lCluster->GetTransformMatrix(lMatrix);
				// printf("%s\n", matrixToStr(lMatrix).c_str());
				FbxVector4 pValue1 = lMatrix.GetT();
				_sk.m_wspos[0] = pValue1.mData[0];
				_sk.m_wspos[1] = pValue1.mData[1];
				_sk.m_wspos[2] = pValue1.mData[2];
				printf("            %.4f %.4f %.4f\t(global)\n",
					   pValue1.mData[0],
					   pValue1.mData[1],
					   pValue1.mData[2]);
				pValue1 = lMatrix.GetR();
				_sk.m_wsrot[0] = pValue1.mData[0];
				_sk.m_wsrot[1] = pValue1.mData[1];
				_sk.m_wsrot[2] = pValue1.mData[2];
				printf("            %.4f %.4f %.4f\t(global)\n",
					   pValue1.mData[0],
					   pValue1.mData[1],
					   pValue1.mData[2]);
				pValue1 = lMatrix.GetS();
				_sk.m_wsscl[0] = pValue1.mData[0];
				_sk.m_wsscl[1] = pValue1.mData[1];
				_sk.m_wsscl[2] = pValue1.mData[2];
				printf("            %.4f %.4f %.4f\t(global)\n",
					   pValue1.mData[0],
					   pValue1.mData[1],
					   pValue1.mData[2]);

				joint_to_world = false;
			}
            lMatrix = lCluster->GetTransformLinkMatrix(lMatrix);
			// printf("%s\n", matrixToStr(lMatrix).c_str());
			FbxVector4 pValue2 = lMatrix.GetT();
			_sk.m_joints[j_idx].m_lspos[0] = pValue2.mData[0];
			_sk.m_joints[j_idx].m_lspos[1] = pValue2.mData[1];
			_sk.m_joints[j_idx].m_lspos[2] = pValue2.mData[2];
			printf("            %.4f %.4f %.4f\t%s\n",
				   pValue2.mData[0],
				   pValue2.mData[1],
				   pValue2.mData[2],
			   	   clustername.c_str());
			pValue2 = lMatrix.GetR();
			_sk.m_joints[j_idx].m_lsrot[0] = pValue2.mData[0];
			_sk.m_joints[j_idx].m_lsrot[1] = pValue2.mData[1];
			_sk.m_joints[j_idx].m_lsrot[2] = pValue2.mData[2];
			printf("            %.4f %.4f %.4f\t%s\n",
				   pValue2.mData[0],
				   pValue2.mData[1],
				   pValue2.mData[2],
			   	   clustername.c_str());
			pValue2 = lMatrix.GetS();
			_sk.m_joints[j_idx].m_lsscl[0] = pValue2.mData[0];
			_sk.m_joints[j_idx].m_lsscl[1] = pValue2.mData[1];
			_sk.m_joints[j_idx].m_lsscl[2] = pValue2.mData[2];
			printf("            %.4f %.4f %.4f\t%s\n",
				   pValue2.mData[0],
				   pValue2.mData[1],
				   pValue2.mData[2],
			   	   clustername.c_str());

			// get control point blending weights and joint indices
			std::string lString1 = "        Link Indices: ";
            std::string lString2 = "        Weight Values: ";
			int lIndexCount = lCluster->GetControlPointIndicesCount();
            int* jnts = lCluster->GetControlPointIndices();
            double* weights = lCluster->GetControlPointWeights();

			// assign ctrl point weight
			for(size_t k = 0; k < (size_t)lIndexCount; k++) {
                lString1 += std::to_string(jnts[k]);
                lString2 += std::to_string((float)weights[k]);
				mesh.m_ctrlpnts[jnts[k]].addBWPair(j_idx, (float)weights[k]);
                if (k < (size_t)lIndexCount - 1) {
                    lString1 += ", ";
                    lString2 += ", ";
                }
            }
			// printf("%s\n", lString1.c_str());
			// printf("%s\n", lString2.c_str());
		}
	}
}

/// \brief Process mesh node to in-engine mesh structure
/// \param node FbxNode with mesh information
/// \param mesh mesh structure
void processMesh(FbxNode * node, MeshFBX &mesh)
{
	FbxMesh* currmesh = (FbxMesh*)node->GetMesh();
	mesh.m_triangles.resize(currmesh->GetPolygonCount());
	printf("Num tris: %u\n", (uint32_t)mesh.m_triangles.size());
	mesh.m_verts.resize(mesh.m_triangles.size() * 3);
	size_t vert_idx = 0;

	for( size_t i = 0; i < mesh.m_triangles.size(); i++ ) {
		vec3f norm[3];
		vec3f tang[3];
		vec3f uv[3];
		TriFBX &_tri = mesh.m_triangles[i];
		//printf("Tri #%u\n", (unsigned int)i);

		for( int j = 0; j < 3; j++ ) {
			// pull information for each vertex in the triangle
			size_t cp_idx = (size_t)currmesh->GetPolygonVertex(i, j);
			CtrlPnt &currCtrlPnt = mesh.m_ctrlpnts[cp_idx];

			// position
			setVec3(currCtrlPnt.m_pos, mesh.m_verts[vert_idx].m_pos);
			// uv
			getVertInfo<FbxGeometryElementUV*>(currmesh->GetElementUV(), cp_idx,
											   currmesh->GetTextureUVIndex(i, j),
											   uv[j]);
			setVec2(uv[j], mesh.m_verts[vert_idx].m_uv);
			// normals
			getVertInfo<FbxGeometryElementNormal*>(currmesh->GetElementNormal(),
												   cp_idx, vert_idx, norm[j]);
			setVec3(norm[j], mesh.m_verts[vert_idx].m_norm);
			// tangent
			getVertInfo<FbxGeometryElementTangent*>(currmesh->GetElementTangent(),
													cp_idx, vert_idx, tang[j]);
			setVec3(tang[j], mesh.m_verts[vert_idx].m_tang);
			// joints
			mesh.m_verts[vert_idx].m_joint[0] = currCtrlPnt.m_joint[0];
			mesh.m_verts[vert_idx].m_joint[1] = currCtrlPnt.m_joint[1];
			mesh.m_verts[vert_idx].m_joint[2] = currCtrlPnt.m_joint[2];
			mesh.m_verts[vert_idx].m_joint[3] = currCtrlPnt.m_joint[3];
			// blends
			mesh.m_verts[vert_idx].m_jblend[0] = currCtrlPnt.m_blend[0];
			mesh.m_verts[vert_idx].m_jblend[1] = currCtrlPnt.m_blend[1];
			mesh.m_verts[vert_idx].m_jblend[2] = currCtrlPnt.m_blend[2];
			mesh.m_verts[vert_idx].m_jblend[3] = currCtrlPnt.m_blend[3];
			/*
			printf("\t pos_%u: %f, %f, %f\n", j, currCtrlPnt.m_pos[0],
				currCtrlPnt.m_pos[1], currCtrlPnt.m_pos[2]);
			printf("\tnorm_%u: %f, %f, %f\n", j, norm[j][0], norm[j][1], norm[j][2]);
			printf("\t tan_%u: %f, %f, %f\n", j, tang[j][0], tang[j][1], tang[j][2]);
			printf("\t  uv_%u: %f, %f\n", j, uv[j][0], uv[j][1]);
			*/
			_tri.m_indices[j] = vert_idx;
			vert_idx += 1;
		}
	}
}

/// \brief Process connect material index to mesh (per-triangle)
/// \param node FbxNode with mesh information
/// \param mesh mesh structure
dd_array<size_t> connectMatToMesh(FbxNode * node,
								  MeshFBX & mesh,
								  const uint8_t num_mats)
{
	FbxLayerElementArrayTemplate<int>* mat_idxes;
	FbxGeometryElement::EMappingMode mat_mapmode = FbxGeometryElement::eNone;
	FbxMesh* currMesh = node->GetMesh();
	// counts number of triangle per material (ebo buffer)
	dd_array<size_t> tris_in_mat(num_mats);

	if( currMesh->GetElementMaterial() ) {
		mat_idxes = &(currMesh->GetElementMaterial()->GetIndexArray());
		mat_mapmode = currMesh->GetElementMaterial()->GetMappingMode();

		if( mat_idxes ) {
			switch( mat_mapmode ) {
				case FbxGeometryElement::eByPolygon:
				{
					if(mat_idxes->GetCount() == (int)mesh.m_triangles.size()) {
						for( size_t i = 0; i < mesh.m_triangles.size(); ++i ) {
							size_t mat_idx = mat_idxes->GetAt(i);
							mesh.m_triangles[i].m_mat_idx = mat_idx;
							tris_in_mat[mat_idx] += 1;
						}
					}
					break;
				}
				case FbxGeometryElement::eAllSame:
				{
					unsigned int mat_idx = mat_idxes->GetAt(0);
					for( unsigned int i = 0; i < mesh.m_triangles.size(); ++i ) {
						mesh.m_triangles[i].m_mat_idx = mat_idx;
						tris_in_mat[0] += 1;
					}
					break;
				}
				default:
					fprintf(stderr, "FBX Mat::Error::Invalid mapping index\n");
					break;
			}
		}
	}
	return tris_in_mat;
}

/// \brief Process all materials for mesh
/// \param node FbxNode with mesh information
/// \param mats material container
dd_array<MatFBX> processMats(FbxNode * node)
{
	size_t mat_count = node->GetMaterialCount();
	dd_array<MatFBX> mats(mat_count);

	auto transferD3 = [&] (FbxDouble3 &source, vec3f sink) {
		sink[0] = static_cast<float>(source.mData[0]);
		sink[1] = static_cast<float>(source.mData[1]);
		sink[2] = static_cast<float>(source.mData[2]);
	};

	auto flushSink = [] (vec3f &sink) {
		sink[0] = sink[1] = sink[2] = 0.f;
	};

	auto getTexChannels = [&](FbxProperty& _prop,
							  const int m_idx,
							  const MatType type) {
		// fill in textures and set flag
		int num_texts = _prop.GetSrcObjectCount<FbxTexture>();
		MatFBX &_m = mats[m_idx];
		if( num_texts > 0 ) {
			for(int i = 0; i < num_texts; i++) {
                FbxTexture* tex = _prop.GetSrcObject<FbxTexture>(i);
				FbxFileTexture* file_tex = FbxCast<FbxFileTexture>(tex);
                if(tex) {
					switch ((int)type) {
						case (int)MatType::DIFF:
							_m.m_diffmap.set(file_tex->GetFileName());
							_m.m_textypes |= MatType::DIFF;
							break;
						case (int)MatType::SPEC:
							_m.m_specmap.set(file_tex->GetFileName());
							_m.m_textypes |= MatType::SPEC;
							break;
						case (int)MatType::EMIT:
							_m.m_emitmap.set(file_tex->GetFileName());
							_m.m_textypes |= MatType::EMIT;
							break;
						case (int)MatType::NORMAL:
							_m.m_normmap.set(file_tex->GetFileName());
							_m.m_textypes |= MatType::NORMAL;
							break;
						default:
							break;
					}
				}
			}
		}
	};

	// grab material name and base atrributes
	for( size_t i = 0; i < mat_count; i++ ) {
		FbxSurfaceMaterial *surface_mat = node->GetMaterial(i);
		mats[i] = MatFBX(surface_mat->GetName());

		FbxDouble3 _d3;
		FbxDouble _d1;
		FbxProperty lProperty;

		if( surface_mat->GetClassId().Is(FbxSurfaceLambert::ClassId) ) {
			// ambient
			_d3 = reinterpret_cast<FbxSurfacePhong *>(surface_mat)->Ambient;
			transferD3(_d3, mats[i].m_ambient);

			// diffuse
			_d3 = reinterpret_cast<FbxSurfacePhong *>(surface_mat)->Diffuse;
			transferD3(_d3, mats[i].m_diffuse);
			lProperty = surface_mat->FindProperty(FbxSurfaceMaterial::sDiffuse);
			getTexChannels(lProperty, i, MatType::DIFF);

			// emissive
			_d3 = reinterpret_cast<FbxSurfacePhong *>(surface_mat)->Emissive;
			transferD3(_d3, mats[i].m_emmisive);
			lProperty = surface_mat->FindProperty(FbxSurfaceMaterial::sEmissive);
			getTexChannels(lProperty, i, MatType::EMIT);

			// normal
			lProperty = surface_mat->FindProperty(FbxSurfaceMaterial::sNormalMap);
			getTexChannels(lProperty, i, MatType::NORMAL);

			// specular
			lProperty = surface_mat->FindProperty(
				FbxSurfaceMaterial::sShininess);
			getTexChannels(lProperty, i, MatType::SPEC);

			// Transparency factor
			_d1 = reinterpret_cast<FbxSurfacePhong *>(
				surface_mat)->TransparencyFactor;
			mats[i].m_transfactor = static_cast<float>(_d1);

			flushSink(mats[i].m_specular);
			mats[i].m_specfactor = 0.f;
			mats[i].m_reflectfactor = 0.f;
		}
		if( surface_mat->GetClassId().Is(FbxSurfacePhong::ClassId) ) {
			// ambient
			_d3 = reinterpret_cast<FbxSurfacePhong *>(surface_mat)->Ambient;
			transferD3(_d3, mats[i].m_ambient);

			// diffuse
			_d3 = reinterpret_cast<FbxSurfacePhong *>(surface_mat)->Diffuse;
			transferD3(_d3, mats[i].m_diffuse);
			lProperty = surface_mat->FindProperty(FbxSurfaceMaterial::sDiffuse);
			getTexChannels(lProperty, i, MatType::DIFF);

			// emissive
			_d3 = reinterpret_cast<FbxSurfacePhong *>(surface_mat)->Emissive;
			transferD3(_d3, mats[i].m_emmisive);
			lProperty = surface_mat->FindProperty(FbxSurfaceMaterial::sEmissive);
			getTexChannels(lProperty, i, MatType::EMIT);

			// normal
			lProperty = surface_mat->FindProperty(FbxSurfaceMaterial::sNormalMap);
			getTexChannels(lProperty, i, MatType::NORMAL);

			// Transparency factor
			_d1 = reinterpret_cast<FbxSurfacePhong *>(
				surface_mat)->TransparencyFactor;
			mats[i].m_transfactor = static_cast<float>(_d1);

			// specular
			_d3 = reinterpret_cast<FbxSurfacePhong *>(surface_mat)->Specular;
			transferD3(_d3, mats[i].m_specular);
			lProperty = surface_mat->FindProperty(
				FbxSurfaceMaterial::sShininess);
			getTexChannels(lProperty, i, MatType::SPEC);

			// reflection factor
			_d1 = reinterpret_cast<FbxSurfacePhong *>(
				surface_mat)->ReflectionFactor;
			mats[i].m_reflectfactor = static_cast<float>(_d1);

			// specular factor
			_d1 = reinterpret_cast<FbxSurfacePhong *>(
				surface_mat)->SpecularFactor;
			mats[i].m_specfactor = static_cast<float>(_d1);
		}
	}
	return mats;
}

/// \brief Extract animation data from curves
dd_array<vec2_f> DisplayCurve(FbxAnimCurve* pCurve)
{
	dd_array<vec2_f> output;

    static const char* interpolation[] = { "?", "constant", "linear", "cubic"};
    static const char* constantMode[] =  { "?", "Standard", "Next" };
    static const char* cubicMode[] =     { "?", "Auto", "Auto break", "Tcb",
										   "User", "Break", "User break" };
    static const char* tangentWVMode[] = { "?", "None", "Right", "Next left" };

	auto InterpolationFlagToIndex = [](int flags)
	{
		if( (flags & FbxAnimCurveDef::eInterpolationConstant) ==
		 	FbxAnimCurveDef::eInterpolationConstant ) { return 1; }
	    if( (flags & FbxAnimCurveDef::eInterpolationLinear) ==
		 	FbxAnimCurveDef::eInterpolationLinear ) { return 2; }
		if( (flags & FbxAnimCurveDef::eInterpolationCubic) ==
		 	FbxAnimCurveDef::eInterpolationCubic ) { return 3; }
	    return 0;
	};

	auto ConstantmodeFlagToIndex = [](int flags)
	{
    	if( (flags & FbxAnimCurveDef::eConstantStandard) ==
			FbxAnimCurveDef::eConstantStandard ) { return 1; }
    	if( (flags & FbxAnimCurveDef::eConstantNext) ==
			FbxAnimCurveDef::eConstantNext ) { return 2; }
    	return 0;
	};

	auto TangentmodeFlagToIndex = [](int flags)
	{
	    if( (flags & FbxAnimCurveDef::eTangentAuto) ==
			FbxAnimCurveDef::eTangentAuto ) { return 1; }
	    if( (flags & FbxAnimCurveDef::eTangentAutoBreak) ==
			FbxAnimCurveDef::eTangentAutoBreak ) { return 2; }
	    if( (flags & FbxAnimCurveDef::eTangentTCB) ==
			FbxAnimCurveDef::eTangentTCB ) { return 3; }
	    if( (flags & FbxAnimCurveDef::eTangentUser) ==
			FbxAnimCurveDef::eTangentUser ) { return 4; }
	    if( (flags & FbxAnimCurveDef::eTangentGenericBreak) ==
			FbxAnimCurveDef::eTangentGenericBreak ) { return 5; }
	    if( (flags & FbxAnimCurveDef::eTangentBreak) ==
			FbxAnimCurveDef::eTangentBreak ) { return 6; }
	    return 0;
	};

	auto TangentweightFlagToIndex = [](int flags)
	{
	    if( (flags & FbxAnimCurveDef::eWeightedNone) ==
			FbxAnimCurveDef::eWeightedNone ) { return 1; }
	    if( (flags & FbxAnimCurveDef::eWeightedRight) ==
			FbxAnimCurveDef::eWeightedRight ) { return 2; }
	    if( (flags & FbxAnimCurveDef::eWeightedNextLeft) ==
			FbxAnimCurveDef::eWeightedNextLeft ) { return 3; }
	    return 0;
	};

	auto TangentVelocityFlagToIndex = [](int flags)
	{
	    if( (flags & FbxAnimCurveDef::eVelocityNone) ==
			FbxAnimCurveDef::eVelocityNone ) { return 1; }
	    if( (flags & FbxAnimCurveDef::eVelocityRight) ==
			FbxAnimCurveDef::eVelocityRight ) { return 2; }
	    if( (flags & FbxAnimCurveDef::eVelocityNextLeft) ==
			FbxAnimCurveDef::eVelocityNextLeft ) { return 3; }
	    return 0;
	};

    FbxTime   lKeyTime;
    float   lKeyValue;
    char    lTimeString[256];
    FbxString lOutputString;
    int     lCount;

	int lKeyCount = pCurve->KeyGetCount();
	output.resize(lKeyCount);

    for(lCount = 0; lCount < lKeyCount; lCount++)
    {
        lKeyValue = static_cast<float>(pCurve->KeyGetValue(lCount));
        lKeyTime  = pCurve->KeyGetTime(lCount);

        lOutputString = "            Key Time: ";
        lOutputString += lKeyTime.GetTimeString(lTimeString, FbxUShort(256));
        lOutputString += ".... Key Value: ";
        lOutputString += lKeyValue;
        lOutputString += " [ ";
		lOutputString += interpolation[ InterpolationFlagToIndex(pCurve->KeyGetInterpolation(lCount)) ];
		
		// get value and frame number
		char* frame_num = lKeyTime.GetTimeString(lTimeString, FbxUShort(256));
		output[lCount].data[0] = std::strtod(frame_num, nullptr);
		output[lCount].data[1] = lKeyValue;

        if ((pCurve->KeyGetInterpolation(lCount)&FbxAnimCurveDef::eInterpolationConstant) == FbxAnimCurveDef::eInterpolationConstant)
        {
            lOutputString += " | ";
            lOutputString += constantMode[ ConstantmodeFlagToIndex(pCurve->KeyGetConstantMode(lCount)) ];
        }
        else if ((pCurve->KeyGetInterpolation(lCount)&FbxAnimCurveDef::eInterpolationCubic) == FbxAnimCurveDef::eInterpolationCubic)
        {
            lOutputString += " | ";
            lOutputString += cubicMode[ TangentmodeFlagToIndex(pCurve->KeyGetTangentMode(lCount)) ];
            lOutputString += " | ";
			lOutputString += tangentWVMode[ TangentweightFlagToIndex(pCurve->KeyGet(lCount).GetTangentWeightMode()) ];
            lOutputString += " | ";
			lOutputString += tangentWVMode[ TangentVelocityFlagToIndex(pCurve->KeyGet(lCount).GetTangentVelocityMode()) ];
        }
        lOutputString += " ]";
        lOutputString += "\n";
        //printf("%s\n", lOutputString.Buffer());
	}
	return output;
}
