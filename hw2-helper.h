#ifndef HW2_H
#define HW2_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include "openGLHeader.h"
#include "imageIO.h"
#include "point.h"
#include "texturePoint.h"

// Contains the control points of the spline.
struct Spline
{
  int numControlPoints;
  Point *points;
} spline;

void loadSpline(char *argv)
{
  FILE *fileSpline = fopen(argv, "r");
  if (fileSpline == NULL)
  {
    printf("Cannot open file %s.\n", argv);
    exit(1);
  }

  // Read the number of spline control points.
  fscanf(fileSpline, "%d\n", &spline.numControlPoints);
  printf("Detected %d control points.\n", spline.numControlPoints);

  // Allocate memory.
  spline.points = (Point *)malloc(spline.numControlPoints * sizeof(Point));
  // Load the control points.
  for (int i = 0; i < spline.numControlPoints; i++)
  {
    if (fscanf(fileSpline, "%f %f %f",
               &spline.points[i].x,
               &spline.points[i].y,
               &spline.points[i].z) != 3)
    {
      printf("Error: incorrect number of control points in file %s.\n", argv);
      exit(1);
    }
  }
}

// Multiply C = A * B, where A is a m x p matrix, and B is a p x n matrix.
// All matrices A, B, C must be pre-allocated (say, using malloc or similar).
// The memory storage for C must *not* overlap in memory with either A or B.
// That is, you **cannot** do C = A * C, or C = C * B. However, A and B can overlap, and so C = A * A is fine, as long as the memory buffer for A is not overlaping in memory with that of C.
// Very important: All matrices are stored in **column-major** format.
// Example. Suppose
//      [ 1 8 2 ]
//  A = [ 3 5 7 ]
//      [ 0 2 4 ]
//  Then, the storage in memory is
//   1, 3, 0, 8, 5, 2, 2, 7, 4.
void MultiplyMatrices(int m, int p, int n, const float *A, const float *B, float *C)
{
  for (int i = 0; i < m; i++)
  {
    for (int j = 0; j < n; j++)
    {
      float entry = 0.0;
      for (int k = 0; k < p; k++)
        entry += A[k * m + i] * B[j * p + k];
      C[m * j + i] = entry;
    }
  }
}

int initTexture(const char *imageFilename, GLuint textureHandle)
{
  // Read the texture image.
  ImageIO img;
  ImageIO::fileFormatType imgFormat;
  ImageIO::errorType err = img.load(imageFilename, &imgFormat);

  if (err != ImageIO::OK)
  {
    printf("Loading texture from %s failed.\n", imageFilename);
    return -1;
  }

  // Check that the number of bytes is a multiple of 4.
  if (img.getWidth() * img.getBytesPerPixel() % 4)
  {
    printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
    return -1;
  }

  // Allocate space for an array of pixels.
  int width = img.getWidth();
  int height = img.getHeight();
  unsigned char *pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

  // Fill the pixelsRGBA array with the image pixels.
  memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
  for (int h = 0; h < height; h++)
    for (int w = 0; w < width; w++)
    {
      // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
      pixelsRGBA[4 * (h * width + w) + 0] = 0;   // red
      pixelsRGBA[4 * (h * width + w) + 1] = 0;   // green
      pixelsRGBA[4 * (h * width + w) + 2] = 0;   // blue
      pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

      // set the RGBA channels, based on the loaded image
      int numChannels = img.getBytesPerPixel();
      for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
        pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
    }

  // Bind the texture.
  glBindTexture(GL_TEXTURE_2D, textureHandle);

  // Initialize the texture.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

  // Generate the mipmaps for this texture.
  glGenerateMipmap(GL_TEXTURE_2D);

  // Set the texture parameters.
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // Query support for anisotropic texture filtering.
  GLfloat fLargest;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
  printf("Max available anisotropic samples: %f\n", fLargest);
  // Set anisotropic texture filtering.
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

  // Query for any errors.
  GLenum errCode = glGetError();
  if (errCode != 0)
  {
    printf("Texture initialization error. Error code: %d.\n", errCode);
    return -1;
  }

  // De-allocate the pixel array -- it is no longer needed.
  delete[] pixelsRGBA;

  return 0;
}

/**
 *  Calculates and returns M * C, the Catumull-Rom spline basis times control point matrix
 *  @param i index of the first of 4 control points in the spline
 */
