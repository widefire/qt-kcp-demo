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
	void DrawMouse(D3D11_MAPPED_SUBRESOURCE mapedResource, unsigned char *ptrScreen);
	void DrawMouseMonochrome(D3D11_MAPPED_SUBRESOURCE mapedResource,unsigned char *ptrScreen);
	void ProcessMonochromeMouse(D3D11_MAPPED_SUBRESOURCE mapedResource, unsigned char *ptrScreen);
	void DrawMouseColor(D3D11_MAPPED_SUBRESOURCE mapedResource, unsigned char *ptrScreen);
	void DrawMouseMaskedColor(D3D11_MAPPED_SUBRESOURCE mapedResource, unsigned char *ptrScreen);
	//by gdi ,not good
	void DrawMouse2(D3D11_MAPPED_SUBRESOURCE mapedResource, unsigned char *ptrScreen);
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
	unsigned char *m_MouseBufferResult = nullptr;
	unsigned int m_MouseCacheSize = 0;
	DXGI_OUTDUPL_POINTER_SHAPE_INFO	m_MouseInfo;
	DXGI_OUTDUPL_POINTER_POSITION m_MousePosition;
	DXGI_FORMAT m_textureFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
};
LPBYTE GetCursorData(HICON hIcon, ICONINFO &ii, UINT &width, UINT &height);

static LPBYTE GetBitmapData(HBITMAP hBmp, BITMAP &bmp)
{
	if (!hBmp)
		return NULL;

	if (GetObject(hBmp, sizeof(bmp), &bmp) != 0) {
		UINT bitmapDataSize = bmp.bmHeight*bmp.bmWidth*bmp.bmBitsPixel;
		bitmapDataSize >>= 3;

		LPBYTE lpBitmapData = new BYTE[bitmapDataSize];
		GetBitmapBits(hBmp, bitmapDataSize, lpBitmapData);

		return lpBitmapData;
	}

	return NULL;
}

static inline BYTE BitToAlpha(LPBYTE lp1BitTex, int pixel, bool bInvert)
{
	BYTE pixelByte = lp1BitTex[pixel / 8];
	BYTE pixelVal = pixelByte >> (7 - (pixel % 8)) & 1;

	if (bInvert)
		return pixelVal ? 0xFF : 0;
	else
		return pixelVal ? 0 : 0xFF;
}


