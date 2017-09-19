#include "FBX_Utility.h"
#include <map>
#include <fstream>

size_t numTabs = 0;

/// \brief Split mesh into subsequent EBO buffers based on shared materials
/// \param _mesh MeshFBX struct that contains all mesh buffer data
/// \param ebo_data Lists buffer sizes for each material (sorted by index)
void AssetFBX::addMesh(MeshFBX & _mesh, dd_array<size_t> &ebo_data)
{
	m_id.set(_mesh.m_id._str());
	m_verts = std::move(_mesh.m_verts);

	printf("Materials\n");
	// resize ebo and material buffer (materials w/ no triangles are skipped)
	dd_array<uint32_t> valid_mats(ebo_data.size());
	std::map<uint32_t, uint32_t> mat_idx;
	uint8_t idx = 0;
	for( size_t i = 0; i < ebo_data.size(); i++ ) {
		printf("ebo #%lu: %lu\n", i, ebo_data[i]);
		if( ebo_data[i] > 0 ) {
			printf("\t%s \tregistered at: %u\n", m_matbin[i].m_id._str(), idx);
			valid_mats[idx] = i;
			mat_idx[(uint8_t)i] = idx;
			idx += 1;
		}
	}
	// set valid materials
	if( idx < m_matbin.size() ) {
		dd_array<MatFBX> temp(idx);
		for( size_t i = 0; i < idx; i++ ) {
			temp[i] = std::move(m_matbin[valid_mats[i]]);
		}
		m_matbin = std::move(temp);
	}
	// size ebo bins for meshes
	m_ebos.resize(m_matbin.size());
	for(auto& idx_pair : mat_idx) {
		m_ebos[idx_pair.second].indices.resize(ebo_data[idx_pair.first]);
	}
	// separate triangles into correct bins
	dd_array<uint32_t> idx_tracker(m_matbin.size());
	for( size_t i = 0; i < _mesh.m_triangles.size(); i++ ) {
		size_t key = _mesh.m_triangles[i].m_mat_idx;
		size_t value = mat_idx[key];
		m_ebos[value].indices[idx_tracker[value]][0] =
			_mesh.m_triangles[i].m_indices[0];
		m_ebos[value].indices[idx_tracker[value]][1] =
			_mesh.m_triangles[i].m_indices[1];
		m_ebos[value].indices[idx_tracker[value]][2] =
			_mesh.m_triangles[i].m_indices[2];
		idx_tracker[value] += 1;
	}
	/*
	for (size_t i = 0; i < m_ebos.size(); i++) {
		printf("Ebo: #%lu\n", i);
		for (size_t j = 0; j < m_ebos[i].indices.size(); j++) {
			printf("\t%u, %u, %u\n", m_ebos[i].indices[j][0],
				m_ebos[i].indices[j][1],
				m_ebos[i].indices[j][2]);
		}
	}
	//*/
}

/// \brief Export skeleton to format specified by dd_entity_map.txt
void AssetFBX::exportSkeleton()
{
	char lineBuff[256];
	std::fstream outfile;
	outfile.open("skeleton.ddb", std::ofstream::out);

	// check file is open
	if (outfile.bad()) {
		printf("Could not open skeleton output file\n" );
		return;
	}

	// size
	snprintf(lineBuff, sizeof(lineBuff), "%u", m_skeleton.m_numJoints);
	outfile << "<size>\n" << lineBuff << "\n</size>\n";
	// joint to world space
	outfile << "<global>\n";
	snprintf(lineBuff, sizeof(lineBuff), "p %.3f %.3f %.3f\n",
			 m_skeleton.m_wspos[0],
			 m_skeleton.m_wspos[1],
			 m_skeleton.m_wspos[2]);
	outfile << lineBuff;
	snprintf(lineBuff, sizeof(lineBuff), "r %.3f %.3f %.3f\n",
			 m_skeleton.m_wsrot[0],
			 m_skeleton.m_wsrot[1],
			 m_skeleton.m_wsrot[2]);
	outfile << lineBuff;
	snprintf(lineBuff, sizeof(lineBuff), "s %.3f %.3f %.3f\n",
			 m_skeleton.m_wsscl[0],
			 m_skeleton.m_wsscl[1],
			 m_skeleton.m_wsscl[2]);
	outfile << lineBuff;
	outfile << "</global>\n";
	// joints
	for (size_t i = 0; i < m_skeleton.m_numJoints; i++) {
		outfile << "<joint>\n";
		JointFBX& _j = m_skeleton.m_joints[i];
		snprintf(lineBuff, sizeof(lineBuff), "%s %u %u\n",
				 _j.m_name._str(),
			 	 _j.m_idx,
			 	 _j.m_parent);
		outfile << lineBuff;
		snprintf(lineBuff, sizeof(lineBuff), "p %.3f %.3f %.3f\n",
				 _j.m_lspos[0],
				 _j.m_lspos[1],
				 _j.m_lspos[2]);
		outfile << lineBuff;
		snprintf(lineBuff, sizeof(lineBuff), "r %.3f %.3f %.3f\n",
				 _j.m_lsrot[0],
				 _j.m_lsrot[1],
				 _j.m_lsrot[2]);
		outfile << lineBuff;
		snprintf(lineBuff, sizeof(lineBuff), "s %.3f %.3f %.3f\n",
				 _j.m_lsscl[0],
				 _j.m_lsscl[1],
				 _j.m_lsscl[2]);
		outfile << lineBuff;
		outfile << "</joint>\n";
	}
}