float *calcMC(int i, bool debug = false)
{
  // s = 1/2
  float s = 0.5f;

  /*
    M = [  -s  2-s  s-2    s  ]
        [  2s  s-3  3-2s  -s  ]
        [  -s   0    s     0  ]
        [  0    1    0     0  ]
  */
  vector<float> M = {-s, 2 * s, -s, 0,
                     2 - s, s - 3, 0, 1,
                     s - 2, 3 - (2 * s), s, 0,
                     s, -s, 0, 0};

  /*
    C = [ x1  y1  z1 ]
        [ x2  y2  z2 ]
        [ x3  y3  z3 ]
        [ x4  y4  z4 ]
  */
  int n = spline.numControlPoints;
  vector<float> C;
  for (int j = 0; j < 4; ++j)
  {
    int index = (i + j) % n;
    C.push_back(spline.points[index].x);
    if (debug)
    {
      printf("index: %2d %s\n", index, spline.points[index].toString().c_str());
    }
  }

  for (int j = 0; j < 4; ++j)
  {
    int index = (i + j) % n;
    C.push_back(spline.points[index].y);
  }

  for (int j = 0; j < 4; ++j)
  {
    int index = (i + j) % n;
    C.push_back(spline.points[index].z);
  }

  float *R = new float[12];
  MultiplyMatrices(4, 4, 3, M.data(), C.data(), R);
  return R;
}

Point calculatePosition(float u, float *R)
{
  vector<float> posMatrix = {u * u * u, u * u, u, 1};
  float p[3];
  MultiplyMatrices(1, 4, 3, posMatrix.data(), R, p);
  return Point(p);
}

// NOTE: returns normalized tangent
Point calculateTangent(float u, float *R)
{
  vector<float> tangentMatrix = {3 * u * u, 2 * u, 1, 0};
  float t[3];
  MultiplyMatrices(1, 4, 3, tangentMatrix.data(), R, t);
  return Point(t).normalize();
}

void normalizeVector(float *v)
{
  float magnitude = sqrt((v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]));
  v[0] /= magnitude;
  v[1] /= magnitude;
  v[2] /= magnitude;
}

void addPointToVector(const Point &p, vector<float> &vec)
{
  vec.push_back(p.x);
  vec.push_back(p.y);
  vec.push_back(p.z);
}

void addColorToVector(const Point &p, vector<float> &vec)
{
  vec.push_back(p.x);
  vec.push_back(p.y);
  vec.push_back(p.z);
  vec.push_back(1);
}

void addColorToVector(float r, float g, float b, vector<float> &vec)
{
  vec.push_back(r);
  vec.push_back(g);
  vec.push_back(b);
  vec.push_back(1);
}

void addTriangleToVector(const Point &p1, const Point &p2, const Point &p3, vector<float> &vec)
{
  addPointToVector(p1, vec);
  addPointToVector(p2, vec);
  addPointToVector(p3, vec);
}

void addTriangleColorsToVector(const Point &p1, const Point &p2, const Point &p3, vector<float> &vec)
{
  addColorToVector(p1, vec);
  addColorToVector(p2, vec);
  addColorToVector(p3, vec);
}

void addTriangleNormalToVector(const Point &n, vector<float> &vec)
{
  addPointToVector(n, vec);
  addPointToVector(n, vec);
  addPointToVector(n, vec);
}

void addUVTriangleToVector(const TexturePoint &p1, const TexturePoint &p2, const TexturePoint &p3, vector<float> &UVVec)
{
  UVVec.push_back(p1.s);
  UVVec.push_back(p1.t);
  UVVec.push_back(p2.s);
  UVVec.push_back(p2.t);
  UVVec.push_back(p3.s);
  UVVec.push_back(p3.t);
}

void addTextureTriangleToVectors(const TexturePoint &p1, const TexturePoint &p2, const TexturePoint &p3, vector<float> &positionVec, vector<float> &UVVec)
{
  addTriangleToVector(p1, p2, p3, positionVec);
  addUVTriangleToVector(p1, p2, p3, UVVec);
}

void setTextureUnit(GLuint program, GLint unit)
{
  // select texture unit affected by subsequent texture calls
  glActiveTexture(unit);

  // get a handle to the “textureImage” shader variable
  GLint h_textureImage = glGetUniformLocation(program, "textureImage");

  // deem the shader variable “textureImage” to read from texture unit “unit”
  glUniform1i(h_textureImage, unit - GL_TEXTURE0);
}

void setUniform3f(GLuint program, const char* varName, float x, float y, float z)
{
  GLint location = glGetUniformLocation(program, varName);
  glUniform3f(location, x, y, z);
}

void setUniform4f(GLuint program, const char* varName, float x, float y, float z, float w)
{
  GLint location = glGetUniformLocation(program, varName);
  glUniform4f(location, x, y, z, w);
}

#endif
