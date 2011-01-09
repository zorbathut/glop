// TODO: Add raw accessor for textures and add comments
//       Add comments
//       Allow removing points
//       glBlend
//
// A simple triangle-based mesh that can be edited and rendered. All data is stored in flat arrays
// and rendering is done with vertex arrays.

#ifndef GLOP_GLOP3D_MESH_H__
#define GLOP_GLOP3D_MESH_H__

#ifndef GLOP_LEAN_AND_MEAN

// Includes
#include "../Base.h"
#include "../Color.h"
#include "Point3.h"

// Class declarations
class Texture;

// Mesh class definition
class Mesh {
 public:
  // Constructors
  Mesh(int num_points_allocated, int num_triangles_allocated, bool has_normals, bool has_colors,
       bool has_textures);
  ~Mesh();

  // Point and triangle allocation
  int GetNumPoints() const {return num_points_;}
  int GetNumPointsAllocated() const {return num_points_allocated_;}
  void AllocatePoints(int num_points);
  int GetNumTriangles() const {return num_triangles_;}
  int GetNumTrianglesAllocated() const {return num_triangles_allocated_;}
  void AllocateTriangles(int num_triangles);

  // Mutation
  int AddPoint(const Point3 &position);
  int AddPoint(const Point3 &position, const Vec3 &normal, const Color &color, float tu, float tv);
  int AddTriangle(int v1, int v2, int v3, const Texture *texture = 0);
  void SetPoint(int index, const Point3 &position);
  void SetNormal(int index, const Vec3 &normal);
  void SetColor(int index, const Color &color);
  void SetTextureCoords(int index, float tu, float tv);
  void SetVertexIndices(int triangle, int v1, int v2, int v3);
  void SetTexture(int triangle, const Texture *texture);

  // Raw data accessors
  const float *points() const {return points_;}
  float *points() {return points_;}
  const float *normals() const {return normals_;}
  float *normals() {return normals_;}
  const float *colors() const {return colors_;}
  float *colors() {return colors_;}
  const float *texture_coords() const {return texture_coords_;}
  float *texture_coords() {return texture_coords_;}
  const Texture * const *textures() const {return textures_;}
  const Texture **textures() {return textures_;}
  const short *vertex_indices() const {return vertex_indices_;}
  short *vertex_indices() {return vertex_indices_;}

  // "Natural" accessors
  Point3 GetPoint(int index) const {
    const float *base = &points_[3*index];
    return Point3(base[0], base[1], base[2]);
  }
  Point3 GetNormal(int index) const {
    const float *base = &normals_[3*index];
    return Vec3(base[0], base[1], base[2]);
  }
  Color GetColor(int index) const {
    const float *base = &colors_[4*index];
    return Color(base[0], base[1], base[2], base[3]);
  }
  float GetTextureU(int index) const {return texture_coords_[2*index];}
  float GetTextureV(int index) const {return texture_coords_[2*index+1];}
  const Texture *GetTexture(int triangle) const {return textures_[triangle];}
  int GetVertexIndex(int triangle, int vertex) const {return vertex_indices_[3*triangle+vertex];}

  // Utilities
  float GetRadius() const;
  void DirtyRadius() {radius_ = -1;}
  void Render(const Viewpoint &viewpoint);
  void Render() const;
  void DirtyRendering() {num_groups_ = -1;}

 private:
  // Mesh definition
  short num_points_allocated_, num_triangles_allocated_;
  short num_points_, num_triangles_;
  float *points_;
  float *normals_;
  float *colors_;
  float *texture_coords_;
  short *vertex_indices_;
  const Texture **textures_;

  // Rendering data
  mutable short num_groups_allocated_, num_groups_;
  mutable short *group_start_, *group_size_;
  mutable bool *group_is_fixed_normal_, *group_is_fixed_color_;

  // Other data
  mutable float radius_;
  DISALLOW_EVIL_CONSTRUCTORS(Mesh);
};

// StockMeshes class definition.
class StockMeshes {
 public:
  static Mesh *NewBoxMesh(float width, float height, float depth, const Color &color,
                          const Texture *texture = 0);
  static Mesh *NewCubeMesh(float size, const Color &color, const Texture *texture = 0) {
    return NewBoxMesh(size, size, size, color, texture);
  }
  static Mesh *NewSphereMesh(float width, float height, float depth, const Color &color,
                             int precision, const Texture *texture = 0);
  static Mesh *NewSphereMesh(float radius, const Color &color, int precision,
                             const Texture *texture = 0) {
    return NewSphereMesh(radius*2, radius*2, radius*2, color, precision, texture);
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(StockMeshes);
};

#endif // GLOP_LEAN_AND_MEAN

#endif // GLOP_GLOP3D_MESH_H__
