#ifndef VECTOR3D_H
#define VECTOR3D_H

#include <cmath>

struct Vector3d {
    double x, y, z;

    Vector3d() : x(0.0), y(0.0), z(0.0) {}
    Vector3d(double x, double y, double z) : x(x), y(y), z(z) {}

    Vector3d operator+(const Vector3d& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vector3d operator-(const Vector3d& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vector3d operator*(double s) const { return { x * s, y * s, z * s }; }
    Vector3d operator-() const { return { -x, -y, -z }; }

    double dot(const Vector3d& o) const { return x * o.x + y * o.y + z * o.z; }
    Vector3d cross(const Vector3d& o) const {
        return { y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x };
    }
    double squaredNorm() const { return x * x + y * y + z * z; }
    double norm() const { return std::sqrt(squaredNorm()); }
    Vector3d normalized() const {
        double n = norm();
        return n < 1e-9 ? Vector3d(0, 0, 0) : Vector3d(x / n, y / n, z / n);
    }
    bool isZero() const { return norm() < 1e-7; }
};

#endif // VECTOR3D_H