#pragma once
extern "C" {
#include "KCP/ikcp.h"
}
#include <string>
class KCPNewwork
{
public:
	KCPNewwork(std::string addr,int port,int kcpId,std::string uuid,void *param);
	~KCPNewwork();
	std::string addr;
	int port;
	int kcpId;
	std::string uuid;
	void *param;
	ikcpcb *ikcp;
};

