#include "viewer3d.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Viewer3D::Viewer3D(QWidget* parent)
    : QWidget(parent), m_pitch(0.5), m_yaw(0.5), m_zoom(0.04), m_isDragging(false), m_sphereRadius(0)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
}

Vector3d Viewer3D::geodeticToECEF(const QGeoCoordinate& coord)
{
    double lat = coord.latitude() * M_PI / 180.0;
    double lon = coord.longitude() * M_PI / 180.0;

    // ЗАЩИТА ОТ NaN: Если высота не задана, Qt возвращает NaN. Сбрасываем в 0.
    double h = coord.altitude();
    if (std::isnan(h)) {
        h = 0.0;
    }
    else {
        h /= 1000.0; // Переводим метры в километры
    }

    double sinLat = std::sin(lat);
    double cosLat = std::cos(lat);

    // Радиус кривизны
    double N = a / std::sqrt(1.0 - e2 * sinLat * sinLat);

    double x = (N + h) * cosLat * std::cos(lon);
    double y = (N + h) * cosLat * std::sin(lon);
    double z = (N * (1.0 - e2) + h) * sinLat;
    return Vector3d(x, y, z);
}

void Viewer3D::setData(const QVector<QGeoCoordinate>& path, double cx, double cy, double cz, double r)
{
    m_ecefPath.clear();
    for (const auto& coord : path) {
        m_ecefPath.append(geodeticToECEF(coord));
    }
    m_sphereCenter = Vector3d(cx / 1000.0, cy / 1000.0, cz / 1000.0);
    m_sphereRadius = r / 1000.0;
    update();
}

QPointF Viewer3D::project3DToScreen(const Vector3d& p, bool& isBehind) const
{
    // 1. Поворот вокруг оси Z (Yaw)
    double x1 = p.x * std::cos(m_yaw) - p.y * std::sin(m_yaw);
    double y1 = p.x * std::sin(m_yaw) + p.y * std::cos(m_yaw);
    double z1 = p.z;

    // 2. Поворот вокруг оси X (Pitch)
    double x2 = x1;
    double y2 = y1 * std::cos(m_pitch) - z1 * std::sin(m_pitch);
    double z2 = y1 * std::sin(m_pitch) + z1 * std::cos(m_pitch);

    // Если точка ушла за задний план глобуса
    isBehind = false; //(y2 < 0);

    // Проекция на экран (0,0 посередине виджета)
    double screenX = width() / 2.0 + x2 * m_zoom;
    double screenY = height() / 2.0 - z2 * m_zoom;

    return QPointF(screenX, screenY);
}

