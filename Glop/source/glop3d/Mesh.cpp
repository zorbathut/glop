// Includes
#include "../../include/glop3d/Mesh.h"
#include "../../include/Base.h"
#include "../../include/OpenGl.h"

Mesh::Mesh(int num_points_allocated, int num_triangles_allocated, bool has_normals,
           bool has_colors, bool has_textures)
: num_points_allocated_(num_points_allocated),
  num_triangles_allocated_(num_triangles_allocated),
  num_points_(0), num_triangles_(0),
  points_((float*)malloc(3 * num_points_allocated * sizeof(float))),
  normals_(has_normals? (float*)malloc(3 * num_points_allocated * sizeof(float)) : 0),
  colors_(has_colors? (float*)malloc(4 * num_points_allocated * sizeof(float)) : 0),
  texture_coords_(has_textures? (float*)malloc(2 * num_points_allocated * sizeof(float)) : 0),
  vertex_indices_((short*)malloc(3 * num_triangles_allocated * sizeof(short))),
  textures_(has_textures? (const Texture**)malloc(num_triangles_allocated * sizeof(void*)) : 0),
  num_groups_allocated_(0), num_groups_(-1),
  group_start_(0), group_size_(0),
  group_is_fixed_normal_(0), group_is_fixed_color_(0),
  radius_(-1) {}

Mesh::~Mesh() {
  free(points_);
  if (normals_ != 0) free(normals_);
  if (colors_ != 0) free(colors_);
  if (texture_coords_ != 0) free(texture_coords_);
  free(vertex_indices_);
  if (textures_ != 0) free(textures_);
  if (group_start_ != 0) free(group_start_);
  if (group_size_ != 0) free(group_size_);
  if (group_is_fixed_normal_ != 0) free(group_is_fixed_normal_);
  if (group_is_fixed_color_ != 0) free(group_is_fixed_color_);
}

void Mesh::AllocatePoints(int num_points) {
  ASSERT(num_points >= num_points_);
  num_points_allocated_ = num_points;
  points_ = (float*)realloc(points_, 3 * num_points * sizeof(float));
  if (normals_ != 0)
    normals_ = (float*)realloc(normals_, 3 * num_points * sizeof(float));
  if (colors_ != 0)
    colors_ = (float*)realloc(colors_, 3 * num_points * sizeof(float));
  if (texture_coords_ != 0)
    texture_coords_ = (float*)realloc(texture_coords_, 2 * num_points * sizeof(float));
}

void Mesh::AllocateTriangles(int num_triangles) {
  ASSERT(num_triangles >= num_triangles_);
  num_triangles_allocated_ = num_triangles;
  vertex_indices_ = (short*)realloc(vertex_indices_, 3 * num_triangles * sizeof(short));
  if (textures_ != 0)
    textures_ = (const Texture**)realloc(textures_, num_triangles * sizeof(void*));
}

int Mesh::AddPoint(const Point3 &position) {
  memcpy(&points_[3*num_points_], position.GetData(), 3 * sizeof(float));
  return (num_points_++);
}

int Mesh::AddPoint(const Point3 &position, const Vec3 &normal, const Color &color,
                   float tu, float tv) {
  memcpy(&points_[3*num_points_], position.GetData(), 3 * sizeof(float));
  memcpy(&normals_[3*num_points_], normal.GetData(), 3 * sizeof(float));
  memcpy(&colors_[4*num_points_], color.GetData(), 4 * sizeof(float));
  texture_coords_[2*num_points_+0] = tu;
  texture_coords_[2*num_points_+1] = tv;
  return (num_points_++);
}

int Mesh::AddTriangle(int v1, int v2, int v3, const Texture *texture) {
  vertex_indices_[3*num_triangles_] = v1;
  vertex_indices_[3*num_triangles_+1] = v2;
  vertex_indices_[3*num_triangles_+2] = v3;
  if (textures_ != 0)
    textures_[num_triangles_] = texture;
  return (num_triangles_++);
}

void Mesh::SetPoint(int index, const Point3 &position) {
  memcpy(&points_[3*index], position.GetData(), 3 * sizeof(float));
}

void Mesh::SetNormal(int index, const Vec3 &normal) {
  memcpy(&normals_[3*index], normal.GetData(), 3 * sizeof(float));
}

