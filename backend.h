#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QGeoCoordinate>
#include <QVector>
#include "vector3d.h"

class Backend : public QObject
{
    Q_OBJECT
public:
    explicit Backend(QObject* parent = nullptr);

    // Главный метод расчета пересечения
    QVector<QGeoCoordinate> calculateIntersectionPoints(double cx, double cy, double cz, double r, double step);

private:
    const double a = 6378137.0;             // Большая полуось WGS-84
    const double f = 1.0 / 298.257223563;   // Сжатие
    const double b = a * (1.0 - f);         // Малая полуось
    const double e2 = 2 * f - f * f;        // Квадрат эксцентриситета

    Vector3d findInitialPoint(const Vector3d& C, double R);
    Vector3d computeTangent(const Vector3d& p, const Vector3d& c);
    Vector3d correctorNewton(const Vector3d& p_pred, const Vector3d& c, double R);
    QGeoCoordinate ECEFToGeodetic(const Vector3d& ecef);
};

#endif // BACKEND_H