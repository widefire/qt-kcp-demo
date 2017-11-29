#include "DXGIDevices.h"
#include <stdio.h>



DXGIDevices::DXGIDevices()
{
	if (0 == InitializeDx()) {
		InitDuplication();
	}
}


DXGIDevices::~DXGIDevices()
{
	ReleaseDx();
}

HRESULT DXGIDevices::GetFrame()
{
	HRESULT hr = S_OK;
	do
	{
		IDXGIResource* DesktopResource = nullptr;
		DXGI_OUTDUPL_FRAME_INFO FrameInfo;
		hr =  m_DesktopDuplication->AcquireNextFrame(500, &FrameInfo, &DesktopResource);
		if (FAILED(hr))
		{
			break;
		}

		if ( m_AcquiredDesktopImage)
		{
			 m_AcquiredDesktopImage->Release();
			 m_AcquiredDesktopImage = nullptr;
		}
		hr = DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(& m_AcquiredDesktopImage));
		DesktopResource->Release();
		DesktopResource = nullptr;
		if (FAILED(hr))
		{
			break;
		}

		 m_DeviceContext->CopyResource( m_DesktopImg,  m_AcquiredDesktopImage);
		 //use bit or to draw mouse
		 if (FrameInfo.PointerPosition.Visible)
		 {
			 m_MouseBufferSize = FrameInfo.PointerShapeBufferSize;
			 hr = GetMouseBuffer();
			 if (SUCCEEDED(hr))
			 {
				 DrawMouse();
			 }
			 else
			 {
				 //mouse error
			 }
		 }
		 
		 //!use bit or to draw mouse

		D3D11_MAPPED_SUBRESOURCE mapedResource;
		hr =  m_DeviceContext->Map( m_DesktopImg, 0, D3D11_MAP::D3D11_MAP_READ, 0, &mapedResource);
		if (FAILED(hr))
		{
			break;
		}
		 m_DeviceContext->Unmap( m_DesktopImg, 0);

	} while (0);
	return hr;
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

		D3D11_TEXTURE2D_DESC textureDesc;
		textureDesc.Width = 1920;
		textureDesc.Height = 1080;
		textureDesc.MipLevels = 0;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_STAGING;
		textureDesc.BindFlags = 0;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		textureDesc.MiscFlags = 0;

		hr=m_Device->CreateTexture2D(&textureDesc, nullptr, &m_DesktopImg);
		if (FAILED(hr))
		{
			break;
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

	if (m_MouseBufferSize>m_MouseCacheSize)
	{
		if (m_MouseBuffer!=nullptr)
		{
			delete[]m_MouseBuffer;
			m_MouseBuffer = nullptr;
		}
		m_MouseCacheSize = m_MouseBufferSize;
		m_MouseBuffer = new unsigned char[m_MouseBufferSize];
	}

	UINT BufferSizeRequired;
	hr=m_DesktopDuplication->GetFramePointerShape(m_MouseBufferSize,
		m_MouseBuffer,
		&BufferSizeRequired,
		&m_MouseInfo);
	if (FAILED(hr))
	{
		return hr;
	}

	return hr;
}

void DXGIDevices::DrawMouse()
{
	switch (m_MouseInfo.Type)
	{
	case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME:
		/*The pointer type is a monochrome mouse pointer,
		which is a monochrome bitmap. 
		The bitmap's size is specified by width and height in a 1 bits per pixel (bpp) 
		device independent bitmap (DIB) format AND mask that is followed by 
		another 1 bpp DIB format XOR mask of the same size*/
		//1bit 1 pixel
		break;
	case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR:
		/*The pointer type is a color mouse pointer, which is a color bitmap.
		The bitmap's size is specified by width and height in a 32 bpp ARGB DIB format.*/
		//ARGB
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
		break;
	default:
		break;
	}
	FILE *fp = fopen("mouse.bin","wb");
	fwrite(m_MouseBuffer, 1, m_MouseBufferSize, fp);
	fclose(fp);
}
