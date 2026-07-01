#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include "backend.h"
#include "mapwidget.h"
#include "viewer3d.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);

private slots:
    void onCalculateClicked();

private:
    Backend m_backend;
    MapWidget* m_mapWidget;
    Viewer3D* m_viewer3D;

    QLineEdit* m_editX;
    QLineEdit* m_editY;
    QLineEdit* m_editZ;
    QLineEdit* m_editRadius;
    QSpinBox* m_spinStep;
    QLabel* m_labelError;
};

#endif // MAINWINDOW_H