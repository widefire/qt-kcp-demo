#include "DXGIDevices.h"
#include <stdio.h>
#include <vector>

#include "LOG4Z/log4z.h"
#include "CommonUtils/BitReader.h"



DXGIDevices::DXGIDevices()
{
	m_inited = false;
	if (0 == InitializeDx()) {
		if (0 == InitDuplication()) {
			m_inited = true;
		}
	}
}


DXGIDevices::~DXGIDevices()
{
	ReleaseDx();
	if (m_MouseBuffer!=nullptr)
	{
		delete[]m_MouseBuffer;
		m_MouseBuffer = nullptr;
	}
	if (m_MouseBufferResult!=nullptr)
	{
		delete[]m_MouseBufferResult;
		m_MouseBufferResult = nullptr;
	}
}

bool DXGIDevices::Valid()
{
	return m_inited;
}

HRESULT DXGIDevices::GetFrame(unsigned char **data, int sizeIn, int &pitch, int &totalSize)
{
	HRESULT hr = S_OK;
	bool frameAcquired = false;
	do
	{
		if (data==nullptr)
		{
			hr = E_FAIL;
			break;
		}

		IDXGIResource* DesktopResource = nullptr;
		DXGI_OUTDUPL_FRAME_INFO FrameInfo;

		if (m_AcquiredDesktopImage)
		{
			m_AcquiredDesktopImage->Release();
			m_AcquiredDesktopImage = nullptr;
		}
		hr =  m_DesktopDuplication->AcquireNextFrame(10, &FrameInfo, &DesktopResource);
		if (FAILED(hr))
		{
			if (hr== DXGI_ERROR_WAIT_TIMEOUT)
			{
				//screen no update
				//totalSize = sizeIn;
				//pitch = pitch;
				//hr = S_OK;
			}
			else if(hr== DXGI_ERROR_ACCESS_LOST)
			{
				//device losted;
				//need recreate the device
			}
			break;
		}

		
		frameAcquired = true;
		hr = DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(& m_AcquiredDesktopImage));
		
		DesktopResource->Release();
		DesktopResource = nullptr;
		if (FAILED(hr))
		{
			break;
		}

		RecreateTexture();
		
		 m_DeviceContext->CopyResource( m_DesktopImg,  m_AcquiredDesktopImage);
		

		D3D11_MAPPED_SUBRESOURCE mapedResource;
		hr =  m_DeviceContext->Map( m_DesktopImg, 0, D3D11_MAP::D3D11_MAP_READ, 0, &mapedResource);
		if (FAILED(hr))
		{
			break;
		}
		
		if (1)
		{
			if (sizeIn != mapedResource.DepthPitch)
			{
				if (sizeIn != 0 || *data != nullptr)
				{
					delete[] * data;
					*data = nullptr;
				}
				*data = new unsigned char[mapedResource.DepthPitch];
			}
			memcpy(*data, mapedResource.pData, mapedResource.DepthPitch);
			pitch = mapedResource.RowPitch;
			totalSize = mapedResource.DepthPitch;
			//use bit or to draw mouse
			//the visible mean in the picture
			//if (FrameInfo.PointerPosition.Visible)
			CURSORINFO ci;
			ZeroMemory(&ci, sizeof(ci));
			ci.cbSize = sizeof(ci);
			BOOL bMouseCaptured = GetCursorInfo(&ci);
			if(bMouseCaptured)
			{
				if (FrameInfo.PointerPosition.Visible)
				{
					m_MousePosition = FrameInfo.PointerPosition;
				}
				else
				{
					/*m_MousePosition.Position.x = ci.ptScreenPos.x;
					m_MousePosition.Position.y = ci.ptScreenPos.y;*/
				}
				m_MousePosition.Position.x = m_MousePosition.Position.x > 0 ? m_MousePosition.Position.x : 0;
				m_MousePosition.Position.y = m_MousePosition.Position.y > 0 ? m_MousePosition.Position.y : 0;
				hr = GetMouseBuffer();
				if (SUCCEEDED(hr))
				{
					DrawMouse(mapedResource,*data);
				}
				else
				{
					//mouse error
					LOGF(" get mouse buffer failed "<<hr);
				}
			}
			else
			{
				LOGF("mouse un visible");
			}
			//DrawMouse(mapedResource, *data);

			//!use bit or to draw mouse
		}
		
		m_DeviceContext->Unmap( m_DesktopImg, 0);

	} while (0);

	if (frameAcquired)
	{
		m_DesktopDuplication->ReleaseFrame();
	}
	return hr;
}

