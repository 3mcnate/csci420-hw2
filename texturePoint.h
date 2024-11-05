#ifndef TEXTURE_POINT_H
#define TEXTURE_POINT_H

#include "point.h"

struct TexturePoint : public Point
{
  float s;
  float t;

  TexturePoint(float x, float y, float z, float s, float t) : Point(x, y, z), s(s), t(t) {}

  TexturePoint(const Point& p, float s, float t) : Point(p), s(s), t(t) {}

};

#endif