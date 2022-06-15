#ifndef POINT_H
#define POINT_H

#include <cmath>

class Point
{
public:
    float x;
    float y;

    Point() : x(0), y(0) {}
    Point(float x, float y) : x(x), y(y) {}
    ~Point() {}

    float abs() const { return sqrt(x * x + y * y); }

    Point &operator+=(const Point &other) { return x += other.x, y += other.y, *this; }
    Point &operator-=(const Point &other) { return x -= other.x, y -= other.y, *this; }
    Point &operator*=(float mult) { return x *= mult, y *= mult, *this; }
    Point &operator/=(float divi) { return x /= divi, y /= divi, *this; }
    friend Point operator+(Point lhs, const Point &rhs) { return lhs += rhs; }
    friend Point operator-(Point lhs, const Point &rhs) { return lhs -= rhs; }
    friend Point operator*(Point pt, float mult) { return pt *= mult; }
    friend Point operator*(float mult, Point pt) { return pt *= mult; }
};

#endif