void Viewer3D::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(15, 15, 20)); // Космический фон

    // 1. Рисуем сетку Земли (параллели и меридианы)
    painter.setPen(QPen(QColor(0, 120, 255, 60), 1));
    for (int lat = -80; lat <= 80; lat += 20) {
        QPolygonF poly;
        for (int lon = -180; lon <= 180; lon += 5) {
            bool behind;
            QPointF pt = project3DToScreen(geodeticToECEF(QGeoCoordinate(lat, lon)), behind);
            if (!behind) {
                poly.append(pt);
            }
            else {
                if (poly.size() > 1) { painter.drawPolyline(poly); }
                poly.clear();
            }
        }
        if (poly.size() > 1) painter.drawPolyline(poly);
    }
    for (int lon = -180; lon < 180; lon += 30) {
        QPolygonF poly;
        for (int lat = -90; lat <= 90; lat += 5) {
            bool behind;
            QPointF pt = project3DToScreen(geodeticToECEF(QGeoCoordinate(lat, lon)), behind);
            if (!behind) {
                poly.append(pt);
            }
            else {
                if (poly.size() > 1) { painter.drawPolyline(poly); }
                poly.clear();
            }
        }
        if (poly.size() > 1) painter.drawPolyline(poly);
    }

    // 2. Рисуем ОБЪЕМНЫЙ каркас Сферы (параллели и меридианы)
    if (m_sphereRadius > 0) {
        // Сделаем линии сферы оранжевыми и чуть более заметными (альфа-канал 140)
        painter.setPen(QPen(QColor(255, 165, 0, 140), 1));

        // --- Шаг А: Рисуем горизонтальные кольца (параллели сферы) ---
        for (int lat = -60; lat <= 60; lat += 30) {
            double latRad = lat * M_PI / 180.0;

            // Радиус кольца на текущей высоте и его смещение по оси Z
            double r_lat = m_sphereRadius * std::cos(latRad);
            double z_lat = m_sphereCenter.z + m_sphereRadius * std::sin(latRad);

            QPolygonF sPoly;
            for (int lon = 0; lon <= 360; lon += 10) {
                double lonRad = lon * M_PI / 180.0;

                Vector3d p(m_sphereCenter.x + r_lat * std::cos(lonRad),
                    m_sphereCenter.y + r_lat * std::sin(lonRad),
                    z_lat);

                bool behind;
                QPointF pt = project3DToScreen(p, behind);
                if (!behind) {
                    sPoly.append(pt);
                }
                else {
                    if (sPoly.size() > 1) painter.drawPolyline(sPoly);
                    sPoly.clear();
                }
            }
            if (sPoly.size() > 1) painter.drawPolyline(sPoly);
        }

        // --- Шаг Б: Рисуем вертикальные кольца (меридианы сферы) ---
        for (int lon = 0; lon < 360; lon += 45) {
            double lonRad = lon * M_PI / 180.0;

            QPolygonF sPoly;
            for (int lat = -90; lat <= 90; lat += 10) {
                double latRad = lat * M_PI / 180.0;

                // Вычисляем 3D-координаты меридиана сферы
                Vector3d p(m_sphereCenter.x + m_sphereRadius * std::cos(latRad) * std::cos(lonRad),
                    m_sphereCenter.y + m_sphereRadius * std::cos(latRad) * std::sin(lonRad),
                    m_sphereCenter.z + m_sphereRadius * std::sin(latRad));

                bool behind;
                QPointF pt = project3DToScreen(p, behind);
                if (!behind) {
                    sPoly.append(pt);
                }
                else {
                    if (sPoly.size() > 1) painter.drawPolyline(sPoly);
                    sPoly.clear();
                }
            }
            if (sPoly.size() > 1) painter.drawPolyline(sPoly);
        }
    }

    // 3. Рисуем ЛИНИЮ ПЕРЕСЕЧЕНИЯ
    if (!m_ecefPath.isEmpty()) {
        painter.setPen(QPen(Qt::red, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPolygonF pathPoly;
        for (const auto& ptECEF : m_ecefPath) {
            bool behind;
            QPointF ptScreen = project3DToScreen(ptECEF, behind);
            if (!behind) {
                pathPoly.append(ptScreen);
            }
            else {
                if (pathPoly.size() > 1) {
                    painter.drawPolyline(pathPoly);
                }
                pathPoly.clear();
            }
        }
        if (pathPoly.size() > 1) painter.drawPolyline(pathPoly);
    }
}

void Viewer3D::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastMousePos = event->pos();
    }
}

void Viewer3D::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDragging) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();

        m_yaw += delta.x() * 0.005;
        m_pitch += delta.y() * 0.005;

        // Ограничение триггера Pitch (чтобы не перевернуть глобус «вверх ногами»)
        if (m_pitch > M_PI_2) m_pitch = M_PI_2;
        if (m_pitch < -M_PI_2) m_pitch = -M_PI_2;

        update();
    }
}

void Viewer3D::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) m_isDragging = false;
}

void Viewer3D::wheelEvent(QWheelEvent* event)
{
    if (event->angleDelta().y() > 0) m_zoom *= 1.1;
    else m_zoom /= 1.1;

    // Новые безопасные лимиты для масштаба глобуса
    if (m_zoom < 0.005) m_zoom = 0.005;
    if (m_zoom > 0.5) m_zoom = 0.5;

    update();
}