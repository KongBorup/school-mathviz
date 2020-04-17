#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtMath>
#include <iostream>
#include "Matrix_NxN.h"
#include "Parser.h"
#include "InputHandler.h"
#include <QCoreApplication>
#include <exception>

//Create global variables
double x_max = 50;
double x_min = 0;
double func_spacing = 0.1;
int ind_plot = 0;
int ind_color_num = 0;
QString historie;
QColor qs[10] {};


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    //Setup the UI and change the background color to white
    ui->setupUi(this);
    this->setStyleSheet("background-color: #ffffff;");

    //Create different colors for the graph
    qs[0] = QColor(44, 160, 44);
    qs[1] = QColor(255, 127, 14);
    qs[2] = QColor(214, 39, 40);
    qs[3] = QColor(148, 103, 189);
    qs[4] = QColor(140, 86, 75);
    qs[5] = QColor(227, 119, 194);
    qs[6] = QColor(127, 127, 127);
    qs[7] = QColor(188, 189, 34);
    qs[8] = QColor(23, 190, 207);
    qs[9] = QColor(31, 119, 180);


    //Make another x- and y-axis but disable the ticklabels
    ui->customPlot->xAxis2->setVisible(true);
    ui->customPlot->xAxis2->setTickLabels(false);
    ui->customPlot->yAxis2->setVisible(true);
    ui->customPlot->yAxis2->setTickLabels(false);

    //Connect the x-axes and then the y-axes so they follow eachother when moved
    connect(ui->customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot->yAxis2, SLOT(setRange(QCPRange)));

    //Connect buttons with their corresponding functions
    connect(MainWindow::findChild<QPushButton *>("inputButton"), SIGNAL(released()), this, SLOT(input_pressed()));
    connect(MainWindow::findChild<QPushButton *>("buttonMax"), SIGNAL(released()),this, SLOT(max_x()));
    connect(MainWindow::findChild<QPushButton *>("buttonMin"), SIGNAL(released()),this, SLOT(min_x()));
    connect(MainWindow::findChild<QPushButton *>("buttonReset"), SIGNAL(released()),this, SLOT(plot_reset()));
    connect(MainWindow::findChild<QPushButton *>("buttonSpacing"), SIGNAL(released()),this, SLOT(change_spacing()));

    //Change the font of the history and make it bold
    QFont font = ui->historie->font();
    font.setPointSize(15);
    font.setBold(true);
    ui->historie->setFont(font);

    //Create an interaction where you can select, zoom, and drag the plot
    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
}
void MainWindow::draw_vec(Vector_N<2> vec)
{
    //Create vector and set start and end value of the vector
    QVector<double> x(2), y(2);
    double x2 = vec.get_x();
    double y2 = vec.get_y();
    x[0] = 0;
    x[1] = x2;
    y[0] = 0;
    y[1] = y2;

    //Create a new graph and set the data
    ui->customPlot->addGraph();
    ui->customPlot->graph(ind_plot)->setData(x, y);

    //Create a pen that sets the color and width of the vector line
    QPen vecPen;
    vecPen.setColor(qs[ind_color_num]);
    vecPen.setWidth(2);
    ui->customPlot->graph(ind_plot)->setPen(vecPen);

    //Create an arrowhead at the end of the vector and make it the same color and width as the line
    QCPItemLine *arrow = new QCPItemLine(ui->customPlot);
    arrow->end->setCoords(x[1], y[1]);
    arrow->setHead(QCPLineEnding::esSpikeArrow);
    arrow->setPen(vecPen);

    //Scale the axes so all the plots can be seen and then refresh the plot
    ui->customPlot->rescaleAxes();
    ui->customPlot->replot();

    //Set the history label to the expression and vectors coordinates
    ui->historie->setText("<table align=""center"">"
                         "<tr>"
                          "<td> <\td>"
                          "<td> <\td>"
                           "<td> </td>"
                        " </tr>"
                          "<tr>"
                          "<td> <\td>"
                          "<td> <\td>"
                          "<td> [" + QString::number(x2) + "] <\td>"
                          "<\tr>"
                        " <tr>"
                          "<td> " + historie + " <\td>"
                          "<td> = <\td>"
                           "<td>[" + QString::number(y2) +"] </td>"
                         "</tr>"
                       "</table>");

    //The variable that contains the number of plots and the index of what color is being used goes up
    ind_plot++;
    ind_color_num++;

    //If the color index goes out of bounds reset it
    if(ind_color_num == 10)
    {
        ind_color_num = 0;
    }
}