/// \brief Export mesh to format specified by dd_entity_map.txt
void AssetFBX::exportMesh()
{
	char lineBuff[256];
	snprintf(lineBuff, sizeof(lineBuff), "%s.ddm", m_id._str());
	std::fstream outfile;
	outfile.open(lineBuff, std::ofstream::out);

	// check file is open
	if (outfile.bad()) {
		printf("Could not open mesh output file\n" );
		return;
	}

	// name
	snprintf(lineBuff, sizeof(lineBuff), "%s", m_id._str());
	outfile << "<name>\n" << lineBuff << "\n</name>\n";
	// buffer sizes
	outfile << "<buffer>\n";
	snprintf(lineBuff, sizeof(lineBuff), "v %lu", m_verts.size());
	outfile << lineBuff << "\n";
	snprintf(lineBuff, sizeof(lineBuff), "e %lu", m_ebos.size());
	outfile << lineBuff << "\n";
	snprintf(lineBuff, sizeof(lineBuff), "m %lu", m_matbin.size());
	outfile << lineBuff << "\n";
	outfile << "</buffer>\n";
	// material data
	for (size_t i = 0; i < m_matbin.size(); i++) {
		MatFBX& _m = m_matbin[i];
		outfile << "<material>\n";
		snprintf(lineBuff, sizeof(lineBuff), "n %s", _m.m_id._str());
		outfile << lineBuff << "\n";
		if (static_cast<bool>(_m.m_textypes & MatType::DIFF)) {
			snprintf(lineBuff, sizeof(lineBuff), "D %s", _m.m_diffmap._str());
			outfile << lineBuff << "\n";
		}
		if (static_cast<bool>(_m.m_textypes & MatType::NORMAL)) {
			snprintf(lineBuff, sizeof(lineBuff), "N %s", _m.m_normmap._str());
			outfile << lineBuff << "\n";
		}
		if (static_cast<bool>(_m.m_textypes & MatType::SPEC)) {
			snprintf(lineBuff, sizeof(lineBuff), "S %s", _m.m_specmap._str());
			outfile << lineBuff << "\n";
		}
		if (static_cast<bool>(_m.m_textypes & MatType::ROUGH)) {
			snprintf(lineBuff, sizeof(lineBuff), "R %s", _m.m_roughmap._str());
			outfile << lineBuff << "\n";
		}
		if (static_cast<bool>(_m.m_textypes & MatType::METAL)) {
			snprintf(lineBuff, sizeof(lineBuff), "M %s", _m.m_metalmap._str());
			outfile << lineBuff << "\n";
		}
		if (static_cast<bool>(_m.m_textypes & MatType::EMIT)) {
			snprintf(lineBuff, sizeof(lineBuff), "E %s", _m.m_emitmap._str());
			outfile << lineBuff << "\n";
		}
		if (static_cast<bool>(_m.m_textypes & MatType::AO)) {
			snprintf(lineBuff, sizeof(lineBuff), "A %s", _m.m_aomap._str());
			outfile << lineBuff << "\n";
		}
		// vector properties
		snprintf(lineBuff, sizeof(lineBuff), "a %.3f %.3f %.3f",
				 _m.m_ambient[0], _m.m_ambient[1], _m.m_ambient[2]);
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "d %.3f %.3f %.3f",
				 _m.m_diffuse[0], _m.m_diffuse[1], _m.m_diffuse[2]);
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "s %.3f %.3f %.3f",
				 _m.m_specular[0], _m.m_specular[1], _m.m_specular[2]);
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "e %.3f %.3f %.3f",
				 _m.m_emmisive[0], _m.m_emmisive[1], _m.m_emmisive[2]);
		outfile << lineBuff << "\n";

		// float properties
		snprintf(lineBuff, sizeof(lineBuff), "x %.3f", _m.m_transfactor);
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "y %.3f", _m.m_reflectfactor);
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "z %.3f", _m.m_specfactor);
		outfile << lineBuff << "\n";

		outfile << "</material>\n";
	}
	// vertex data
	outfile << "<vertex>\n";
	for (size_t i = 0; i < m_verts.size(); i++) {
		snprintf(lineBuff, sizeof(lineBuff), "v %.3f %.3f %.3f",
				 m_verts[i].m_pos[0],
				 m_verts[i].m_pos[1],
				 m_verts[i].m_pos[2]);
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "n %.3f %.3f %.3f",
				 m_verts[i].m_norm[0],
				 m_verts[i].m_norm[1],
				 m_verts[i].m_norm[2]);
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "t %.3f %.3f %.3f",
				 m_verts[i].m_tang[0],
				 m_verts[i].m_tang[1],
				 m_verts[i].m_tang[2]);
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "u %.3f %.3f",
				 m_verts[i].m_uv[0],
				 m_verts[i].m_uv[1]);
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "j %u %u %u %u",
				 m_verts[i].m_joint[0],
				 m_verts[i].m_joint[1],
				 m_verts[i].m_joint[2],
				 m_verts[i].m_joint[3]);
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "b %.3f %.3f %.3f %.3f",
				 m_verts[i].m_jblend[0],
				 m_verts[i].m_jblend[1],
				 m_verts[i].m_jblend[2],
				 m_verts[i].m_jblend[3]);
		outfile << lineBuff << "\n";
	}
	outfile << "</vertex>\n";

	// ebo data
	for (size_t i = 0; i < m_ebos.size(); i++) {
		outfile << "<ebo>\n";
		EboMesh& _e = m_ebos[i];
		snprintf(lineBuff, sizeof(lineBuff), "s %lu", _e.indices.size() * 3);
		outfile << lineBuff << "\n";
		outfile << "m " << i << "\n"; // material index
		for (size_t j = 0; j < _e.indices.size(); j++) {
			snprintf(lineBuff, sizeof(lineBuff), "- %u %u %u",
					 _e.indices[j][0], _e.indices[j][1], _e.indices[j][2]);
			outfile << lineBuff << "\n";
		}
		outfile << "</ebo>\n";
	}

	outfile.close();
}
