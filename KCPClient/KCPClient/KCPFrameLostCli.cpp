#include "KCPFrameLostCli.h"
#include "timeUtils.h"

KCPFrameLostCli::KCPFrameLostCli(std::string peerAddr, int port, int id)
	: IKCPNetworker(peerAddr,port,id)
{
}

KCPFrameLostCli::~KCPFrameLostCli()
{
	if (m_threadSend!=nullptr)
	{
		if (m_threadSend->joinable())
		{
			m_threadSend->join();
		}
		delete m_threadSend;
		m_threadSend = nullptr;
	}
}

void KCPFrameLostCli::SendFrames()
{
	m_sending = true;


	if (nullptr==m_threadSend)
	{
		m_threadSend = new std::thread(std::mem_fun(&KCPFrameLostCli::SendThread), this);
	}
}

void KCPFrameLostCli::SendThread()
{
	m_sending = true;
	unsigned char buf[10000];
	int size = 10000;
	int idx=0;
	while (m_sending)
	{
		isleep(1);
		buf[0] = (idx >> 24) & 0xff;
		buf[1] = (idx >> 16) & 0xff;
		buf[2] = (idx >> 8) & 0xff;
		buf[3] = (idx >> 0) & 0xff;
		if (buf[3]==-128)
		{
			idx = idx;
		}
		idx++;
		WriteData((char*)buf, size);
	}
}
