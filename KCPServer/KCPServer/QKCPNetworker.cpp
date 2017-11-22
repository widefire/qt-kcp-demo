#include "QKCPNetworker.h"
#include "KCPNewwork.h"
#include "timeUtils.h"

QKCPNetworker::QKCPNetworker(int port) :QObject(nullptr)
, m_port(port)
{
	m_isServer = true;
	bool result = m_udpSocket.bind(QHostAddress::LocalHost, port);
	InitAndConnectEvents();
	/*auto ret=QObject::connect(this, &QKCPNetworker::AsyncSendSignal,
		this, &QKCPNetworker::AsyncSendSlot, Qt::BlockingQueuedConnection);
	

	m_threadKcpUpdate = new std::thread(std::mem_fun(&QKCPNetworker::threadKCPUpdate), this);*/
}

QKCPNetworker::QKCPNetworker(std::string peerAddr, int port, int id) : QObject(nullptr)
, m_port(port)
, m_id(id)
, m_peerAddr(peerAddr)
{
	m_isServer = false;
	//create kcp
	int ssrc = 12345;
	m_kcp = new KCPNewwork(m_peerAddr, m_port, ssrc, "", this);
	m_kcp->ikcp->output = KCP_Networker_Callback;
	InitAndConnectEvents();
	/*QObject::connect(&m_udpSocket, SIGNAL(readyRead()),
		this, SLOT(readPendingDatagrams()));
	auto ret = QObject::connect(this, &QKCPNetworker::AsyncSendSignal,
			this, &QKCPNetworker::AsyncSendSlot, Qt::BlockingQueuedConnection);

	m_threadKcpUpdate = new std::thread(std::mem_fun(&QKCPNetworker::threadKCPUpdate), this);*/
}

int QKCPNetworker::WriteData(const char * buf, int len)
{
	std::lock_guard<std::mutex> guard(m_muxKCP);
	int ret = 0;
	if (m_kcp!=nullptr)
	{
		ret = ikcp_send(m_kcp->ikcp, buf, len);;
	}
	return ret;
}

int QKCPNetworker::KCPWrite(const char * buf, int len, KCPNewwork * kcpNet)
{
	//std::lock_guard<std::mutex> guard(m_muxKCP);
	QHostAddress addr;
	addr.setAddress(QString::fromStdString(kcpNet->addr));
	//call signal
	/*int result=AsyncSendSignal(buf, len, addr, kcpNet->port);
	return result;*/
	int result = m_udpSocket.writeDatagram(buf, len, addr, kcpNet->port);
	return result;
}

QKCPNetworker::~QKCPNetworker()
{
	std::lock_guard<std::mutex> guard(m_muxKCP);

	if (m_kcp!=nullptr)
	{
		delete m_kcp;
		m_kcp = nullptr;
	}
}

void QKCPNetworker::ProcessDatagrams(const char * data, int len)
{
	
}

int QKCPNetworker::AsyncSendSlot(const char *data, int len, QHostAddress addr, int port)
{
	int ret= m_udpSocket.writeDatagram(data, len, addr, port);
	return ret;
}

void QKCPNetworker::InitAndConnectEvents()
{

	QObject::connect(&m_udpSocket, SIGNAL(readyRead()),
		this, SLOT(readPendingDatagrams()));
	QObject::connect(&m_timer, &QTimer::timeout,
		this, &QKCPNetworker::KCPUpdate);
	m_timer.start(10);
}


void QKCPNetworker::readPendingDatagrams() {

	while (m_udpSocket.hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(m_udpSocket.pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		m_udpSocket.readDatagram(datagram.data(), datagram.size(),
			&sender, &senderPort);
		//the ssrc for one conn now
		//server
		int ssrc = 12345;
		if (m_kcp == nullptr)
		{
			m_kcp = new KCPNewwork(sender.toString().toStdString(), senderPort, ssrc, "", this);
			m_kcp->ikcp->output = KCP_Networker_Callback;
			//wrong here
		}

		//input data to ikcp
		ikcp_input(m_kcp->ikcp, datagram.data(), datagram.size());
		//read raw data from ikcp
		char buf[s_MTU];
		while (true)
		{
			int hr = ikcp_recv(m_kcp->ikcp, buf, s_MTU);
			if (hr<0)
			{
				break;
			}
			ProcessDatagrams(buf, hr);
		}

	}
}

void QKCPNetworker::KCPUpdate()
{
	m_muxKCP.lock();
	if (m_kcp!=nullptr)
	{
		char buf[1000];
		int size = 1000;
		auto clock = iclock();
		ikcp_update(m_kcp->ikcp, clock);
		ikcp_send(m_kcp->ikcp, buf, size);
	}
	m_muxKCP.unlock();
}

int KCP_Networker_Callback(const char * buf, int len, ikcpcb * kcp, void * user)
{
	auto kcpNetwork = (KCPNewwork*)user;
	auto sender = (QKCPNetworker*)kcpNetwork->param;
	int ret = sender->KCPWrite(buf, len, kcpNetwork);
	return ret;
}
