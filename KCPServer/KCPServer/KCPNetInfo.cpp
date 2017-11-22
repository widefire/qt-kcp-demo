#include "KCPNetInfo.h"


KCPNetInfo::KCPNetInfo(std::string addr, int port, int kcpId, std::string uuid, void * param):
	addr(addr),
	port(port),
	kcpId(kcpId),
	uuid(uuid),
	param(param),
	ikcp(nullptr)
{
	ikcp = ikcp_create(kcpId, this);
	switch (s_KCP_MODE)
	{
	case 0:
		ikcp_nodelay(ikcp, 0, 10, 0, 0);
		break;
	case 1:
		ikcp_nodelay(ikcp, 0, 10, 0, 1);
		break;
	case 2:
		ikcp_nodelay(ikcp, 1, 10, 2, 1);
		ikcp->rx_minrto = 10;
		ikcp->fastresend = 1;
		break;
	default:
		break;
	}
}

KCPNetInfo::~KCPNetInfo()
{
	ikcp_flush(ikcp);
	ikcp_release(ikcp);
}
