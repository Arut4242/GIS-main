#include "backend.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Backend::Backend(QObject* parent) : QObject(parent) {}

Vector3d Backend::findInitialPoint(const Vector3d& C, double R)
{
    double Cr = std::sqrt(C.x * C.x + C.y * C.y);
    double Cz = C.z;
    double phi = std::atan2(C.y, C.x);

    double theta_root = 0.0;
    bool found = false;
    const int steps = 180;
    double prev_val = 0.0;

    for (int i = 0; i <= steps; ++i)
    {
        double theta = -M_PI + (2.0 * M_PI * i) / steps;
        double r = a * std::cos(theta);
        double z = b * std::sin(theta);

        double val = (r - Cr) * (r - Cr) + (z - Cz) * (z - Cz) - R * R;

        if (i > 0 && prev_val * val <= 0)
        {
            double t_low = theta - (2.0 * M_PI) / steps;
            double t_high = theta;

            for (int iter = 0; iter < 30; ++iter)
            {
                double t_mid = 0.5 * (t_low + t_high);
                double r_l = a * std::cos(t_low);
                double z_l = b * std::sin(t_low);
                double v_l = (r_l - Cr) * (r_l - Cr) + (z_l - Cz) * (z_l - Cz) - R * R;

                double r_m = a * std::cos(t_mid);
                double z_m = b * std::sin(t_mid);
                double v_m = (r_m - Cr) * (r_m - Cr) + (z_m - Cz) * (z_m - Cz) - R * R;

                if (v_l * v_m <= 0) {
                    t_high = t_mid;
                }
                else {
                    t_low = t_mid;
                }
            }
            theta_root = 0.5 * (t_low + t_high);
            found = true;
            break;
        }
        prev_val = val;
    }

    if (!found)
    {
        double lat = std::atan2(Cz, Cr * (1.0 - e2));
        for (int i = 0; i < 5; ++i) {
            double sinLat = std::sin(lat);
            double N = a / std::sqrt(1.0 - e2 * sinLat * sinLat);
            lat = std::atan2(Cz + e2 * N * sinLat, Cr);
        }
        double r = a * std::cos(lat);
        double z = b * std::sin(lat);
        return Vector3d(r * std::cos(phi), r * std::sin(phi), z);
    }

    double r_res = a * std::cos(theta_root);
    double z_res = b * std::sin(theta_root);
    return Vector3d(r_res * std::cos(phi), r_res * std::sin(phi), z_res);
}

Vector3d Backend::computeTangent(const Vector3d& p, const Vector3d& c)
{
    Vector3d gradF1 = (p - c) * 2.0;
    Vector3d gradF2(2.0 * p.x / (a * a), 2.0 * p.y / (a * a), 2.0 * p.z / (b * b));

    Vector3d T = gradF1.cross(gradF2);
    if (T.isZero()) return Vector3d(0, 0, 0);
    return T.normalized();
}

Vector3d Backend::correctorNewton(const Vector3d& p_pred, const Vector3d& c, double R)
{
    Vector3d p = p_pred;
    const int maxIter = 30;
    const double tol = 1e-4;

    for (int i = 0; i < maxIter; ++i)
    {
        double F1 = (p - c).squaredNorm() - R * R;
        double F2 = (p.x * p.x + p.y * p.y) / (a * a) + (p.z * p.z) / (b * b) - 1.0;

        double rhsX = -F1;
        double rhsY = -F2;

        double J00 = 2.0 * (p.x - c.x); double J01 = 2.0 * (p.y - c.y); double J02 = 2.0 * (p.z - c.z);
        double J10 = 2.0 * p.x / (a * a); double J11 = 2.0 * p.y / (a * a); double J12 = 2.0 * p.z / (b * b);

        double M00 = J00 * J00 + J01 * J01 + J02 * J02;
        double M01 = J00 * J10 + J01 * J11 + J02 * J12;
        double M10 = M01;
        double M11 = J10 * J10 + J11 * J11 + J12 * J12;

        double det = M00 * M11 - M01 * M10;
        if (std::abs(det) < 1e-16) break;

        double invM00 = M11 / det;  double invM01 = -M01 / det;
        double invM10 = -M10 / det; double invM11 = M00 / det;

        double resX = invM00 * rhsX + invM01 * rhsY;
        double resY = invM10 * rhsX + invM11 * rhsY;

        Vector3d dp(
            J00 * resX + J10 * resY,
            J01 * resX + J11 * resY,
            J02 * resX + J12 * resY
        );

        p = p + dp;
        if (dp.norm() < tol) break;
    }
    return p;
}

QGeoCoordinate Backend::ECEFToGeodetic(const Vector3d& ecef)
{
    double X = ecef.x, Y = ecef.y, Z = ecef.z;
    double lon = std::atan2(Y, X);
    double p = std::sqrt(X * X + Y * Y);

    double lat = std::atan2(Z, p * (1.0 - e2));
    for (int i = 0; i < 5; ++i) {
        double sinLat = std::sin(lat);
        double N = a / std::sqrt(1.0 - e2 * sinLat * sinLat);
        lat = std::atan2(Z + e2 * N * sinLat, p);
    }
    double sinLat = std::sin(lat);
    double N = a / std::sqrt(1.0 - e2 * sinLat * sinLat);
    double h = p / std::cos(lat) - N;

    return QGeoCoordinate(lat * 180.0 / M_PI, lon * 180.0 / M_PI, h);
}

QVector<QGeoCoordinate> Backend::calculateIntersectionPoints(double cx, double cy, double cz, double r, double step)
{
    QVector<QGeoCoordinate> pathCoords;
    Vector3d C(cx, cy, cz);
    double R = r;
    double ds = step;

    Vector3d p0 = findInitialPoint(C, R);
    p0 = correctorNewton(p0, C, R);

    double checkF1 = (p0 - C).squaredNorm() - R * R;
    double checkF2 = (p0.x * p0.x + p0.y * p0.y) / (a * a) + (p0.z * p0.z) / (b * b) - 1.0;

    if (std::abs(checkF1) > 10.0 || std::abs(checkF2) > 1e-4) {
        return pathCoords;
    }

    pathCoords.append(ECEFToGeodetic(p0));

    Vector3d p_curr = p0;
    Vector3d T_prev(0, 0, 0);

    bool closed = false;
    bool left_start = false;
    const int maxPoints = 50000;

    for (int i = 0; i < maxPoints; ++i)
    {
        Vector3d T = computeTangent(p_curr, C);
        if (T.isZero()) break;

        if (i > 0 && T.dot(T_prev) < 0) {
            T = -T;
        }
        T_prev = T;

        Vector3d p_pred = p_curr + T * ds;
        p_curr = correctorNewton(p_pred, C, R);

        double dist_to_start = (p_curr - p0).norm();

        if (!left_start && dist_to_start > ds * 2.0) {
            left_start = true;
        }

        if (left_start && dist_to_start < ds * 1.2) {
            closed = true;
            break;
        }

        pathCoords.append(ECEFToGeodetic(p_curr));
    }

    if (closed && !pathCoords.isEmpty()) {
        pathCoords.append(pathCoords.first());
    }

    return pathCoords;
}