#pragma once

#include <QObject>
#include <string>
#include <qudpsocket.h>
#include <mutex>
#include <thread>
#include <qtimer>
#include "KCPNewwork.h"


int KCP_Networker_Callback(const char *buf, int len, ikcpcb *kcp, void *user);
//just support one client now
class QKCPNetworker : public QObject
{
	Q_OBJECT

public:
	//for server
	QKCPNetworker(int port);
	//for client
	QKCPNetworker(std::string peerAddr, int port,int id);
	//for client send up layer
	int WriteData(const char *buf, int len);
	//called by kcp callback
	int KCPWrite(const char *buf, int len, KCPNewwork *kcpNet);
	virtual ~QKCPNetworker();
signals:
	void ProcessDatagramsSignal(const char *data, int len);
protected:
	//user def process datagrams

	virtual void ProcessDatagrams(const char *data, int len);
	private slots:
	void readPendingDatagrams();
	void KCPUpdate();
private:
	void InitAndConnectEvents();

private:
	static const int s_MTU = 1400;
	static const int s_max_send_que = 100000;
	QTimer m_timer;
	QUdpSocket m_udpSocket;
	bool m_isServer=false;
	int m_port = 0;
	int m_id = -1;
	KCPNewwork *m_kcp = nullptr;
	std::string m_peerAddr;
	std::mutex m_muxKCP;
};
