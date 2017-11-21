#include "KCPNewwork.h"


KCPNewwork::KCPNewwork(std::string addr, int port, int kcpId, std::string uuid, void * param):
	addr(addr),
	port(port),
	kcpId(kcpId),
	uuid(uuid),
	param(param),
	ikcp(nullptr)
{
	ikcp = ikcp_create(kcpId, this);
}

KCPNewwork::~KCPNewwork()
{
	ikcp_release(ikcp);
}
