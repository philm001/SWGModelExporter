#include "stdafx.h"
#include "skt_parser.h"
#include "objects/animated_object.h"

using namespace std;
void skt_parser::section_begin(const std::string & name, uint8_t * data_ptr, size_t data_size, uint32_t depth)
{
  if (name == "SLODFORM" || (name == "SKTMFORM" && depth == 1))
  {
    m_skeleton = make_shared<Skeleton>();
    if (name == "SKTMFORM" && depth == 1)
    {
      m_skeleton->add_lod_level();
      m_lods_in_file = 1;
    }
  }
  else
  {
    string form_type(data_ptr, data_ptr + 4);
    if (form_type == "SKTM" && m_skeleton)
    {
      m_sections_received.reset();
      m_skeleton->add_lod_level();
    }
  }
}

void skt_parser::parse_data(const std::string & name, uint8_t * data_ptr, size_t data_size)
{
  base_buffer buffer(data_ptr, data_size);

  if (name == "0000INFO")
  {
    m_lods_in_file = buffer.read_uint16();
  }
  else if (name == "0002INFO")
  {
    m_joints_in_lod = buffer.read_uint32();
    m_sections_received[info] = buffer.end_of_buffer();
  }
  else if (name == "NAME" && m_sections_received[info])
  {
    uint32_t joint_num = 0;
    while (!buffer.end_of_buffer() && joint_num < m_joints_in_lod)
    {
      string joint_name = buffer.read_stringz();
      m_skeleton->add_bone(joint_name);
      joint_num++;
    }
    m_sections_received[sections::name] = buffer.end_of_buffer() && joint_num == m_joints_in_lod;
  }
  else if (name == "PRNT" && m_sections_received[sections::name])
  {
    uint32_t joint_num = 0;
    while (!buffer.end_of_buffer() && joint_num < m_joints_in_lod)
    {
      auto& bone = m_skeleton->get_bone(joint_num);
      bone.parent_idx = buffer.read_uint32();
      joint_num++;
    }
    m_sections_received[prnt] = buffer.end_of_buffer() && joint_num == m_joints_in_lod;
  }
  else if (name == "RPRE" && m_sections_received[prnt])
  {
    uint32_t joint_num = 0;
    while (!buffer.end_of_buffer() && joint_num < m_joints_in_lod)
    {
      auto& bone = m_skeleton->get_bone(joint_num);
      Geometry::Vector4 vec;
      vec.a = buffer.read_float();
      vec.x = buffer.read_float();
      vec.y = buffer.read_float();
      vec.z = buffer.read_float();
      bone.pre_rot_quaternion = vec;
      joint_num++;
    }
    m_sections_received[rpre] = buffer.end_of_buffer() && joint_num == m_joints_in_lod;
  }
  else if (name == "RPST" && m_sections_received[rpre])
  {
    uint32_t joint_num = 0;
    while (!buffer.end_of_buffer() && joint_num < m_joints_in_lod)
    {
      auto& bone = m_skeleton->get_bone(joint_num);
      Geometry::Vector4 vec;
      vec.a = buffer.read_float(); // w param of quat
      vec.x = buffer.read_float();
      vec.y = buffer.read_float();
      vec.z = buffer.read_float();
      bone.post_rot_quaternion = vec;
      joint_num++;
    }
    m_sections_received[rpst] = buffer.end_of_buffer() && joint_num == m_joints_in_lod;
  }
  else if (name == "BPTR" && m_sections_received[rpst])
  {
    uint32_t joint_num = 0;
    while (!buffer.end_of_buffer() && joint_num < m_joints_in_lod)
    {
      auto& bone = m_skeleton->get_bone(joint_num);
      Geometry::Vector3 vec;
      vec.x = buffer.read_float();
      vec.y = buffer.read_float();
      vec.z = buffer.read_float();
      bone.bind_pose_transform = vec;
      joint_num++;
    }
    m_sections_received[bptr] = buffer.end_of_buffer() && joint_num == m_joints_in_lod;
  }
  else if (name == "BPRO" && m_sections_received[bptr])
  {
    uint32_t joint_num = 0;
    while (!buffer.end_of_buffer() && joint_num < m_joints_in_lod)
    {
      auto& bone = m_skeleton->get_bone(joint_num);
      Geometry::Vector4 vec;
      vec.a = buffer.read_float(); // w param of quat
      vec.x = buffer.read_float();
      vec.y = buffer.read_float();
      vec.z = buffer.read_float();
      bone.bind_pose_rotation = vec;
      joint_num++;
    }
    m_sections_received[bpro] = buffer.end_of_buffer() && joint_num == m_joints_in_lod;
  }
  else if (name == "JROR" && m_sections_received[bpro])
  {
    uint32_t joint_num = 0;
    while (!buffer.end_of_buffer() && joint_num < m_joints_in_lod)
    {
      auto& bone = m_skeleton->get_bone(joint_num);
      bone.rotation_order = buffer.read_uint32();
      joint_num++;
    }
    m_sections_received[jror] = buffer.end_of_buffer() && joint_num == m_joints_in_lod;
  }
}
