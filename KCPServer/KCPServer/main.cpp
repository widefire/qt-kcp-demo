#include <QtCore/QCoreApplication>
#include "KCPServer.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	KCPServer svr(nullptr,3001);
	char buf[1000];
	int size = 1000;
	svr.KCPWrite(buf, size);
	return a.exec();
}
