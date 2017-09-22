#include "FBX_Utility.h"
#include <map>
#include <fstream>

size_t numTabs = 0;

/// \brief Split mesh into subsequent EBO buffers based on shared materials
/// \param _mesh MeshFBX struct that contains all mesh buffer data
/// \param ebo_data Lists buffer sizes for each material (sorted by index)
void AssetFBX::addMesh(MeshFBX & _mesh, dd_array<size_t> &ebo_data)
{
	m_id.set(_mesh.m_id.str());
	m_verts = std::move(_mesh.m_verts);

	printf("Materials\n");
	// resize ebo and material buffer (materials w/ no triangles are skipped)
	dd_array<uint32_t> valid_mats(ebo_data.size());
	std::map<uint32_t, uint32_t> mat_idx;
	uint8_t idx = 0;
	for( size_t i = 0; i < ebo_data.size(); i++ ) {
		printf("ebo #%lu: %lu\n", i, ebo_data[i]);
		if( ebo_data[i] > 0 ) {
			printf("\t%s \tregistered at: %u\n", m_matbin[i].m_id.str(), idx);
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
		m_ebos[value].indices[idx_tracker[value]] =
			_mesh.m_triangles[i].m_indices;
		idx_tracker[value] += 1;
	}
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
			 m_skeleton.m_wspos.x(),
			 m_skeleton.m_wspos.y(),
			 m_skeleton.m_wspos.z());
	outfile << lineBuff;
	snprintf(lineBuff, sizeof(lineBuff), "r %.3f %.3f %.3f\n",
			 m_skeleton.m_wsrot.x(),
			 m_skeleton.m_wsrot.y(),
			 m_skeleton.m_wsrot.z());
	outfile << lineBuff;
	snprintf(lineBuff, sizeof(lineBuff), "s %.3f %.3f %.3f\n",
			 m_skeleton.m_wsscl.x(),
			 m_skeleton.m_wsscl.y(),
			 m_skeleton.m_wsscl.z());
	outfile << lineBuff;
	outfile << "</global>\n";
	// joints
	for (size_t i = 0; i < m_skeleton.m_numJoints; i++) {
		outfile << "<joint>\n";
		JointFBX& _j = m_skeleton.m_joints[i];
		snprintf(lineBuff, sizeof(lineBuff), "%s %u %u\n",
				 _j.m_name.str(),
			 	 _j.m_idx,
			 	 _j.m_parent);
		outfile << lineBuff;
		snprintf(lineBuff, sizeof(lineBuff), "p %.3f %.3f %.3f\n",
				 _j.m_lspos.x(),
				 _j.m_lspos.y(),
				 _j.m_lspos.z());
		outfile << lineBuff;
		snprintf(lineBuff, sizeof(lineBuff), "r %.3f %.3f %.3f\n",
				 _j.m_lsrot.x(),
				 _j.m_lsrot.y(),
				 _j.m_lsrot.z());
		outfile << lineBuff;
		snprintf(lineBuff, sizeof(lineBuff), "s %.3f %.3f %.3f\n",
				 _j.m_lsscl.x(),
				 _j.m_lsscl.y(),
				 _j.m_lsscl.z());
		outfile << lineBuff;
		outfile << "</joint>\n";
	}
}

