#pragma once

#include "geometry_common.h"
#include "objects/base_object.h"
//
// Static object
//
class Static_mesh;

class Static_object
{
private:
  std::string m_object_name;

  Geometry::Sphere m_bounding_sphere;
  Geometry::Box m_bounding_box;
  std::vector<std::shared_ptr<Static_mesh>> m_lods;
};

class Vertex
{
private:
  Geometry::Point m_position;
  Geometry::Point m_normal;
  Graphics::Color m_color;
  std::vector<Graphics::Tex_coord> m_tex_coords;
};

class Triangle
{
private:
  std::array<uint16_t, 3> m_vert_indexes;
};

class Submesh
{
private:
  std::string m_shader_name;
  std::vector<Vertex> m_vertices;
  std::vector<Triangle> m_triangles;
};

class Static_mesh
{
private:
  std::vector<Submesh> m_meshes;
};