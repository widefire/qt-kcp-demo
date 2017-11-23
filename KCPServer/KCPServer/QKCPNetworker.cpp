#include "QKCPNetworker.h"
#include "KCPNetInfo.h"
#include "timeUtils.h"

IKCPNetworker::IKCPNetworker(int port) :QObject(nullptr)
, m_port(port)
{
	m_isServer = true;
	if (port<0)
	{
		//qt bind 0 will failed
		static int sport = 1025;
		auto result=m_udpSocket.bind(QHostAddress::Any,sport);
		int lastsport = sport;
		while (!result&&port<65535)
		{
			result = m_udpSocket.bind(QHostAddress::Any, ++sport);
			if (lastsport==sport)
			{
				//looped bind failed
				break;
			}
			if (sport==65535)
			{
				sport = 1025;
			}
		}
		m_port = m_udpSocket.localPort();
	}
	else
	{
		m_udpSocket.bind(QHostAddress::Any, port);
	}
	InitAndConnectEvents();
}

IKCPNetworker::IKCPNetworker(std::string peerAddr, int port, int id) : QObject(nullptr)
, m_peerPort(port)
, m_id(id)
, m_peerAddr(peerAddr)
{
	m_isServer = false;
	//create kcp
	m_kcp = new KCPNetInfo(m_peerAddr, m_peerPort, s_KCP_ID, "", this);
	m_kcp->ikcp->output = KCP_Networker_Callback;
	InitAndConnectEvents();
}

int IKCPNetworker::Port()
{
	return m_udpSocket.localPort();
}

int IKCPNetworker::WriteData(const char * buf, int len)
{
	std::lock_guard<std::mutex> guard(m_muxKCP);
	int ret = 0;
	if (m_kcp!=nullptr)
	{
		if (m_kcp->ikcp->nsnd_que>s_max_send_que)
		{
			//bad network
			return -1;
		}
		ret = ikcp_send(m_kcp->ikcp, buf, len);;
	}
	return ret;
}

int IKCPNetworker::KCPWrite(const char * buf, int len, KCPNetInfo * kcpNet)
{
	QHostAddress addr;
	addr.setAddress(QString::fromStdString(kcpNet->addr));
	int result = m_udpSocket.writeDatagram(buf, len, addr, kcpNet->port);
	return result;
}

IKCPNetworker::~IKCPNetworker()
{
	m_timer.stop();
	std::lock_guard<std::mutex> guard(m_muxKCP);

	if (m_kcp!=nullptr)
	{
		delete m_kcp;
		m_kcp = nullptr;
	}
	for (auto i:m_mapKCP)
	{
		delete i.second;
	}
	m_mapKCP.clear();
}

void IKCPNetworker::ProcessDatagrams(const char * data, int len)
{
	
}


void IKCPNetworker::InitAndConnectEvents()
{

	QObject::connect(&m_udpSocket, SIGNAL(readyRead()),
		this, SLOT(readPendingDatagrams()));
	QObject::connect(&m_timer, &QTimer::timeout,
		this, &IKCPNetworker::KCPUpdate);
	m_timer.start(10);
}


void IKCPNetworker::readPendingDatagrams() {

	while (m_udpSocket.hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(m_udpSocket.pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		m_udpSocket.readDatagram(datagram.data(), datagram.size(),
			&sender, &senderPort);

		

		//input data to ikcp

		ikcpcb *ptrKcp = nullptr;
		if (m_isServer)
		{
			std::string kcpName = sender.toString().toStdString() + ":" + std::to_string(senderPort);
			//
			std::lock_guard<std::mutex> guard(m_muxKCP);
			auto it = m_mapKCP.find(kcpName);
			if (it == m_mapKCP.end())
			{
				auto tmpKCP = new KCPNetInfo(sender.toString().toStdString(), senderPort, s_KCP_ID, kcpName, this);
				tmpKCP->ikcp->output = KCP_Networker_Callback;
				m_mapKCP[kcpName] = tmpKCP;
				it = m_mapKCP.find(kcpName);
			}
			ptrKcp = it->second->ikcp;
		}
		else
		{
			ptrKcp = m_kcp->ikcp;
		}

		ikcp_input(ptrKcp, datagram.data(), datagram.size());
		//read raw data from ikcp
		char buf[s_MTU];
		while (true)
		{
			int hr = ikcp_recv(ptrKcp, buf, s_MTU);
			if (hr<0)
			{
				break;
			}
			ProcessDatagramsSignal(buf,hr);
			ProcessDatagrams(buf, hr);
		}

	}
}

void IKCPNetworker::KCPUpdate()
{
	m_muxKCP.lock();
	auto clock = iclock();
	if (m_isServer)
	{
		for (auto i : m_mapKCP)
		{
			ikcp_update(i.second->ikcp, clock);
		}
	}
	else
	{
		ikcp_update(m_kcp->ikcp, clock);
	}
	m_muxKCP.unlock();
}

int KCP_Networker_Callback(const char * buf, int len, ikcpcb * kcp, void * user)
{
	auto kcpNetwork = (KCPNetInfo*)user;
	auto sender = (IKCPNetworker*)kcpNetwork->param;
	int ret = sender->KCPWrite(buf, len, kcpNetwork);
	return ret;
}