void Mesh::SetColor(int index, const Color &color) {
  memcpy(&colors_[4*index], color.GetData(), 4 * sizeof(float));
}

void Mesh::SetTextureCoords(int index, float tu, float tv) {
  texture_coords_[2*index] = tu;
  texture_coords_[2*index+1] = tv;
}

void Mesh::SetVertexIndices(int triangle, int v1, int v2, int v3) {
  vertex_indices_[3*triangle] = v1;
  vertex_indices_[3*triangle+1] = v2;
  vertex_indices_[3*triangle+2] = v3;
}

void Mesh::SetTexture(int triangle, const Texture *texture) {
  textures_[triangle] = texture;
}

float Mesh::GetRadius() const {
  if (radius_ < 0) {
    radius_ = 0;
    for (int i = 0; i < num_points_; i++) {
      float temp = points_[3*i+0]*points_[3*i+0] + points_[3*i+1]*points_[3*i+1] +
                   points_[3*i+2]*points_[3*i+2];
      if (radius_ < temp)
        radius_ = temp;
    }
  }
  return radius_;
}

void Mesh::Render() const {
  // Rebuild rendering data if necessary
  if (num_groups_ == -1) {
    short group_v0;
    bool need_new_group = true;
    num_groups_ = 0;

    // Loop through each triangle
    for (int i = 0; i < num_triangles_; i++) {
      // Begin a new rendering group if appropriate
      if (need_new_group) {
        // Allocate data if necessary
        if (num_groups_allocated_ <= num_groups_) {
          num_groups_allocated_ = 3 + num_groups_allocated_*2;
          group_start_ = (short*)realloc(group_start_, num_groups_allocated_*sizeof(short));
          group_size_ = (short*)realloc(group_size_, num_groups_allocated_*sizeof(short));
          if (colors_ != 0) {
            group_is_fixed_color_ = (bool*)realloc(group_is_fixed_color_,
              num_groups_allocated_*sizeof(bool));
          }
          if (normals_ != 0) {
            group_is_fixed_normal_ = (bool*)realloc(group_is_fixed_normal_,
              num_groups_allocated_*sizeof(bool));
          }
        }

        // Begin the new group
        group_start_[num_groups_] = 3*i;
        if (group_is_fixed_color_ != 0)
          group_is_fixed_color_[num_groups_] = true;
        if (group_is_fixed_normal_ != 0)
          group_is_fixed_normal_[num_groups_] = true;
        group_v0 = vertex_indices_[3*i];
        need_new_group = false;
      }

      // Process each vertex in this triangle
      for (int j = 0; j < 3; j++) {
        int v = vertex_indices_[3*i+j];
        if (colors_ != 0 && colors_[v] != colors_[group_v0])
          group_is_fixed_color_[num_groups_] = false;
        if (normals_ != 0 && normals_[v] != normals_[group_v0])
          group_is_fixed_normal_[num_groups_] = false;
      }

      // End this group if appropriate
      if (i == num_triangles_-1 || (textures_ != 0 && textures_[i+1] != textures_[i])) {
        group_size_[num_groups_] = 3*(i+1) - group_start_[num_groups_];
        num_groups_++;
        need_new_group = true;
      }
    }
  }

  // Render the mesh. First specific the vertex array pointers.
	glVertexPointer(3, GL_FLOAT, 0, points_);
  if (normals_ != 0) glNormalPointer(GL_FLOAT, 0, normals_);
  if (colors_ != 0) glColorPointer(4, GL_FLOAT, 0, colors_);
  if (texture_coords_ != 0) glTexCoordPointer(2, GL_FLOAT, 0, texture_coords_);
    
	// Loop through each rendering group
  for (int i = 0; i < num_groups_; i++) {
    int start = group_start_[i], v0 = vertex_indices_[start];

		// Set the texture
    if (textures_ != 0 && textures_[start] != 0) {
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      GlUtils::SetTexture(textures_[start]);
    } else {
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
      GlUtils::SetNoTexture();
    }

    // Set the normal
    if (normals_ != 0 && !group_is_fixed_normal_[i]) {
      glEnableClientState(GL_NORMAL_ARRAY);
    } else {
      glDisableClientState(GL_NORMAL_ARRAY);
      if (normals_ != 0) glNormal3fv(&normals_[3*v0]);
    }

    // Set the color
    if (colors_ != 0 && !group_is_fixed_color_[i]) {
      glEnableClientState(GL_COLOR_ARRAY);
    } else {
      glDisableClientState(GL_COLOR_ARRAY);
      if (colors_ != 0) glColor4fv(&colors_[4*v0]);
    }

		// Render the group
    glDrawElements(GL_TRIANGLES, group_size_[i], GL_UNSIGNED_SHORT,
      vertex_indices_ + group_start_[i]);
	}
}

