#pragma once
extern "C" {
#include "IKCP/ikcp.h"
}
#include <string>
class KCPNetInfo
{
public:
	KCPNetInfo(std::string addr,int port,int kcpId,std::string uuid,void *param);
	~KCPNetInfo();
	std::string addr;
	int port;
	int kcpId;
	std::string uuid;
	void *param;
	ikcpcb *ikcp;
	/*
	kcp mode :
	0: default
	1: normal
	2: first
	*/
	static const int s_KCP_MODE = 0;
};

