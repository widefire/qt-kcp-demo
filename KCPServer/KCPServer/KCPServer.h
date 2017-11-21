#pragma once

#include <QObject>
#include <qudpsocket.h>
#include "KCPNewwork.h"

//one connect only now
class KCPServer : public QObject
{
	Q_OBJECT

public:
	KCPServer(QObject *parent,int port);
	~KCPServer();
	int KCPWrite(const char *buf, int len);
	int WriteData(const char *buf, int len, KCPNewwork *kcpNet);
	private slots:
	void readPendingDatagrams();
private:
	QUdpSocket m_udpSocket;
	int m_port;
	KCPNewwork *m_kcp;
};

int KCP_OUTPUT_Callback(const char *buf, int len, ikcpcb *kcp, void *user);
