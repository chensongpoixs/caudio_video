/***********************************************************************************************
created: 		2022-12-16

author:			chensong

purpose:		dxgi _duplication 
************************************************************************************************/

#ifndef _C_DXGI_DUPLICATION_H
#define _C_DXGI_DUPLICATION_H
#include <stdbool.h>
#include <stdint.h>
#include <dxgi1_2.h>
#include <d3d11_2.h>
#include "cdda_impl.h"

#ifdef __cplusplus
extern "C" {
#else
#if defined(_MSC_VER) && !defined(inline)
#define inline __inline
#endif
#endif

namespace chen {

	class cdxgi_output_duplication
	{
	public:
		cdxgi_output_duplication();
		~cdxgi_output_duplication();

	public:
		bool init();
		/// Capture a frame using DDA
		bool Capture(int wait);
		



		void Map();

		HANDLE get_shared_handle() const {
			return  m_SharedHandle_ptr;
		};

		void shared_texture();
	protected:
	private:


		bool _init_dxgi();
		
	private:

		/// D3D11 device context used for the operations demonstrated in this application
		ID3D11Device *m_d3d_ptr = nullptr;
		/// D3D11 device context
		ID3D11DeviceContext *m_ctx_ptr = nullptr;
		cdda_impl *				m_dda_impl_ptr;


		ID3D11Texture2D *  m_pDupTex2D = nullptr;

		ID3D11Texture2D *  m_new_tex2D = nullptr;
		HANDLE   m_SharedHandle_ptr;
		/// Failure count from Capture API
		UINT				m_failCount = 0;
	};


}
#ifdef __cplusplus
}
#endif

#endif // _C_DXGI_DUPLICATION_H