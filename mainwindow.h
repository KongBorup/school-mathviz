#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Matrix_NxN.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
private slots:
    void draw_vec(Vector_N<2>);
    void draw_func(std::vector<Vector_N<2>>);
    void draw_point(Vector_N<2>);
    void input_pressed();
    void min_x();
    void max_x();
    void change_spacing();
    void plot_reset();
    bool isNum(std::string);
};
#endif // MAINWINDOW_H
