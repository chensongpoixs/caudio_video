/***********************************************************************************************
created: 		2022-12-16

author:			chensong

purpose:		dxgi _duplication
************************************************************************************************/
#include "cdxgi_output_duplication.h"
#define   _CRT_SECURE_NO_WARNINGS

#include <d3d11.h>
#include <stdlib.h>
#include <iostream>
#include "cnv_encode.h"
//d3d11.lib
//dxgi.lib
#pragma comment  (lib,"d3d11.lib")
#pragma comment  (lib,"dxgi.lib")


namespace chen {
	cdxgi_output_duplication::cdxgi_output_duplication()
		: m_d3d_ptr(NULL)
		, m_ctx_ptr(NULL)
		, m_dda_impl_ptr(NULL)
	{
	}
	cdxgi_output_duplication::~cdxgi_output_duplication()
	{
	}
	bool cdxgi_output_duplication::init()
	{
		_init_dxgi();
		m_dda_impl_ptr = new cdda_impl(m_d3d_ptr, m_ctx_ptr);
		m_dda_impl_ptr->init();
		return true;
	}
	bool cdxgi_output_duplication::Capture(int wait)
	{
		bool hr = m_dda_impl_ptr->update(&m_pDupTex2D, wait); // Release after preproc
		if (!hr)
		{
			m_failCount++;
		}
		if (hr && m_pDupTex2D)
		{
			shared_texture();
			Map();


		}
		return hr;
	}
	void cdxgi_output_duplication::Map()
	{
		if (!m_new_tex2D)
		{
			return;
		}
		m_ctx_ptr->CopyResource(m_new_tex2D, m_pDupTex2D);

		D3D11_MAPPED_SUBRESOURCE map;
		UINT subResource = 0;
		HRESULT hr = m_ctx_ptr->Map(m_new_tex2D, 0, D3D11_MAP_READ, 0, &map);
		static FILE *out_file_ptr = fopen("./test.yuv", "wb+");
		if (SUCCEEDED(hr))
		{
			fwrite(map.pData, 1, 1920 * 1080 * 4, out_file_ptr);
			fflush(out_file_ptr);
			m_ctx_ptr->Unmap(m_new_tex2D, 0);
		}
		

		
		 
	}
	void cdxgi_output_duplication::shared_texture()
	{
		D3D11_TEXTURE2D_DESC desc;
		m_pDupTex2D->GetDesc(&desc);
		printf("desc Format = %d\n", static_cast<int>(desc.Format));
		if (!m_new_tex2D)
		{
			//desc.CPUAccessFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED; // D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX; // D3D11_RESOURCE_MISC_SHARED;
			m_d3d_ptr->CreateTexture2D(&desc, NULL, &m_new_tex2D);

			//m_new_tex2D->SetEvictionPriority(DXGI_RESOURCE_PRIORITY_MAXIMUM);
			IDXGIResource *dxgi_res;
			HRESULT hr = m_new_tex2D->QueryInterface(__uuidof(IDXGIResource), (void **)&dxgi_res);
			if (FAILED(hr)) {
				WARNING_EX_LOG("create_d3d11_tex: failed to query IDXGIResource interface from texture", hr);
				return  ;
			}
			hr = dxgi_res->GetSharedHandle(&m_SharedHandle_ptr);
			 IDXGIKeyedMutex * km;
		/*	hr = m_new_tex2D->QueryInterface( __uuidof(IDXGIKeyedMutex), (void **)&km);
			if (FAILED(hr)) {
				WARNING_EX_LOG("Failed to query "
					"IDXGIKeyedMutex",
					hr);
			}

			km->AcquireSync(0, INFINITE);*/
			//acquired = true;
				dxgi_res->Release();
				if (FAILED(hr))
				{
					WARNING_EX_LOG("create_d3d11_tex: failed to get shared handle",
						hr);
					return  ;
				}
		}
		else
		{
			// width != width 
		}


	}
	bool cdxgi_output_duplication::_init_dxgi()
	{
		HRESULT hr = S_OK;
		/// Driver types supported
		D3D_DRIVER_TYPE DriverTypes[] =
		{
			D3D_DRIVER_TYPE_HARDWARE,
			D3D_DRIVER_TYPE_WARP,
			D3D_DRIVER_TYPE_REFERENCE,
		};
		UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

		/// Feature levels supported
		D3D_FEATURE_LEVEL FeatureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_1
		};
		UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);
		D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_0;

		/// Create device
		for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
		{
			hr = ::D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, /*D3D11_CREATE_DEVICE_DEBUG*/0, FeatureLevels, NumFeatureLevels,
				D3D11_SDK_VERSION, &m_d3d_ptr, &FeatureLevel, &m_ctx_ptr);
			if (SUCCEEDED(hr))
			{
				return true;
				// Device creation succeeded, no need to loop anymore
				//break;
			}
		}
 
		return false;
	}
}