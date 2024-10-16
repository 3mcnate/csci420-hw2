#include <memory>
#include <algorithm>
#include <vector>
#include <iostream>

using namespace std;

struct Vertex
{
  Vertex(float x, float y, float z, float color)
  {
    this->x = x;
    this->y = y;
    this->z = z;
    this->color = color;
  }

  float x;
  float y;
  float z;
  float color;
};

Vertex createVertex(float xBias, float zBias, float resolution, unsigned char pixel, int x, int y)
{
  float x1 = (-x + xBias) / (resolution - 1);
  float y1 = (float)pixel / 255.0f;
  float z1 = (-y + zBias) / (resolution - 1);
  float color = (pixel + 10.0) / 265.0;

  return Vertex(x1, y1, z1, color);
}

/**
 * @return Vector of vertices in order: {center, left, right, up, down}
 */
vector<Vertex> createVertexAndNeighbors(std::unique_ptr<ImageIO> &image, float xBias, float zBias, float resolution, int x, int y, int h, int w)
{
  vector<Vertex> result;

  // center
  result.push_back(createVertex(xBias, zBias, resolution, image->getPixel(x, y, 0), x, y));

  // left
  int left = x == 0 ? x : x - 1;
  result.push_back(createVertex(xBias, zBias, resolution, image->getPixel(left, y, 0), left, y));

  // right
  int right = x == w - 1 ? x : x + 1;
  result.push_back(createVertex(xBias, zBias, resolution, image->getPixel(right, y, 0), right, y));

  // up
  int up = y == h - 1 ? y : y + 1;
  result.push_back(createVertex(xBias, zBias, resolution, image->getPixel(x, up, 0), x, up));

  // down
  int down = y == 0 ? y : y - 1;
  result.push_back(createVertex(xBias, zBias, resolution, image->getPixel(x, down, 0), x, down));

  return result;
}

void addVertexToVBO(std::unique_ptr<float[]> &vertexVBO, std::unique_ptr<float[]> &colorVBO, int &vIndex, int &cIndex, Vertex vertex)
{
  vertexVBO[vIndex++] = vertex.x;
  vertexVBO[vIndex++] = vertex.y;
  vertexVBO[vIndex++] = vertex.z;

  std::fill_n(colorVBO.get() + cIndex, 4, vertex.color);
  cIndex += 4;
}

void addLineToVBO(std::unique_ptr<float[]> &vertexVBO, std::unique_ptr<float[]> &colorVBO, int &vIndex, int &cIndex, Vertex vertex1, Vertex vertex2)
{
  addVertexToVBO(vertexVBO, colorVBO, vIndex, cIndex, vertex1);
  addVertexToVBO(vertexVBO, colorVBO, vIndex, cIndex, vertex2);
}

void addTriangleToVBO(std::unique_ptr<float[]> &vertexVBO, std::unique_ptr<float[]> &colorVBO, int &vIndex, int &cIndex, Vertex vertex1, Vertex vertex2, Vertex vertex3)
{
  addVertexToVBO(vertexVBO, colorVBO, vIndex, cIndex, vertex1);
  addVertexToVBO(vertexVBO, colorVBO, vIndex, cIndex, vertex2);
  addVertexToVBO(vertexVBO, colorVBO, vIndex, cIndex, vertex3);
}

void addVertexPositionToVBO(float *vertexVBO, int vIndex, const Vertex &vertex)
{
  vertexVBO[vIndex++] = vertex.x;
  vertexVBO[vIndex++] = vertex.y;
  vertexVBO[vIndex++] = vertex.z;
}

void addVertexColorToVBO(float *colorVBO, int cIndex, const Vertex &vertex)
{
  std::fill_n(colorVBO + cIndex, 4, vertex.color);
}

void addTriangleVerticesToVBO(const vector<float *> &vbos, int &vIndex, const vector<Vertex> &vertices1, const vector<Vertex> &vertices2, const vector<Vertex> &vertices3)
{
  for (int i = 0; i < 5; ++i)
  {
    addVertexPositionToVBO(vbos[i], vIndex, vertices1[i]);
  }

  vIndex += 3;

  for (int i = 0; i < 5; ++i)
  {
    addVertexPositionToVBO(vbos[i], vIndex, vertices2[i]);
  }

  vIndex += 3;

  for (int i = 0; i < 5; ++i)
  {
    addVertexPositionToVBO(vbos[i], vIndex, vertices3[i]);
  }

  vIndex += 3;
}

void printVertex(std::unique_ptr<float[]> &vertexVBO, const char *name, int index)
{
  int i = index * 3;
  cout << name << "[" << index << "]: " << vertexVBO[i] << " " << vertexVBO[i + 1] << " " << vertexVBO[i + 2] << endl;
}