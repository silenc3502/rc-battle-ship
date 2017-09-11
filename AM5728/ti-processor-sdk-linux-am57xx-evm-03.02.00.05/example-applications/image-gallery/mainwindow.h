#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QDebug>
#include <QMessageBox>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);

    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *);
private slots:

    void on_prevImage_clicked();

    void on_clearImages_clicked();

    void on_nextImage_clicked();

    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    void displayImage(int count);
    int currentIndex;
    QStringList imagesList;
    QDir* imagesDir;
};



#endif // MAINWINDOW_H
