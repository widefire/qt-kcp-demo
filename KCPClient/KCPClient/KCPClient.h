#pragma once

#include <QObject>
#include <qudpsocket.h>
#include "KCPNewwork.h"

class KCPClient : public QObject
{
	Q_OBJECT

public:
	KCPClient(QObject *parent,QString addr,int port);
	~KCPClient();
	int KCPWrite(const char *buf, int len);
	int WriteData(const char *buf, int len, KCPNewwork *kcpNet);
	private slots:
	void readPendingDatagrams();
private:
	QUdpSocket m_udpSocket;
	int m_port;
	QString m_addr;
	KCPNewwork *m_kcp;
};

int KCP_OUTPUT_Callback(const char *buf, int len, ikcpcb *kcp, void *user);
