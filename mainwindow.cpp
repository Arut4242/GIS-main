#pragma execution_character_set("utf-8")
#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QTabWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    resize(1024, 768);
    setWindowTitle("ГИС: Интерсекция Сферы и WGS-84");

    QWidget* centralWidget = new QWidget();
    setCentralWidget(centralWidget);

    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

    // Левая боковая панель настроек
    QGroupBox* controlGroup = new QGroupBox("Панели управления", centralWidget);
    controlGroup->setFixedWidth(400);
    QVBoxLayout* controlLayout = new QVBoxLayout(controlGroup);

    QFormLayout* formLayout = new QFormLayout();

    m_editX = new QLineEdit("0", centralWidget);
    m_editY = new QLineEdit("0", centralWidget);
    m_editZ = new QLineEdit("6378137", centralWidget);

    QHBoxLayout* ecefLayout = new QHBoxLayout();
    ecefLayout->addWidget(m_editX);
    ecefLayout->addWidget(m_editY);
    ecefLayout->addWidget(m_editZ);
    formLayout->addRow("Центр сферы (ECEF XYZ, м):", ecefLayout);

    m_editRadius = new QLineEdit("500000", centralWidget);
    formLayout->addRow("Радиус (м):", m_editRadius);

    m_spinStep = new QSpinBox(centralWidget);
    m_spinStep->setRange(100, 1000000);
    m_spinStep->setSingleStep(5000);
    m_spinStep->setValue(10000);
    formLayout->addRow("Шаг дискретизации (м):", m_spinStep);

    controlLayout->addLayout(formLayout);

    QPushButton* btnCalculate = new QPushButton("Рассчитать", centralWidget);
    btnCalculate->setStyleSheet("background-color: #0078D7; color: white; font-weight: bold; padding: 6px;");
    controlLayout->addWidget(btnCalculate);

    m_labelError = new QLabel(centralWidget);
    m_labelError->setStyleSheet("color: red;");
    m_labelError->setWordWrap(true);
    controlLayout->addWidget(m_labelError);
    controlLayout->addStretch();

    // Правая область с интерактивной картой
    QTabWidget* tabWidget = new QTabWidget(centralWidget);

    m_mapWidget = new MapWidget(tabWidget);
    m_viewer3D = new Viewer3D(tabWidget);

    tabWidget->addTab(m_mapWidget, "Плоская 2D Карта");
    tabWidget->addTab(m_viewer3D, "Интерактивная 3D Модель");

    mainLayout->addWidget(controlGroup);
    mainLayout->addWidget(tabWidget, 1);

    connect(btnCalculate, &QPushButton::clicked, this, &MainWindow::onCalculateClicked);
}

void MainWindow::onCalculateClicked()
{
    m_labelError->clear();

    double cx = m_editX->text().toDouble();
    double cy = m_editY->text().toDouble();
    double cz = m_editZ->text().toDouble();
    double radius = m_editRadius->text().toDouble();
    double step = m_spinStep->value();

    // Вызываем требуемый метод, возвращающий массив точек
    QVector<QGeoCoordinate> points = m_backend.calculateIntersectionPoints(cx, cy, cz, radius, step);

    if (points.isEmpty()) {
        m_labelError->setText("Сфера не пересекает эллипсоид или заданы некорректные параметры.");
    }

    m_mapWidget->setPath(points);
    m_viewer3D->setData(points, cx, cy, cz, radius);
}