DXGI_FORMAT DXGIDevices::FrameFormat()
{
	return m_textureFormat;
}

int DXGIDevices::InitializeDx()
{
	int ret = -1;
	do
	{
		HRESULT hr = S_OK;
		D3D_DRIVER_TYPE DriverTypes[] =
		{
			D3D_DRIVER_TYPE_HARDWARE,
			D3D_DRIVER_TYPE_WARP,
			D3D_DRIVER_TYPE_REFERENCE,
		};
		UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

		D3D_FEATURE_LEVEL FeatureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_1
		};
		UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

		D3D_FEATURE_LEVEL FeatureLevel;
		for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
		{
			hr = D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, 0, FeatureLevels, NumFeatureLevels,
				D3D11_SDK_VERSION, &m_Device, &FeatureLevel, &m_DeviceContext);
			if (SUCCEEDED(hr))
			{
				break;
			}
		}
		if (FAILED(hr))
		{
			break;
		}


		m_textureFormat = DXGI_FORMAT_B8G8R8A8_UNORM;

		D3D11_TEXTURE2D_DESC textureDesc;
		textureDesc.Width = GetSystemMetrics(SM_CXSCREEN);
		textureDesc.Height = GetSystemMetrics(SM_CYSCREEN);
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = m_textureFormat;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_STAGING;
		textureDesc.BindFlags = 0;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		textureDesc.MiscFlags = 0;

		hr=m_Device->CreateTexture2D(&textureDesc, nullptr, &m_DesktopImg);

		if (FAILED(hr))
		{
			m_textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			hr = m_Device->CreateTexture2D(&textureDesc, nullptr, &m_DesktopImg);
			if (FAILED(hr))
			{
				break;
			}
		}

		ret = 0;
	} while (0);
	return ret;
}

void DXGIDevices::ReleaseDx()
{
	if (m_Device!=nullptr)
	{
		m_Device->Release();
		m_Device = nullptr;
	}
	if (m_DeviceContext!=nullptr)
	{
		m_DeviceContext->Release();
		m_Device = nullptr;
	}
	if (m_DesktopImg!=nullptr)
	{
		m_DesktopImg->Release();
		m_DesktopImg = nullptr;
	}
	if (m_AcquiredDesktopImage!=nullptr)
	{
		m_AcquiredDesktopImage->Release();
		m_AcquiredDesktopImage = nullptr;
	}
}

int DXGIDevices::RecreateTexture()
{
	if (!m_AcquiredDesktopImage)
	{
		return 0;
	}
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	m_AcquiredDesktopImage->GetDesc(&desc);

	if (desc.Format!=m_textureFormat)
	{
		if (m_DesktopImg != nullptr)
		{
			m_DesktopImg->Release();
			m_DesktopImg = nullptr;
		}
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.MiscFlags = 0;

		m_Device->CreateTexture2D(&desc, nullptr, &m_DesktopImg);
	}

	return 0;
}

int DXGIDevices::InitDuplication()
{
	int ret = -1;
	do
	{
		IDXGIDevice* DxgiDevice = nullptr;
		HRESULT hr = m_Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));
		if (FAILED(hr))
		{
			break;
		}

		IDXGIAdapter* DxgiAdapter = nullptr;
		hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
		DxgiDevice->Release();
		DxgiDevice = nullptr;
		if (FAILED(hr))
		{
			break;
		}

		IDXGIOutput* DxgiOutput = nullptr;
		//just first screen now
		unsigned int Output = 0;
		hr = DxgiAdapter->EnumOutputs(Output, &DxgiOutput);
		DxgiAdapter->Release();
		DxgiAdapter = nullptr;
		if (FAILED(hr))
		{
			break;
		}

		DxgiOutput->GetDesc(&m_OutputDesc);
		IDXGIOutput1* DxgiOutput1 = nullptr;
		hr = DxgiOutput->QueryInterface(__uuidof(DxgiOutput1), reinterpret_cast<void**>(&DxgiOutput1));
		DxgiOutput->Release();
		DxgiOutput = nullptr;
		if (FAILED(hr))
		{
			break;
		}

		// Create desktop duplication
		hr = DxgiOutput1->DuplicateOutput(m_Device, &m_DesktopDuplication);
		DxgiOutput1->Release();
		DxgiOutput1 = nullptr;
		if (FAILED(hr))
		{
			if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
			{
				//"There is already the maximum number of applications using the Desktop Duplication API running, please close one of those applications and then try again."
			}
			break;
		}

		ret = 0;
	} while (0);
	return ret;
}