Mesh *StockMeshes::NewBoxMesh(float width, float height, float depth, const Color &color,
                              const Texture *texture) {
  float x = width/2, y = height/2, z = depth/2;
  Mesh *result = new Mesh(24, 12, true, true, true);

  // Top face
  result->AddPoint(Point3(-x/2,  y/2, -z/2), Point3(0, 1, 0), color, 0, 1);
  result->AddPoint(Point3(-x/2,  y/2,  z/2), Point3(0, 1, 0), color, 0, 0);
  result->AddPoint(Point3( x/2,  y/2,  z/2), Point3(0, 1, 0), color, 1, 0);
  result->AddPoint(Point3( x/2,  y/2, -z/2), Point3(0, 1, 0), color, 1, 1);
  result->AddTriangle(0, 1, 2, texture);
  result->AddTriangle(0, 2, 3, texture);

  // Front face
  result->AddPoint(Point3(-x/2, -y/2,  -z/2), Point3(0, 0, -1), color, 0, 1);
  result->AddPoint(Point3(-x/2,  y/2,  -z/2), Point3(0, 0, -1), color, 0, 0);
  result->AddPoint(Point3( x/2,  y/2,  -z/2), Point3(0, 0, -1), color, 1, 0);
  result->AddPoint(Point3( x/2, -y/2,  -z/2), Point3(0, 0, -1), color, 1, 1);
  result->AddTriangle(4, 5, 6, texture);
  result->AddTriangle(4, 6, 7, texture);

  // Left face
  result->AddPoint(Point3(-x/2, -y/2,  z/2), Point3(-1, 0, 0), color, 0, 1);
  result->AddPoint(Point3(-x/2,  y/2,  z/2), Point3(-1, 0, 0), color, 0, 0);
  result->AddPoint(Point3(-x/2,  y/2, -z/2), Point3(-1, 0, 0), color, 1, 0);
  result->AddPoint(Point3(-x/2, -y/2, -z/2), Point3(-1, 0, 0), color, 1, 1);
  result->AddTriangle(8, 9, 10, texture);
  result->AddTriangle(8, 10, 11, texture);

  // Back face
  result->AddPoint(Point3( x/2, -y/2,  z/2), Point3(0, 0, 1), color, 0, 1);
  result->AddPoint(Point3( x/2,  y/2,  z/2), Point3(0, 0, 1), color, 0, 0);
  result->AddPoint(Point3(-x/2,  y/2,  z/2), Point3(0, 0, 1), color, 1, 0);
  result->AddPoint(Point3(-x/2, -y/2,  z/2), Point3(0, 0, 1), color, 1, 1);
  result->AddTriangle(12, 13, 14, texture);
  result->AddTriangle(12, 14, 15, texture);

  // Right face
  result->AddPoint(Point3( x/2, -y/2, -z/2), Point3(1, 0, 0), color, 0, 1);
  result->AddPoint(Point3( x/2,  y/2, -z/2), Point3(1, 0, 0), color, 0, 0);
  result->AddPoint(Point3( x/2,  y/2,  z/2), Point3(1, 0, 0), color, 1, 0);
  result->AddPoint(Point3( x/2, -y/2,  z/2), Point3(1, 0, 0), color, 1, 1);
  result->AddTriangle(16, 17, 18, texture);
  result->AddTriangle(16, 18, 19, texture);

  // Bottom face
  result->AddPoint(Point3(-x/2, -y/2,  z/2), Point3(0, -1, 0), color, 0, 1);
  result->AddPoint(Point3(-x/2, -y/2, -z/2), Point3(0, -1, 0), color, 0, 0);
  result->AddPoint(Point3( x/2, -y/2, -z/2), Point3(0, -1, 0), color, 1, 0);
  result->AddPoint(Point3( x/2, -y/2,  z/2), Point3(0, -1, 0), color, 1, 1);
  result->AddTriangle(20, 21, 22, texture);
  result->AddTriangle(20, 22, 23, texture);

  return result;
}