/// \brief Export mesh to format specified by dd_entity_map.txt
void AssetFBX::exportMesh()
{
	char lineBuff[256];
	snprintf(lineBuff, sizeof(lineBuff), "%s.ddm", m_id.str());
	std::fstream outfile;
	outfile.open(lineBuff, std::ofstream::out);

	// check file is open
	if (outfile.bad()) {
		printf("Could not open mesh output file\n" );
		return;
	}

	// name
	snprintf(lineBuff, sizeof(lineBuff), "%s", m_id.str());
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
		snprintf(lineBuff, sizeof(lineBuff), "n %s", _m.m_id.str());
		outfile << lineBuff << "\n";
		if (static_cast<bool>(_m.m_textypes & MatType::DIFF)) {
			snprintf(lineBuff, sizeof(lineBuff), "D %s", _m.m_diffmap.str());
			outfile << lineBuff << "\n";
		}
		if (static_cast<bool>(_m.m_textypes & MatType::NORMAL)) {
			snprintf(lineBuff, sizeof(lineBuff), "N %s", _m.m_normmap.str());
			outfile << lineBuff << "\n";
		}
		if (static_cast<bool>(_m.m_textypes & MatType::SPEC)) {
			snprintf(lineBuff, sizeof(lineBuff), "S %s", _m.m_specmap.str());
			outfile << lineBuff << "\n";
		}
		if (static_cast<bool>(_m.m_textypes & MatType::ROUGH)) {
			snprintf(lineBuff, sizeof(lineBuff), "R %s", _m.m_roughmap.str());
			outfile << lineBuff << "\n";
		}
		if (static_cast<bool>(_m.m_textypes & MatType::METAL)) {
			snprintf(lineBuff, sizeof(lineBuff), "M %s", _m.m_metalmap.str());
			outfile << lineBuff << "\n";
		}
		if (static_cast<bool>(_m.m_textypes & MatType::EMIT)) {
			snprintf(lineBuff, sizeof(lineBuff), "E %s", _m.m_emitmap.str());
			outfile << lineBuff << "\n";
		}
		if (static_cast<bool>(_m.m_textypes & MatType::AO)) {
			snprintf(lineBuff, sizeof(lineBuff), "A %s", _m.m_aomap.str());
			outfile << lineBuff << "\n";
		}
		// vector properties
		snprintf(lineBuff, sizeof(lineBuff), "a %.3f %.3f %.3f",
				 _m.m_ambient.x(), _m.m_ambient.y(), _m.m_ambient.z());
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "d %.3f %.3f %.3f",
				 _m.m_diffuse.x(), _m.m_diffuse.y(), _m.m_diffuse.z());
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "s %.3f %.3f %.3f",
				 _m.m_specular.x(), _m.m_specular.y(), _m.m_specular.z());
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "e %.3f %.3f %.3f",
				 _m.m_emmisive.x(), _m.m_emmisive.y(), _m.m_emmisive.z());
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
				 m_verts[i].m_pos.x(),
				 m_verts[i].m_pos.y(),
				 m_verts[i].m_pos.z());
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "n %.3f %.3f %.3f",
				 m_verts[i].m_norm.x(),
				 m_verts[i].m_norm.y(),
				 m_verts[i].m_norm.z());
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "t %.3f %.3f %.3f",
				 m_verts[i].m_tang.x(),
				 m_verts[i].m_tang.y(),
				 m_verts[i].m_tang.z());
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "u %.3f %.3f",
				 m_verts[i].m_uv.x(),
				 m_verts[i].m_uv.y());
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "j %u %u %u %u",
				 m_verts[i].m_joint.x(),
				 m_verts[i].m_joint.y(),
				 m_verts[i].m_joint.z(),
				 m_verts[i].m_joint.w());
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "b %.3f %.3f %.3f %.3f",
				 m_verts[i].m_jblend.x(),
				 m_verts[i].m_jblend.y(),
				 m_verts[i].m_jblend.z(),
				 m_verts[i].m_jblend.w());
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
					 _e.indices[j].x(), _e.indices[j].y(), _e.indices[j].z());
			outfile << lineBuff << "\n";
		}
		outfile << "</ebo>\n";
	}

	outfile.close();
}

/// \brief Export animation to format specified by dd_entity_map.txt
void AssetFBX::exportAnimation()
{
	for(unsigned i = 0; i < m_clips.size(); i++) {
		char lineBuff[256];
		snprintf(lineBuff, sizeof(lineBuff), "%s.dda", m_clips[i].m_id.str());
		std::fstream outfile;
		outfile.open(lineBuff, std::ofstream::out);
	
		// check file is open
		if (outfile.bad()) {
			printf("Could not open animation output file\n" );
			return;
		}

		// framerate
		snprintf(lineBuff, sizeof(lineBuff), "%.3f", m_clips[i].m_framerate);
		outfile << "<framerate>\n" << lineBuff << "\n</framerate>\n";
		// buffer sizes
		outfile << "<buffer>\n";
		snprintf(lineBuff, sizeof(lineBuff), "j %u", m_clips[i].m_joints);
		outfile << lineBuff << "\n";
		snprintf(lineBuff, sizeof(lineBuff), "f %lu", m_clips[i].m_clip.size());
		outfile << lineBuff << "\n";
		outfile << "</buffer>\n";	

		// animation data
		outfile << "<animation>\n";
		for(unsigned j = 0; j < m_clips[i].m_joints; j++) {
			snprintf(lineBuff, sizeof(lineBuff), "- %u", j);
			outfile << lineBuff << "\n";
			for(auto& p : m_clips[i].m_clip) {
				snprintf(lineBuff, sizeof(lineBuff), "r %.3f %.3f %.3f",
					p.second.pose[j].rot.x(),
					p.second.pose[j].rot.y(),
					p.second.pose[j].rot.z());
				outfile << lineBuff << "\n";
			}
		}
		outfile << "</animation>\n";

		outfile.close();
	}
}