void DXGIDevices::ReleaseDuplication()
{
	if (m_DesktopDuplication!=nullptr)
	{
		m_DesktopDuplication->Release();
		m_DesktopDuplication = nullptr;
	}
}

HRESULT DXGIDevices::GetMouseBuffer()
{
	HRESULT hr = S_OK;

	if (m_MouseBuffer==nullptr)
	{
		m_MouseCacheSize = 1;
		m_MouseBuffer = new unsigned char[m_MouseCacheSize];
	}
	UINT BufferSizeRequired = 0;
	auto lastInfo = m_MouseInfo;
	hr=m_DesktopDuplication->GetFramePointerShape(m_MouseCacheSize,
		m_MouseBuffer,
		&BufferSizeRequired,
		&m_MouseInfo);

	
	//need more data cache
	if (hr== DXGI_ERROR_MORE_DATA)
	{
		delete[]m_MouseBuffer;
		m_MouseCacheSize = BufferSizeRequired;
		m_MouseBuffer = new unsigned char[m_MouseCacheSize];
		hr = m_DesktopDuplication->GetFramePointerShape(m_MouseCacheSize,
			m_MouseBuffer,
			&BufferSizeRequired,
			&m_MouseInfo);
	}
	// if it is last status,no update
	if (BufferSizeRequired == 0)
	{
		m_MouseInfo = lastInfo;
	}
	else
	{
		m_MouseBufferSize = BufferSizeRequired;
	}
	if (FAILED(hr))
	{
		return hr;
	}

	return hr;
}

void DXGIDevices::DrawMouse(D3D11_MAPPED_SUBRESOURCE mapedResource, unsigned char *ptrScreen)
{

	switch (m_MouseInfo.Type)
	{
	case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME:
		/*The pointer type is a monochrome mouse pointer,
		which is a monochrome bitmap. 
		The bitmap's size is specified by width and height in a 1 bits per pixel (bpp) 
		device independent bitmap (DIB) format AND mask that is followed by 
		another 1 bpp DIB format XOR mask of the same size*/
		//ºÚ°×Î»Í¼
		DrawMouseMonochrome(mapedResource,ptrScreen);
		break;
	case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR:
		/*The pointer type is a color mouse pointer, which is a color bitmap.
		The bitmap's size is specified by width and height in a 32 bpp ARGB DIB format.*/
		//ARGB
		DrawMouseColor(mapedResource, ptrScreen);
		break;
	case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR:
		/*The pointer type is a masked color mouse pointer.
		A masked color mouse pointer is a 32 bpp ARGB format bitmap with the
		mask value in the alpha bits. The only allowed mask values are 0 and 0xFF.
		When the mask value is 0, the RGB value should replace the screen pixel.
		When the mask value is 0xFF, 
		an XOR operation is performed on the RGB value and the screen pixel;
		the result replaces the screen pixel.*/
		//ARGB : A==0 ,MOUSE     A==0xff ,MOUSE XOR SCREEN
		DrawMouseMaskedColor(mapedResource, ptrScreen);
		break;
	default:
		break;
	}
}

