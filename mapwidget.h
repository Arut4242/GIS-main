#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QWidget>
#include <QGeoCoordinate>
#include <QVector>
#include <QPointF>
#include <QPoint>

class MapWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MapWidget(QWidget* parent = nullptr);
    void setPath(const QVector<QGeoCoordinate>& path);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    QVector<QGeoCoordinate> m_path;
    QPointF m_centerLonLat; // Текущий центр карты (Долгота, Широта)
    double m_zoomLevel;     // Коэффициент зума
    QPoint m_lastMousePos;
    bool m_isDragging;

    QImage m_mapImage;

    QPointF lonLatToScreen(double lon, double lat) const;
};

#endif // MAPWIDGET_H