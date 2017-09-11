#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QThread>
#include <QMutex>
#include "loopback.h"
#include <stdio.h>

class MyThread : public QThread
{
	Q_OBJECT

protected:
	void run();
};

MyThread* loopback;
QMutex drm_resource;

MainWindow::MainWindow(QWidget *parent) :
QMainWindow(parent),
ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	this->setAttribute(Qt::WA_TranslucentBackground);
	this->setWindowFlags(Qt::FramelessWindowHint);

	loopback = new MyThread();

	if(init_loopback() < 0) {
		/* TODO: make sure exit occurs properly */
	}

	/* Configure GUI */
	this->setFixedSize(status.display_xres, status.display_yres);
	ui->frame->setGeometry(0, 0, status.display_xres, status.display_yres);

	/* Set the back ground color to black */
	/* This is a workaround required when moving from Qt4 to QT5.  */
	/* QT5 doesn't support QWSServer. This is causing the graphics */
	/* windows not to clear up when being instructed to be hidden  */
	/* Making the background color black solves the problem        */
	ui->frame->setStyleSheet("background-color:black;");
	ui->frame->show();

	ui->frame_pip->setGeometry(25, 25, status.display_xres/3, status.display_yres/3);
	ui->capture->setGeometry(status.display_xres/2 - 210,
		status.display_yres - 50,
		91, 41);
	ui->switch_2->setGeometry(status.display_xres/2 - 100,
		status.display_yres - 50,
		91, 41);
	ui->pip->setGeometry(status.display_xres/2 + 10,
		status.display_yres - 50,
		91, 41);
	ui->exit->setGeometry(status.display_xres/2 + 120,
		status.display_yres - 50,
		91, 41);

	if(status.num_cams == 1) {
		/* Reconfigure GUI to reflect single camera mode */
		ui->pip->setHidden(true);
		ui->pip->setDisabled(true);
		ui->switch_2->setHidden(true);
		ui->switch_2->setDisabled(true);
		ui->frame_pip->setHidden(true);
		ui->capture->setGeometry(status.display_xres/2 - 100,
			status.display_yres - 50,
			91, 41);
		ui->exit->setGeometry(status.display_xres/2 + 10,
			status.display_yres - 50,
			91, 41);
	}

	/* Start the camera loopback thread */
	loopback->start();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::on_capture_clicked()
{
	status.jpeg=true;
}

void MainWindow::on_switch_2_clicked()
{
	/* While changing the drm display properties, make sure loopback thread */
	/* isn't running thereby accessing drm properties  simultaneously, and  */ 
	/* hence can cause system crash                                         */
	drm_resource.lock();

	if (status.num_cams==2){
		//switch the main camera by toggleing the main_cam value.
		status.main_cam = !status.main_cam;
		set_plane_properties();
	}

	drm_resource.unlock();
}

void MainWindow::on_pip_clicked()
{
	/* While changing the drm display properties, make sure loopback thread */
	/* isn't running thereby accessing drm properties  simultaneously, and  */ 
	/* hence can cause system crash                                         */
	drm_resource.lock();

	if (status.pip==true) {
		status.pip=false;
		drm_disable_pip();
		ui->frame_pip->setHidden(true);
	}
	else {
		drm_enable_pip();
		ui->frame_pip->setHidden(false);
		status.pip=true;
	}

	drm_resource.unlock();
}

void MainWindow::on_exit_clicked()
{
	status.exit=true;
	loopback->wait(1000);
	this->close();
}

void MyThread::run() {

	while(status.exit == false) {
		drm_resource.lock();
		process_frame();
		drm_resource.unlock();	
		msleep(1);
	}
	end_streaming();
	exit_devices();
}


#include "mainwindow.moc"