void DXGIDevices::DrawMouseMonochrome(D3D11_MAPPED_SUBRESOURCE mapedResource, unsigned char *ptrScreen)
{
	/*FILE *fp = fopen("MonochromeMouse", "wb");
	fwrite(m_MouseBuffer, 1, m_MouseBufferSize, fp);
	fclose(fp);*/
	ProcessMonochromeMouse(mapedResource, ptrScreen);
	/*
	1:8
	1 bit 1 byte
	*/
	int pos = m_MousePosition.Position.y*mapedResource.RowPitch +
		m_MousePosition.Position.x * 4;
	int posMax = (m_MousePosition.Position.y + 1)*mapedResource.RowPitch;
	posMax = posMax < mapedResource.DepthPitch ? posMax : mapedResource.DepthPitch;
	m_MouseBuffer;
	m_MouseBufferSize;
	int posCur = 0;
	for (int y = 0; y < m_MouseInfo.Height/2; y++)
	{
		for (int x = 0; x < m_MouseInfo.Width; x++)
		{
			if (pos + 5 * x<posMax)
			{
				ptrScreen[pos + 4 * x + 0] = m_MouseBufferResult[posCur++];
				ptrScreen[pos + 4 * x + 1 ] = m_MouseBufferResult[posCur++];
				ptrScreen[pos + 4 * x + 2 ] = m_MouseBufferResult[posCur++];
				ptrScreen[pos + 4 * x + 3 ] = m_MouseBufferResult[posCur++];
			}
		}
		pos += mapedResource.RowPitch;
		posMax += mapedResource.RowPitch;
		posMax = posMax < mapedResource.DepthPitch ? posMax : mapedResource.DepthPitch;
	}
	
}

void DXGIDevices::ProcessMonochromeMouse(D3D11_MAPPED_SUBRESOURCE mapedResource, unsigned char * ptrScreen)
{
	int height = m_MouseInfo.Height / 2;
	int width = m_MouseInfo.Width;
	UINT SkipX = 0;
	UINT SkipY = 0;
	UINT DesktopPitchInPixels = mapedResource.RowPitch/sizeof(UINT);//?
	if (m_MouseBufferResult!=nullptr)
	{
		delete[]m_MouseBufferResult;
	}
	m_MouseBufferResult = new unsigned char[width*height * 4];
	unsigned int *InitBuffer32=reinterpret_cast<UINT*>(m_MouseBufferResult);
	unsigned int *DesktopScreen32= reinterpret_cast<UINT*>(ptrScreen);
	for (int Row = 0,DskRow=m_MousePosition.Position.y; Row < height; Row++,DskRow++)
	{
		unsigned char Mask = 0x80;
		Mask = Mask >> (SkipX % 8);
		for (INT Col = 0,DskCol=m_MousePosition.Position.x; Col < width; ++Col, DskCol++)
		{
			// Get masks using appropriate offsets
			BYTE AndMask = m_MouseBuffer[((Col + SkipX) / 8) + ((Row + SkipY) * (m_MouseInfo.Pitch))] & Mask;
			BYTE XorMask = m_MouseBuffer[((Col + SkipX) / 8) + ((Row + SkipY + (m_MouseInfo.Height / 2)) * (m_MouseInfo.Pitch))] & Mask;
			UINT AndMask32 = (AndMask) ? 0xFFFFFFFF : 0xFF000000;
			UINT XorMask32 = (XorMask) ? 0x00FFFFFF : 0x00000000;
			auto a=InitBuffer32[(Row * width) + Col];
			auto b=DesktopScreen32[(Row * DesktopPitchInPixels) + Col];
			// Set new pixel
			//InitBuffer32[(Row * width) + Col] = (DesktopScreen32[(Row * DesktopPitchInPixels) + Col] & AndMask32) ^ XorMask32;
			InitBuffer32[(Row * width) + Col] =
				(DesktopScreen32[(DskRow * DesktopPitchInPixels) + DskCol] & AndMask32) ^ XorMask32;

			// Adjust mask
			if (Mask == 0x01)
			{
				Mask = 0x80;
			}
			else
			{
				Mask = Mask >> 1;
			}
		}
	}
}

