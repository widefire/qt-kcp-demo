#include <QtCore/QCoreApplication>
#include "KCPClient.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	KCPClient client(nullptr, "127.0.0.1", 3001);
	char buf[1000];
	int size = 1000;
	client.KCPWrite(buf, size);
	return a.exec();
}
