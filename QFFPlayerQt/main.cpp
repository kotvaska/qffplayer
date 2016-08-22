#include "qffplayer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QFFPlayer w;
	w.show();
	return a.exec();
}
