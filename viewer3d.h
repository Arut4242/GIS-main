#ifndef VIEWER3D_H
#define VIEWER3D_H

#include <QWidget>
#include <QGeoCoordinate>
#include <QVector>
#include <QPoint>
#include "vector3d.h"

class Viewer3D : public QWidget
{
    Q_OBJECT
public:
    explicit Viewer3D(QWidget* parent = nullptr);
    void setData(const QVector<QGeoCoordinate>& path, double cx, double cy, double cz, double r);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    QVector<Vector3d> m_ecefPath; // Линия пересечения в 3D
    Vector3d m_sphereCenter;      // Центр сферы в 3D
    double m_sphereRadius;        // Радиус сферы

    // Углы поворота камеры и зум
    double m_pitch; // Вверх-вниз
    double m_yaw;   // Влево-вправо
    double m_zoom;  // Масштаб

    QPoint m_lastMousePos;
    bool m_isDragging;

    // Константы WGS-84 (в километрах)
    const double a = 6378.137;
    const double b = 6356.752;
    const double f = 1.0 / 298.257223563;
    const double e2 = 2.0 * f - f * f;

    Vector3d geodeticToECEF(const QGeoCoordinate& coord);
    QPointF project3DToScreen(const Vector3d& p, bool& isBehind) const;
    void drawWireframeEllipsoid(QPainter& painter);
    void drawWireframeSphere(QPainter& painter);
};

#endif // VIEWER3D_H