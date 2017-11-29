#pragma once

#include "QKCPNetworker.h"

class KCPFrameLostCli : public IKCPNetworker
{
	Q_OBJECT

public:
	KCPFrameLostCli(std::string peerAddr, int port, int id);
	~KCPFrameLostCli();
	void SendFrames();
private:
	void SendThread();
	std::thread *m_threadSend = nullptr;
	volatile bool m_sending = false;
};
