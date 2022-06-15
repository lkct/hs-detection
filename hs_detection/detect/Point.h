#ifndef POINT_H
#define POINT_H

class Point
{
public:
    float x;
    float y;

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
};

#endif
