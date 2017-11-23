#include "KCPFrameLostSvr.h"
#include <iostream>

KCPFrameLostSvr::KCPFrameLostSvr(int port)
	: IKCPNetworker(port)
{
}

KCPFrameLostSvr::~KCPFrameLostSvr()
{
}

void KCPFrameLostSvr::ProcessDatagrams(const char * data, int len)
{
	static int NextIdx = 0;
	if (len>4)
	{
		auto ptr = (unsigned char*)data;
		int tmp = unsigned int(ptr[0]) << 24;
		tmp |= unsigned int(ptr[1]) << 16;
		tmp |= unsigned int(ptr[2]) << 8;
		tmp |= unsigned int(ptr[3]) << 0;
		if (tmp!= NextIdx)
		{
			int a0 = data[0];
			int a1 = data[1];
			int a2 = data[2];
			int a3 = data[3];
			std::cout << tmp << std::endl;
		}
		NextIdx = tmp+1;
	}
}
