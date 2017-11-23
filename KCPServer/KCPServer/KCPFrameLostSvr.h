#pragma once

#include "QKCPNetworker.h"

class KCPFrameLostSvr : public IKCPNetworker
{
	Q_OBJECT

public:
	KCPFrameLostSvr(int port);
	~KCPFrameLostSvr();
private:
	virtual void ProcessDatagrams(const char *data, int len)override;
};
