#include <QtCore/QCoreApplication>
#include "QKCPNetworker.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	QKCPNetworker svr(3001);
	char buf[1000];
	int size = 1000;

	return a.exec();
}
