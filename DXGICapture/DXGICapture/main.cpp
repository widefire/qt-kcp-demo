#include "DXGICapture.h"
#include <QtWidgets/QApplication>
#include "DXGIDevices.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	DXGICapture w;
	w.show();
	DXGIDevices dx;
	dx.GetFrame();
	//w.CaptureFrame();
	return a.exec();
}