void DXGIDevices::DrawMouseColor(D3D11_MAPPED_SUBRESOURCE mapedResource, unsigned char *ptrScreen)
{
	/*FILE *fp = fopen("ColorMouse", "wb");
	fwrite(m_MouseBuffer, 1, m_MouseBufferSize, fp);
	fclose(fp);*/
	//its argb
	//screen may bgra or rgba
	int pos = m_MousePosition.Position.y*mapedResource.RowPitch +
		m_MousePosition.Position.x * 4;
	int posMax = (m_MousePosition.Position.y + 1)*mapedResource.RowPitch;
	posMax = posMax < mapedResource.DepthPitch ? posMax : mapedResource.DepthPitch;
	int posCur = 0;
	for (int y = 0; y < m_MouseInfo.Height; y++)
	{
		for (int x = 0; x < m_MouseInfo.Width; x++)
		{
			if (pos+5*x<posMax)
			{
					auto chAlpha = m_MouseBuffer[posCur + 3];
					auto fAlpha = chAlpha / 255.0f;
					if (chAlpha!=0)
					{
#if 0
						ptrScreen[pos + 4 * x + 0] = m_MouseBuffer[posCur + 0];
						ptrScreen[pos + 4 * x + 1] = m_MouseBuffer[posCur + 1];
						ptrScreen[pos + 4 * x + 2] = m_MouseBuffer[posCur + 2];
#else
						ptrScreen[pos + 4 * x + 0] = m_MouseBuffer[posCur + 0] * fAlpha + (1.0 - fAlpha)*ptrScreen[pos + 4 * x + 0];
						ptrScreen[pos + 4 * x + 1] = m_MouseBuffer[posCur + 1] * fAlpha + (1.0 - fAlpha)*ptrScreen[pos + 4 * x + 1];
						ptrScreen[pos + 4 * x + 2] = m_MouseBuffer[posCur + 2] * fAlpha + (1.0 - fAlpha)*ptrScreen[pos + 4 * x + 2];
#endif
						
					}
					posCur += 4;
				
				
			}
		}
		pos += mapedResource.RowPitch;
		posMax += mapedResource.RowPitch;
		posMax = posMax < mapedResource.DepthPitch ? posMax : mapedResource.DepthPitch;
	}
}

void DXGIDevices::DrawMouseMaskedColor(D3D11_MAPPED_SUBRESOURCE mapedResource, unsigned char *ptrScreen)
{
	int pos = m_MousePosition.Position.y*mapedResource.RowPitch +
		m_MousePosition.Position.x * 4;
	int posMax = (m_MousePosition.Position.y + 1)*mapedResource.RowPitch;
	posMax = posMax < mapedResource.DepthPitch ? posMax : mapedResource.DepthPitch;
	int posCur = 0;
	for (int y = 0; y < m_MouseInfo.Height; y++)
	{
		for (int x = 0; x < m_MouseInfo.Width; x++)
		{
			if (pos + 5 * x<posMax)
			{

				auto chAlpha = m_MouseBuffer[posCur + 3];
				auto fAlpha = chAlpha / 255.0f;
				if (chAlpha == 0)
				{
					ptrScreen[pos + 4 * x + 0] = m_MouseBuffer[posCur + 0];
					ptrScreen[pos + 4 * x + 1] = m_MouseBuffer[posCur + 1];
					ptrScreen[pos + 4 * x + 2] = m_MouseBuffer[posCur + 2];
				}
				else
				{
					ptrScreen[pos + 4 * x + 0] ^= m_MouseBuffer[posCur + 0];
					ptrScreen[pos + 4 * x + 1] ^= m_MouseBuffer[posCur + 1];
					ptrScreen[pos + 4 * x + 2] ^= m_MouseBuffer[posCur + 2];
				}
				posCur += 4;


			}
		}
		pos += mapedResource.RowPitch;
		posMax += mapedResource.RowPitch;
		posMax = posMax < mapedResource.DepthPitch ? posMax : mapedResource.DepthPitch;
	}
}