void MainWindow::draw_func(std::vector<Vector_N<2>> expr)
{
        //Set the amount of points in the graph
        int num_points = expr.size();

        //Create a vector and set the x-values with the corresponding y-values
        QVector<double> x1(num_points), y1(num_points);
        for(int i = 0; i < num_points; i++)
        {
            x1[i] = expr[i].get_x();
            y1[i] = expr[i].get_y();
        }

        //Create a new graph and set the data
        ui->customPlot->addGraph();
        ui->customPlot->graph(ind_plot)->setData(x1,y1);

        //Set the color of the graph
        QPen linePen;
        linePen.setColor(qs[ind_color_num]);
        linePen.setWidth(2);
        ui->customPlot->graph(ind_plot)->setPen(linePen);

        //Rescale the axes so you can see all the plots and then refresh the plots
        ui->customPlot->rescaleAxes();
        ui->customPlot->replot();

        //Set the history label equal to the expression that was entered
        ui->historie->setText(historie);

        //The variable with the amount of plots and index of the color being used goes up
        ind_plot++;
        ind_color_num++;

        //If the color index goes out of bounds reset it
        if(ind_color_num == 10)
        {
            ind_color_num = 0;
        }
    }

void MainWindow::input_pressed()
{
    //Make a pointer to the lineedit with the name "lineInput" and save the text inside it to a variable
    QLineEdit *input = MainWindow::findChild<QLineEdit *>("lineInput");
    QString inputVal = input->text();

    //Clear the lineinput from text
    ui->lineInput->clear();

    //Try and process the input
    try {
        //Create an inputhandler with the input from "lineInput"
        InputHandler ih(inputVal.toStdString().c_str());

        //If the input has the vector indentifier...
        if(ih.inp_kind == InputKind::vect)
        {
            //Evaluate the vector and create a string with just the expression
            Vector_N<2> res = ih.evaluate_vec();
            std::string from = "(";
            std::string to = ")";
            std::string str = inputVal.toStdString().c_str();
            std::string token = str.substr(str.find(from)+1,str.find(to));
            token.pop_back();

            //Set the history variable to the expression and plot the function
            historie = QString::fromStdString(token);
            draw_vec(res);
        }

        //If the input has the function indentifier...
        if (ih.inp_kind == InputKind::func)
        {
            //Evaluate the function and create a string with just the expression
            std::vector<Vector_N<2>> res = ih.evaluate_func(x_min, x_max, func_spacing);
            std::string from = "(";
            std::string to = ")";
            std::string str = inputVal.toStdString().c_str();
            std::string token = str.substr(str.find(from)+1,str.back());
            token.pop_back();

            //Set the history variable to the expression and plot the function
            historie = QString::fromStdString(token);
            draw_func(res);
        }

        //If the input has the point identifier...
        if (ih.inp_kind == InputKind::point)
        {
            //Evaluate the expression and plot the point
            Vector_N<2> res = ih.evaluate_vec();
            draw_point(res);
        }

      //Catch the exception if the processing of the input fails
    } catch (std::exception& e)
    {
        //Create a messagebox which tells the user that the input could not be processed
        QMessageBox msg_box;
        msg_box.setText("Input notation could not be processed. Please try and use the right notation");
        msg_box.exec();

        return;
    }
}

void MainWindow::min_x()
{
    //Make a pointer to the lineedit with the name "xMin" and save the text inside it to a variable
    QLineEdit *input = MainWindow::findChild<QLineEdit *>("xMin");
    QString inputVal = input->text();

    //Check if the input is a number
    bool c = isNum(inputVal.toStdString().c_str());

    //If the input isn't a number...
    if(!c){

        //Tell the user it has to be a number
        QMessageBox msg_box;
        msg_box.setText("x min needs to be a number");
        msg_box.exec();

        return;

    //If the inputVal is less than or equal to x_max...
    } else if(inputVal.toDouble() <= x_max)
    {
        //Clear the text inside the lineedit and change the label "xMin_text"'s text to the input
        ui->xMin->clear();
        ui->xMin_text->setText("x min = " + inputVal);

        //Change the global variable "x_min" to the input as double
        x_min = inputVal.toDouble();

    //If the inputVal is greater than x_max...
    } else
    {
        //Create a messagebox which tells the user that x min can't be greater than x max
        QMessageBox msg_box;
        msg_box.setText("x max needs to be greater than x min");
        msg_box.exec();

        return;
    }
}

