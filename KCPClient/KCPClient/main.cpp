#include <QtCore/QCoreApplication>
#include "QKCPNetworker.h"
#include "timeUtils.h"
#include "KCPFrameLostCli.h"


int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	//QKCPNetworker client( "127.0.0.1", 3001,12345);
	//std::string addr = "127.0.0.1";
	std::string addr = "192.168.20.229";
	if (argc>1)
	{
		addr = argv[1];
	}

	KCPFrameLostCli cli(addr, 3001, 12345);
	cli.SendFrames();
	return a.exec();
}
