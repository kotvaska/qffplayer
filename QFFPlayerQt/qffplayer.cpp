#include <QMainWindow>
#include <QDesktopWidget>
#include <QLabel>

#include <iostream>

#include "qffplayer.h"

QFFPlayer::QFFPlayer(QWidget *parent)
	: QMainWindow(parent) {
	ui.setupUi(this);
	setMouseTracking(true);
	ui.centralWidget->setMouseTracking(true);

	sdlWidget = new QSDLScreenWidget(parent);
	sdlWidget->setMouseTracking(true);

	view = new QWebView;
	view->load(QUrl("http://localhost/"));
	view->setMouseTracking(true);
	QGroupBox *groupBox = new QGroupBox(parent);
	QGridLayout *grid = new QGridLayout;
	grid->addWidget(view, 0, 0);
	grid->addWidget(createButtons(), 1, 0);
	grid->setRowStretch(0, 1);
	grid->setRowStretch(1, 0);
	groupBox->setLayout(grid);

	stackedWidget = new QStackedWidget;
	cameraWidget = stackedWidget->addWidget(sdlWidget);
	tasksWidget = stackedWidget->addWidget(groupBox);
	errorWidget = stackedWidget->addWidget(createErrorPage());
	stackedWidget->setMouseTracking(true);

	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(stackedWidget);
	ui.centralWidget->setLayout(layout);

	QObject::connect(sdlWidget, SIGNAL(changeCam()), this, SLOT(changeLayout()));
	QObject::connect(sdlWidget, SIGNAL(errorCam()), this, SLOT(showError()));

	changeView(1);
	ui.centralWidget->showFullScreen();
}

QGroupBox* QFFPlayer::createButtons() {
	QGroupBox *groupBox = new QGroupBox;
	QHBoxLayout *layout = new QHBoxLayout;
	QButtonGroup *group = new QButtonGroup(groupBox);

	for (int i = 0; i < COUNT_CAM; ++i) {
		QPushButton *button = new QPushButton(tr("Камера %1").arg(i + 1));
		button->setMinimumWidth(100);
		button->setMinimumHeight(150);
		button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		group->addButton(button);
		layout->addWidget(button);
	}

	groupBox->setLayout(layout);
	connect(group, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(onClick(QAbstractButton*)));

	return groupBox;
}

QWidget* QFFPlayer::createErrorPage() {
	QWidget *w = new QWidget;
	QPalette pal(palette());

	pal.setColor(QPalette::Background, Qt::black);
	w->setAutoFillBackground(true);
	w->setPalette(pal);

	QVBoxLayout* layout = new QVBoxLayout(w);
	QLabel* label = new QLabel(tr("Камера недоступна"));
	label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter | Qt::AlignCenter);

	pal.setColor(QPalette::WindowText, Qt::red);
	label->setPalette(pal);

	layout->addWidget(label);
	w->setLayout(layout);
	w->setMouseTracking(true);

	return w;
}

void QFFPlayer::onClick(QAbstractButton *button) {
	int camera = button->text().split(" ")[1].toInt() - 1;
	changeLayout();
	sdlWidget->changeCamera(camera);
}

void QFFPlayer::mouseReleaseEvent(QMouseEvent *event) {
	changeLayout();
	if (sdlWidget->getCamCount() == COUNT_CAM) sdlWidget->changeCamera();
}

QFFPlayer::~QFFPlayer() {
	sdlWidget->closeFfmpeg();
}

void QFFPlayer::changeLayout() {
	changeView((sdlWidget->getCamCount() != COUNT_CAM) ? 1 : 0);
}

void QFFPlayer::showError() {
	changeView(2);
}

/**
* 0 - task widget
* 1 - camera widget
* 2 - error widget
*/
void QFFPlayer::changeView(int state) {
	switch (state) {

	case 0:
		if (stackedWidget->currentIndex() != tasksWidget) {
			sdlWidget->stopPlaying();
			stackedWidget->setCurrentIndex(tasksWidget);
		}
		return;
	case 1:
		if (stackedWidget->currentIndex() != cameraWidget) {
			stackedWidget->setCurrentIndex(cameraWidget);
		}
		return;
	case 2:
		if (stackedWidget->currentIndex() != errorWidget) {
			stackedWidget->setCurrentIndex(errorWidget);
		}
		return;
	default:
		return;
	}

}
