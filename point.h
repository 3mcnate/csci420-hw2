#include <sstream>

#ifndef POINT_H
#define POINT_H

// Represents one spline control point.
struct Point
{
  float x, y, z;

  Point(float x, float y, float z) : x(x), y(y), z(z) {}

  Point(float *point)
  {
    x = point[0];
    y = point[1];
    z = point[2];
  }

  string toString() const
  {
    stringstream ss;
    ss << "(" << x << ", " << y << ", " << z << ")";
    return ss.str();
  }

  Point &print()
  {
    cout << toString() << endl;
    return *this;
  }

  Point operator+(const Point &rhs)
  {
    return Point(x + rhs.x, y + rhs.y, z + rhs.z);
  }

  Point operator-()
  {
    return Point(-x, -y, -z);
  }

  Point &normalize()
  {
    float magnitude = sqrt((x * x) + (y * y) + (z * z));
    x /= magnitude;
    y /= magnitude;
    z /= magnitude;
    return *this;
  }
};

Point crossProduct(const Point &A, const Point &B)
{
  float Cx = A.y * B.z - A.z * B.y;
  float Cy = A.z * B.x - A.x * B.z;
  float Cz = A.x * B.y - A.y * B.x;
  return Point(Cx, Cy, Cz);
}

Point operator*(const float a, const Point &p)
{
  return Point(a * p.x, a * p.y, a * p.z);
}

#endif