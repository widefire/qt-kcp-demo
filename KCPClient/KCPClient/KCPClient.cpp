#include "KCPClient.h"

KCPClient::KCPClient(QObject *parent, QString addr, int port)
	: QObject(parent),
	m_port(port),
	m_kcp(nullptr),
	m_addr(addr)
{
	QObject::connect(&m_udpSocket, SIGNAL(readyRead()),
		this, SLOT(readPendingDatagrams()));
}

KCPClient::~KCPClient()
{
	if (m_udpSocket.isOpen())
	{
		m_udpSocket.close();
	}
	if (m_kcp != nullptr)
	{
		delete m_kcp;
		m_kcp = nullptr;
	}
}

int KCPClient::KCPWrite(const char * buf, int len)
{
	int result = 0;
	if (m_kcp == nullptr)
	{
		int ssrc = 12345;
		m_kcp = new KCPNewwork(m_addr.toStdString(), m_port, ssrc, "", this);
		m_kcp->ikcp->output = KCP_OUTPUT_Callback;
	}
	if (m_kcp != nullptr)
	{
		result=ikcp_send(m_kcp->ikcp, buf, len);
		ikcp_flush(m_kcp->ikcp);
	}
	return result;
}

int KCPClient::WriteData(const char * buf, int len, KCPNewwork * kcpNet)
{
	QHostAddress addr;
	addr.setAddress(m_addr);
	return m_udpSocket.writeDatagram(buf, len, addr, m_port);
}

void KCPClient::readPendingDatagrams() {
	while (m_udpSocket.hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(m_udpSocket.pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		m_udpSocket.readDatagram(datagram.data(), datagram.size(),
			&sender, &senderPort);
		int ssrc = 12345;
		if (m_kcp == nullptr)
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
	auto client = (KCPClient*)kcpNet->param;

	return client->WriteData(buf, len, kcpNet);
}

