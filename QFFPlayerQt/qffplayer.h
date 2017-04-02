#ifndef QFFPLAYER_H
#define QFFPLAYER_H

#include <QtWidgets/QMainWindow>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QWebView>
#include <QGroupBox>
#include <QPushButton>
#include "SDLWidget.h"

#include "ui_qffplayer.h"

class QFFPlayer : public QMainWindow
{
	Q_OBJECT
public:
	void changeView(int state);
	QFFPlayer(QWidget *parent = 0);
	~QFFPlayer();

protected:
	void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private:
	QGroupBox* createButtons();
	QWidget* createErrorPage();

	public slots:
	void showError();
	void changeLayout();
	void onClick(QAbstractButton *button);

private:
	Ui::QFFPlayerClass ui;
	int cameraWidget, tasksWidget, errorWidget;
	QSDLScreenWidget *sdlWidget;
	QWebView *view;
	QStackedWidget *stackedWidget;

};

#endif // QFFPLAYER_H
