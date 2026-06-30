#include "mapwidget.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <qstring.h>

MapWidget::MapWidget(QWidget* parent)
    : QWidget(parent), m_centerLonLat(0.0, 0.0), m_zoomLevel(1.0), m_isDragging(false)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    m_mapImage.load(":/world_map.jpg");
}

void MapWidget::setPath(const QVector<QGeoCoordinate>& path)
{
    m_path = path;
    if (!m_path.isEmpty()) {
        // Автоматически фокусируемся на начале полученной кривой
        m_centerLonLat = QPointF(m_path.first().longitude(), m_path.first().latitude());
    }
    update();
}

QPointF MapWidget::lonLatToScreen(double lon, double lat) const
{
    double baseScale = width() / 360.0;
    double S = baseScale * m_zoomLevel;

    double x = width() / 2.0 + (lon - m_centerLonLat.x()) * S;
    double y = height() / 2.0 - (lat - m_centerLonLat.y()) * S;
    return QPointF(x, y);
}

void MapWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    // Включаем сглаживание для линий и текстуры карты
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Фон вокруг карты (космический серый/черный)
    painter.fillRect(rect(), QColor(25, 25, 25));

    // --- ОТРИСОВКА КАРТЫ ЗЕМЛИ ---
    if (!m_mapImage.isNull()) {
        // Вычисляем экранные координаты углов карты мира
        // Карта в этой проекции простирается от -180 до 180 по долготе и от -90 до 90 по широте
        QPointF topLeft = lonLatToScreen(-180.0, 90.0);
        QPointF bottomRight = lonLatToScreen(180.0, -90.0);

        QRectF mapTargetRect(topLeft, bottomRight);

        // Рисуем текстуру Земли. Она автоматически растянется/сдвинется по нашим формулам!
        painter.drawImage(mapTargetRect, m_mapImage);
    }

    // --- ОТРИСОВКА СЕТКИ КООРДИНАТ ---
    // Делаем сетку чуть прозрачнее, чтобы она не перекрывала карту слишком сильно
    painter.setPen(QPen(QColor(255, 255, 255, 70), 1, Qt::DashLine));
    for (int lon = -180; lon <= 180; lon += 30) {
        painter.drawLine(lonLatToScreen(lon, -90), lonLatToScreen(lon, 90));
        QPointF p = lonLatToScreen(lon, 0);
        if (rect().contains(p.toPoint())) painter.drawText(p + QPointF(4, -4), QString::number(lon));
    }
    for (int lat = -90; lat <= 90; lat += 30) {
        painter.drawLine(lonLatToScreen(-180, lat), lonLatToScreen(180, lat));
        QPointF p = lonLatToScreen(0, lat);
        if (rect().contains(p.toPoint())) painter.drawText(p + QPointF(4, -4), QString::number(lat));
    }

    // Главные оси (Экватор и Гринвич)
    painter.setPen(QPen(QColor(255, 255, 255, 120), 1.5));
    painter.drawLine(lonLatToScreen(-180, 0), lonLatToScreen(180, 0));
    painter.drawLine(lonLatToScreen(0, -90), lonLatToScreen(0, 90));

    // --- ОТРИСОВКА ЛИНИИ ПЕРЕСЕЧЕНИЯ СФЕРЫ С ГЕОИДОМ ---
    if (!m_path.isEmpty()) {
        painter.setPen(QPen(Qt::red, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPolygonF poly;
        for (const auto& coord : m_path) {
            poly.append(lonLatToScreen(coord.longitude(), coord.latitude()));
        }
        painter.drawPolyline(poly);
    }
}

void MapWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastMousePos = event->pos();
    }
}

void MapWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDragging) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();

        double baseScale = width() / 360.0;
        double S = baseScale * m_zoomLevel;

        m_centerLonLat.setX(m_centerLonLat.x() - (delta.x() / S));
        m_centerLonLat.setY(m_centerLonLat.y() + (delta.y() / S));

        // Нормализация координат карты
        if (m_centerLonLat.x() > 180) m_centerLonLat.setX(-180);
        if (m_centerLonLat.x() < -180) m_centerLonLat.setX(180);
        if (m_centerLonLat.y() > 90) m_centerLonLat.setY(90);
        if (m_centerLonLat.y() < -90) m_centerLonLat.setY(-90);

        update();
    }
}

void MapWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
    }
}

void MapWidget::wheelEvent(QWheelEvent* event)
{
    double zoomFactor = 1.25;
    if (event->angleDelta().y() < 0) {
        zoomFactor = 1.0 / zoomFactor;
    }
    m_zoomLevel *= zoomFactor;

    if (m_zoomLevel < 0.6) m_zoomLevel = 0.6;
    if (m_zoomLevel > 3000.0) m_zoomLevel = 3000.0;

    update();
}