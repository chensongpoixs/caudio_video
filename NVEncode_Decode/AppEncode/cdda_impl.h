/***********************************************************************************************
created: 		2022-12-16

author:			chensong

purpose:		ddaimpl
************************************************************************************************/

#ifndef _C_DDA_IMPL_H
#define _C_DDA_IMPL_H
#include <stdbool.h>
#include <stdint.h>
#include <dxgi1_2.h>
#include <d3d11_2.h>


#ifdef __cplusplus
extern "C" {
#else
#if defined(_MSC_VER) && !defined(inline)
#define inline __inline
#endif
#endif

namespace chen {

	class cdda_impl
	{
		public:
			explicit cdda_impl(ID3D11Device *pDev, ID3D11DeviceContext* pDevCtx);
			~cdda_impl();
	public:
		bool init();

		bool update(ID3D11Texture2D **ppTex2D, int wait);
		void destroy();


	public:
		uint32_t width() const { return m_width; }
		uint32_t height() const { return m_height; }
	private:
		/// The DDA object
		IDXGIOutputDuplication* m_dup_ptr; // = nullptr;
		/// The D3D11 device used by the DDA session
		ID3D11Device*          m_d3d_ptr; // pD3DDev = nullptr;
		/// The D3D11 Device Context used by the DDA session
		ID3D11DeviceContext*    m_ctx_ptr; // = nullptr;
		/// The resource used to acquire a new captured frame from DDA
		IDXGIResource *   m_resource_ptr; // = nullptr;
		/// Output width obtained from DXGI_OUTDUPL_DESC
		uint32_t			  m_width = 0;
		/// Output height obtained from DXGI_OUTDUPL_DESC
		uint32_t			  m_height = 0;
		/// Running count of no. of accumulated desktop updates
		int				  m_frameno = 0;
		/// output file stream to dump timestamps
		//ofstream ofs;
		/// DXGI_OUTDUPL_FRAME_INFO::latPresentTime from the last Acquired frame
		LARGE_INTEGER		m_lastPTS = { 0 };
		/// Clock frequency from QueryPerformaceFrequency()
		LARGE_INTEGER		m_qpcFreq = { 0 };

	};


}
#ifdef __cplusplus
}
#endif

#endif // _C_DDA_IMPL_H