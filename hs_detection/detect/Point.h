#ifndef POINT_H
#define POINT_H

#include <cmath>

namespace HSDetection
{
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
        Point &operator/=(float divisor) { return x /= divisor, y /= divisor, *this; }
        // not using reference for cascaded non-inplace ops
        friend Point operator+(Point lhs, const Point &rhs) { return lhs += rhs; }
        friend Point operator-(Point lhs, const Point &rhs) { return lhs -= rhs; }
        friend Point operator*(Point pt, float mult) { return pt *= mult; }
        friend Point operator*(float mult, Point pt) { return pt *= mult; }
        friend Point operator/(Point pt, float divisor) { return pt /= divisor; }
    };

} // namespace HSDetection

#endif
