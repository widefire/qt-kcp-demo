#pragma once

#include <QObject>
#include <string>
#include <qudpsocket.h>
#include <mutex>
#include <thread>
#include <qtimer>
#include <map>
#include "KCPNetInfo.h"


//just support one client now
class IKCPNetworker : public QObject
{
	Q_OBJECT

	friend int KCP_Networker_Callback(const char *buf, int len, ikcpcb *kcp, void *user);
public:
	//for server
	IKCPNetworker(int port=-1);
	//for client
	IKCPNetworker(std::string peerAddr, int port,int id);
	/*
	auto bind ,qt auto bind will failed?
	*/
	int Port();
	//for client send up layer
	int WriteData(const char *buf, int len);
	virtual ~IKCPNetworker();
signals:
	void ProcessDatagramsSignal(const char *buf,int len);
protected:
	//user def process datagrams

	virtual void ProcessDatagrams(const char *data, int len);
	private slots:
	void readPendingDatagrams();
	void KCPUpdate();
private:
	void InitAndConnectEvents();
	//called by kcp callback
	int KCPWrite(const char *buf, int len, KCPNetInfo *kcpNet);

private:
	//the recv mtu must bigger than transform mtu
	static const int s_MTU = 1400;
	static const int s_max_send_que = 100000;
	static const int s_KCP_ID = 1;
	QTimer m_timer;
	QUdpSocket m_udpSocket;
	bool m_isServer=false;
	int m_id = -1;
	//client
	KCPNetInfo *m_kcp = nullptr;
	std::string m_peerAddr;
	int m_peerPort;
	//server
	std::mutex m_muxKCP;
	std::map<std::string, KCPNetInfo*> m_mapKCP;
	int m_port = 0;
};
