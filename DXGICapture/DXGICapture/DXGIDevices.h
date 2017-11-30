#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
class DXGIDevices
{
public:
	DXGIDevices();
	~DXGIDevices();
	bool Valid();
	HRESULT GetFrame(unsigned char **data, int sizeIn, int &pitch,int &totalSize);
	DXGI_FORMAT FrameFormat();
private:
	int InitializeDx();
	void ReleaseDx();
	int RecreateTexture();
	int InitDuplication();
	void ReleaseDuplication();
	HRESULT GetMouseBuffer();
	void DrawMouse();
public:
	bool m_inited = false;
	ID3D11Device *m_Device = nullptr;
	ID3D11DeviceContext *m_DeviceContext = nullptr;
	ID3D11Texture2D *m_DesktopImg = nullptr;
	ID3D11Texture2D *m_AcquiredDesktopImage = nullptr;
	IDXGIOutputDuplication *m_DesktopDuplication = nullptr;
	DXGI_OUTPUT_DESC m_OutputDesc;

	unsigned int m_MouseBufferSize = 0;//in bytes
	unsigned char *m_MouseBuffer = nullptr;
	unsigned int m_MouseCacheSize = 0;
	DXGI_OUTDUPL_POINTER_SHAPE_INFO	m_MouseInfo;
	DXGI_FORMAT m_textureFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
};

