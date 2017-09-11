#include "mainwindow.h"
#include "ui_mainwindow.h"

QString dirLocation;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);


    currentIndex = 0;

    QStringList filters;
    filters << "*.jpeg" << "*.jpg";

    imagesDir = new QDir (dirLocation);

    imagesDir->setFilter(QDir::Files);
    imagesDir->setNameFilters( filters);
    imagesDir->setSorting(QDir::Name);
    imagesList = imagesDir->entryList();


     ui->prevImage->setEnabled(false);
    if(imagesList.count() == 0 || imagesList.count() == 1) {

        ui->nextImage->setEnabled(false);

        if(imagesList.count() == 0) {
            ui->label->clear();
            ui->label->setText("No images found");
            ui->clearImages->setEnabled(false);
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_prevImage_clicked()
{
    currentIndex--;
    if(currentIndex <= 0)
       ui->prevImage->setEnabled(false);

    if(currentIndex < imagesList.count())
       ui->nextImage->setEnabled(true);

    displayImage(currentIndex);
}

void MainWindow::on_clearImages_clicked()
{

     QMessageBox::StandardButton reply;
     reply = QMessageBox::question(this,"Delete Confirmation","Delete image?",QMessageBox::Yes|QMessageBox::No);

     if(reply == QMessageBox::No)
         return;

     ui->nextImage->setEnabled(false);
     ui->prevImage->setEnabled(false);

     QStringList clearList = imagesDir->entryList();


    imagesDir->remove(clearList[currentIndex]);

    imagesDir->refresh();

    imagesList = imagesDir->entryList();

    currentIndex--;

    if(currentIndex < 0)
        currentIndex = 0;

    ui->nextImage->setEnabled(false);
    ui->prevImage->setEnabled(false);

    if(imagesList.count() > 1 && imagesList.count() -1 > currentIndex)
        ui->nextImage->setEnabled(true);

    if(currentIndex > 0)
        ui->prevImage->setEnabled(true);

    if(imagesList.count() >= 1)
        displayImage(currentIndex);
    else {
        ui->clearImages->setEnabled(false);
        ui->label->clear();
        ui->label->setText("No images found");
    }


}

void MainWindow::on_nextImage_clicked()
{
    currentIndex++;
    if(currentIndex >= imagesList.size()-1)
       ui->nextImage->setEnabled(false);

    ui->prevImage->setEnabled(true);

    displayImage(currentIndex);

}

void MainWindow::displayImage(int index) {

    int width = ui->label->width();
    int height = ui->label->height();
    QPixmap image;
    image.load(dirLocation+imagesList[index]);
    image = image.scaled(width,height,Qt::KeepAspectRatio);
    ui->label->setPixmap(image);

}

void MainWindow::on_pushButton_clicked()
{
    this->close();
}
void MainWindow::resizeEvent(QResizeEvent *ev)
{
    QMainWindow::resizeEvent(ev);

    if(imagesList.count() > 0)
        displayImage(currentIndex);
}
