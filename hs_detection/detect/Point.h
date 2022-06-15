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

    Point &operator+=(const Point &other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
    Point &operator*=(float mult)
    {
        x *= mult;
        y *= mult;
        return *this;
    }
    Point &operator/=(float divi)
    {
        x /= divi;
        y /= divi;
        return *this;
    }
    friend Point operator-(const Point &lhs, const Point &rhs)
    {
        return Point(lhs.x - rhs.x, lhs.y - rhs.y);
    }
    static float abs(const Point &pt)
    {
        return sqrt(pt.x * pt.x + pt.y * pt.y);
    }
};

#endif