void DXGIDevices::DrawMouse2(D3D11_MAPPED_SUBRESOURCE mapedResource, unsigned char * ptrScreen)
{
	CURSORINFO ci;
	ZeroMemory(&ci, sizeof(ci));
	ci.cbSize = sizeof(ci);
	BOOL bMouseCaptured = GetCursorInfo(&ci);
	if (bMouseCaptured)
	{
		if (ci.flags&CURSOR_SHOWING)
		{
			HICON hIcon = CopyIcon(ci.hCursor);
			if (hIcon)
			{
				ICONINFO ii;
				if (GetIconInfo(hIcon, &ii))
				{
					UINT width, height;
					LPBYTE lpData = GetCursorData(hIcon, ii, width, height);
					if (lpData && width && height)
					{
						int pos = ci.ptScreenPos.y*mapedResource.RowPitch +
							ci.ptScreenPos.x * 4;
						int posMax = (ci.ptScreenPos.y + 1)*mapedResource.RowPitch;
						posMax = posMax < mapedResource.DepthPitch ? posMax : mapedResource.DepthPitch;
						int posCursor = 0;
						for (int y = 0; y < height; y++)
						{
							for (int x = 0; x < width; x++)
							{
								if (pos+5*x<posMax)
								{
									//the cursor is bgra
									if ( DXGI_FORMAT_B8G8R8A8_UNORM== m_textureFormat)
									{
										ptrScreen[pos + 4 * x + 0] |= lpData[posCursor++];
										ptrScreen[pos + 4 * x + 1] |= lpData[posCursor++];
										ptrScreen[pos + 4 * x + 2] |= lpData[posCursor++];
										ptrScreen[pos + 4 * x + 3] |= lpData[posCursor++];
									}
									else
									{

										ptrScreen[pos + 4 * x + 3] |= lpData[posCursor++];
										ptrScreen[pos + 4 * x + 2] |= lpData[posCursor++];
										ptrScreen[pos + 4 * x + 1] |= lpData[posCursor++];
										ptrScreen[pos + 4 * x + 0] |= lpData[posCursor++];
									}
								}
							}
							pos += mapedResource.RowPitch;
							posMax += mapedResource.RowPitch;
							posMax = posMax < mapedResource.DepthPitch ? posMax : mapedResource.DepthPitch;
						}
						delete[]lpData;
					}
					DeleteObject(ii.hbmColor);
					DeleteObject(ii.hbmMask);
				}
				DestroyIcon(hIcon);
			}
		}
	}
}

LPBYTE GetCursorData(HICON hIcon, ICONINFO &ii, UINT &width, UINT &height)
{
	BITMAP bmpColor, bmpMask;
	LPBYTE lpBitmapData = NULL, lpMaskData = NULL;

	if (lpBitmapData = GetBitmapData(ii.hbmColor, bmpColor)) {
		if (bmpColor.bmBitsPixel < 32) {
			delete []lpBitmapData;
			return NULL;
		}

		if (lpMaskData = GetBitmapData(ii.hbmMask, bmpMask)) {
			int pixels = bmpColor.bmHeight*bmpColor.bmWidth;
			bool bHasAlpha = false;

			//god-awful horrible hack to detect 24bit cursor
			for (int i = 0; i<pixels; i++) {
				if (lpBitmapData[i * 4 + 3] != 0) {
					bHasAlpha = true;
					break;
				}
			}

			if (!bHasAlpha) {
				for (int i = 0; i<pixels; i++) {
					lpBitmapData[i * 4 + 3] = BitToAlpha(lpMaskData, i, false);
				}
			}

			delete[]lpMaskData;
		}

		width = bmpColor.bmWidth;
		height = bmpColor.bmHeight;
	}
	else if (lpMaskData = GetBitmapData(ii.hbmMask, bmpMask)) {
		bmpMask.bmHeight /= 2;

		int pixels = bmpMask.bmHeight*bmpMask.bmWidth;
		lpBitmapData = new BYTE[pixels * 4];
		memset(lpBitmapData, 0, pixels * 4);

		UINT bottom = bmpMask.bmWidthBytes*bmpMask.bmHeight;

		for (int i = 0; i<pixels; i++) {
			BYTE transparentVal = BitToAlpha(lpMaskData, i, false);
			BYTE colorVal = BitToAlpha(lpMaskData + bottom, i, true);

			if (!transparentVal)
				lpBitmapData[i * 4 + 3] = colorVal; //as an alternative to xoring, shows inverted as black
			else
				*(LPDWORD)(lpBitmapData + (i * 4)) = colorVal ? 0xFFFFFFFF : 0xFF000000;
		}

		delete[]lpMaskData;

		width = bmpMask.bmWidth;
		height = bmpMask.bmHeight;
	}

	return lpBitmapData;
}
