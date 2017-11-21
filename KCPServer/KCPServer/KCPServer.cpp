#include "KCPServer.h"

KCPServer::KCPServer(QObject *parent,int port)
	: QObject(parent),
	m_port(port),
	m_kcp(nullptr)
{
	bool result=m_udpSocket.bind(QHostAddress::LocalHost,port);
	QObject::connect(&m_udpSocket, SIGNAL(readyRead()),
		this, SLOT(readPendingDatagrams()));
}

KCPServer::~KCPServer()
{
	if (m_udpSocket.isOpen())
	{
		m_udpSocket.close();
	}
	if (m_kcp!=nullptr)
	{
		delete m_kcp;
		m_kcp = nullptr;
	}
}

int KCPServer::KCPWrite(const char * buf, int len)
{
	if (m_kcp!=nullptr)
	{
		return ikcp_send(m_kcp->ikcp, buf, len);
		
	}
	return 0;
}

int KCPServer::WriteData(const char * buf, int len, KCPNewwork * kcpNet)
{
	QHostAddress addr;
	addr.setAddress(QString::fromStdString( kcpNet->addr));
	return m_udpSocket.writeDatagram(buf, len, addr, kcpNet->port);
}

void KCPServer::readPendingDatagrams() {
	while (m_udpSocket.hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(m_udpSocket.pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		m_udpSocket.readDatagram(datagram.data(), datagram.size(),
			&sender, &senderPort);
		int ssrc = 12345;
		if (m_kcp==nullptr)
		{
			m_kcp = new KCPNewwork(sender.toString().toStdString(), senderPort, ssrc, "", this);
		}
		//input data to ikcp
		ikcp_input(m_kcp->ikcp, datagram.data(), datagram.size());
		//read raw data from ikcp
		char buf[1400];
		while (true)
		{
			int hr = ikcp_recv(m_kcp->ikcp, buf, 1400);
			if (hr<0)
			{
				break;
			}
			//real data;
		}
		//processTheDatagram(datagram);
	}
}

int KCP_OUTPUT_Callback(const char * buf, int len, ikcpcb * kcp, void * user)
{
	auto kcpNet = (KCPNewwork*)user;
	auto svr = (KCPServer*)kcpNet->param;

	return svr->WriteData(buf, len, kcpNet);
}