void MainWindow::max_x()
{
    //Make a pointer to the lineedit with the name "xMax" and save the text inside it to a variable
    QLineEdit *input = MainWindow::findChild<QLineEdit *>("xMax");
    QString inputVal = input->text();

    //Check if the input is a number
    bool c = isNum(inputVal.toStdString().c_str());

    //If input isn't a number...
    if(!c){

        //Tell the user the input has to be a number
        QMessageBox msg_box;
        msg_box.setText("x max needs to be a number");
        msg_box.exec();

        return;

    //If the input is greater than or equal to x_min...
    } else if(inputVal.toDouble() >= x_min)
    {
        //Clear the text inside the lineedit and change the label "xMax_text"'s text to the input
        ui->xMax->clear();
        ui->xMax_text->setText("x max = " + inputVal);

        //Change the global variable "x_max" to the input as double
        x_max = inputVal.toDouble();

    //If the input is less than x_min...
    } else
    {
        //Create a messagebox which tells the user that x max can't be less than x min
        QMessageBox msg_box;
        msg_box.setText("x max needs to be greater than x min");
        msg_box.exec();

        return;
    }

}
void MainWindow::plot_reset()
{
    //Reset the plot, remove the history text and set the variable "ind_plot" to 0
    ui->customPlot->clearItems();
    ui->customPlot->clearGraphs();
    ui->customPlot->replot();
    ui->historie->setText("");
    ind_plot = 0;
}
void MainWindow::draw_point(Vector_N<2> point)
{
    //Create a vector and set the x and y to the input
    QVector<double> x(1), y(1);
    double x2 = point.get_x();
    double y2 = point.get_y();
    x[0] = x2;
    y[0] = y2;

    //Create a new graph and set the data
    ui->customPlot->addGraph();
    ui->customPlot->graph(ind_plot)->setData(x, y);

    //Change the style of the plot so there is no line and only a filled circle
    QCPScatterStyle myScatter;
    myScatter.setShape(QCPScatterStyle::ssCircle);
    myScatter.setPen(QPen(qs[ind_color_num]));
    myScatter.setBrush(qs[ind_color_num]);
    myScatter.setSize(6);
    ui->customPlot->graph(ind_plot)->setScatterStyle(myScatter);
    ui->customPlot->graph(ind_plot)->setLineStyle(QCPGraph::lsNone);

    //Rescale the axes so all plots can be seen and then refresh the plot
    ui->customPlot->rescaleAxes();
    ui->customPlot->replot();

    //Set the history label to the point and add 1 to the amount of plots
    ui->historie->setText("(" + QString::number(x2) + ", " + QString::number(y2)+ ")");


    //The variable with the amount of plots and index of the color being used goes up
    ind_plot++;
    ind_color_num++;

    //If the color index goes out of bounds reset it
    if(ind_color_num == 10)
    {
        ind_color_num = 0;
    }
}

void MainWindow::change_spacing()
{
    //Make a pointer to the lineedit with the name "spacing" and save the text inside it to a variable
    QLineEdit *input = MainWindow::findChild<QLineEdit *>("spacing");
    QString inputVal = input->text();

    //If the input isn't a  positive number...
    if(!((inputVal <= '9' && inputVal >= '0'))){

        //Create a box that tells the user it needs to be a number
        QMessageBox msg_box;
        msg_box.setText("spacing needs to be a number");
        msg_box.exec();

        return;

    //If the input is a number...
    } else {
        //Clear the text inside the lineedit and set the label "spacing_text"'s text to the input
        ui->spacing->clear();
        ui->spacing_text->setText("spacing = " + inputVal);

        //Set the variable "fun_spacing" to the input as double
        func_spacing = inputVal.toDouble();
    }
}

bool MainWindow::isNum(std::string line)
{
    //Check if the string only contains numbers
    char* p;
    strtol(line.c_str(), &p, 10);
    return *p == 0;
}

MainWindow::~MainWindow()
{
    delete ui;
}

