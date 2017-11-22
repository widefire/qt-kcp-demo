#include <QtCore/QCoreApplication>
#include "QKCPNetworker.h"
#include "timeUtils.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	QKCPNetworker client( "127.0.0.1", 3001,12345);
	char buf[1000];
	int size = 1000;
	for (int i = 0; i < 1; i++)
	{
		isleep(2000);
		client.WriteData(buf, size);
	}
	return a.exec();
}
