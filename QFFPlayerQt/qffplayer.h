#ifndef QFFPLAYER_H
#define QFFPLAYER_H

#include <QtWidgets/QMainWindow>
#include "ui_qffplayer.h"

class QFFPlayer : public QMainWindow
{
	Q_OBJECT

public:
	QFFPlayer(QWidget *parent = 0);
	~QFFPlayer();

private:
	Ui::QFFPlayerClass ui;
};

#endif // QFFPLAYER_H
