/***********************************************************************************************
created: 		2022-12-16

author:			chensong

purpose:		ddaimpl
************************************************************************************************/
#include "cdda_impl.h"
#include <iomanip>
#include "clog.h"
#include <winerror.h>

#pragma warning(disable:4996)
#pragma warning(disable:4838)

#if !defined(SAFE_RELEASE)
#define SAFE_RELEASE(X) if(X){X->Release(); X=nullptr;}
#endif

namespace chen {
	cdda_impl::cdda_impl(ID3D11Device * pDev, ID3D11DeviceContext * pDevCtx)
		: m_dup_ptr(NULL)
		, m_d3d_ptr(pDev)
		, m_ctx_ptr(pDevCtx)
		, m_resource_ptr(NULL)
		, m_width(0)
	{
		m_d3d_ptr->AddRef();
		m_ctx_ptr->AddRef();

		::QueryPerformanceFrequency(&m_qpcFreq);
	}
	cdda_impl::~cdda_impl()
	{
	}
	bool cdda_impl::init()
	{
		IDXGIOutput * pOutput = nullptr;
		IDXGIDevice2* pDevice = nullptr;
		IDXGIFactory1* pFactory = nullptr;
		IDXGIAdapter *pAdapter = nullptr;
		IDXGIOutput1* pOut1 = nullptr;

		/// Release all temporary refs before exit
#define CLEAN_RETURN(x) \
    SAFE_RELEASE(pDevice);\
    SAFE_RELEASE(pFactory);\
    SAFE_RELEASE(pOutput);\
    SAFE_RELEASE(pOut1);\
    SAFE_RELEASE(pAdapter);   
  //  return x;

		HRESULT hr = S_OK;
		/// To create a DDA object given a D3D11 device, we must first get to the DXGI Adapter associated with that device
		if (FAILED(hr = m_d3d_ptr /*pD3DDev*/->QueryInterface(__uuidof(IDXGIDevice2), (void**)&pDevice)))
		{
			CLEAN_RETURN(hr);
			return false;
		}

		if (FAILED(hr = pDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&pAdapter)))
		{
			CLEAN_RETURN(hr);
			return false;
		}

		DXGI_ADAPTER_DESC dxgi_adapter_desc;
		pAdapter->GetDesc(&dxgi_adapter_desc);
		DEBUG_EX_LOG("adapter_desc name = %s", dxgi_adapter_desc.Description);

		/// Once we have the DXGI Adapter, we enumerate the attached display outputs, and select which one we want to capture
		/// This sample application always captures the primary display output, enumerated at index 0.
		if (FAILED(hr = pAdapter->EnumOutputs(0, &pOutput)))
		{
			CLEAN_RETURN(hr);
			return false;
		}

		 
		if (FAILED(hr = pOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&pOut1)))
		{
			CLEAN_RETURN(hr);
			return false;
		}
		/// Ask DXGI to create an instance of IDXGIOutputDuplication for the selected output. We can now capture this display output
		if (FAILED(hr = pOut1->DuplicateOutput(pDevice, &m_dup_ptr)))
		{
			CLEAN_RETURN(hr);
			return false;
		}
		DXGI_OUTPUT_DESC dxgi_desc;
		pOut1->GetDesc(&dxgi_desc);
		DEBUG_EX_LOG("IDXGIOutput1.device_name = %s", dxgi_desc.DeviceName);

		DXGI_OUTDUPL_DESC outDesc;
		ZeroMemory(&outDesc, sizeof(outDesc));
		m_dup_ptr->GetDesc(&outDesc);


		DEBUG_EX_LOG("[width = %u][height = %u]", outDesc.ModeDesc.Width, outDesc.ModeDesc.Height);

		m_height = outDesc.ModeDesc.Height;
		m_width = outDesc.ModeDesc.Width;
		CLEAN_RETURN(hr);
		//return false;
		return true;
	}
	bool cdda_impl::update(ID3D11Texture2D ** ppTex2D, int wait)
	{
		HRESULT hr = S_OK;
		DXGI_OUTDUPL_FRAME_INFO frameInfo;
		ZeroMemory(&frameInfo, sizeof(frameInfo));
		int acquired = 0;


#define RETURN_ERR(x) {DEBUG_EX_LOG(__FUNCTION__": %d : Line %d return 0x%x\n", m_frameno, __LINE__, x);return x;}

		if (m_resource_ptr)
		{
			m_dup_ptr->ReleaseFrame();
			m_resource_ptr->Release();
			m_resource_ptr = nullptr;
		}

		hr = m_dup_ptr->AcquireNextFrame(wait, &frameInfo, &m_resource_ptr);
		if (FAILED(hr))
		{
			if (hr == DXGI_ERROR_WAIT_TIMEOUT)
			{
				WARNING_EX_LOG(__FUNCTION__": %d : Wait for %d ms timed out\n", m_frameno, wait);
			}
			if (hr == DXGI_ERROR_INVALID_CALL)
			{
				WARNING_EX_LOG(__FUNCTION__": %d : Invalid Call, previous frame not released?\n", m_frameno);
			}
			if (hr == DXGI_ERROR_ACCESS_LOST)
			{
				WARNING_EX_LOG(__FUNCTION__": %d : Access lost, frame needs to be released?\n", m_frameno);
			}
			return false;
		}
		if (frameInfo.AccumulatedFrames == 0 || frameInfo.LastPresentTime.QuadPart == 0)
		{
			// No image update, only cursor moved.
		//	ofs << "frameNo: " << frameno << " | Accumulated: " << frameInfo.AccumulatedFrames << "MouseOnly?" << frameInfo.LastMouseUpdateTime.QuadPart << endl;
			//RETURN_ERR(DXGI_ERROR_WAIT_TIMEOUT);
			WARNING_EX_LOG("frameNo: %d | Accumulated: %u| MouseOnly? = %llu", m_frameno, frameInfo.AccumulatedFrames,  frameInfo.LastMouseUpdateTime.QuadPart);
			return false;
		}

		if (!m_resource_ptr)
		{
			WARNING_EX_LOG(__FUNCTION__": %d : Null output resource. Return error.\n", m_frameno);
			return  false;
		}

		if (FAILED(hr = m_resource_ptr->QueryInterface(__uuidof(ID3D11Texture2D), (void**)ppTex2D)))
		{
			WARNING_EX_LOG("m_resource_ptr->QueryInterface");
			return false ;
		}
#define MICROSEC_TIME(x,f)\
    x.QuadPart *= 1000000;\
    x.QuadPart /= f.QuadPart;

		LARGE_INTEGER pts = frameInfo.LastPresentTime;  MICROSEC_TIME(pts, m_qpcFreq);
		LONGLONG interval = pts.QuadPart - m_lastPTS.QuadPart;

		DEBUG_EX_LOG(__FUNCTION__": %d : Accumulated Frames %u PTS Interval %lld PTS %lld\n", m_frameno, frameInfo.AccumulatedFrames, interval * 1000, frameInfo.LastPresentTime.QuadPart);
		//ofs << "frameNo: " << frameno << " | Accumulated: " << frameInfo.AccumulatedFrames << " | PTS: " << frameInfo.LastPresentTime.QuadPart << " | PTSInterval: " << (interval) * 1000 << endl;
		m_lastPTS = pts; // store microsec value
		m_frameno += frameInfo.AccumulatedFrames;
		return true;
	}
	void cdda_impl::destroy()
	{
		if (m_resource_ptr)
		{
			m_dup_ptr->ReleaseFrame();
			SAFE_RELEASE(m_resource_ptr);
		}

		m_width = m_height = m_frameno = 0;

		SAFE_RELEASE(m_dup_ptr);
		SAFE_RELEASE(m_ctx_ptr);
		SAFE_RELEASE(m_d3d_ptr);

	}
}