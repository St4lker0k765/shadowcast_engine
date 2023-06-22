#include "stdafx.h"
#include "XRayDDSNewLoader.h"

XRayDDSNewLoader::XRayDDSNewLoader()
{
	m_Cube = false;
	m_dds_data = nullptr;
	m_h = 0; m_w = 0; m_mips = 0; m_depth = 0;
}

XRayDDSNewLoader::~XRayDDSNewLoader()
{
	Clear();
}

void XRayDDSNewLoader::Clear()
{
	if (m_dds_data)
		xr_free(m_dds_data);
	m_dds_data = nullptr;
}

void XRayDDSNewLoader::CreateStartDDS()
{
	Clear();
	m_dds_data = xr_alloc<u8>(static_cast<u32>(get_dds_size_memory()));
}

bool XRayDDSNewLoader::Load(const char* file_name)
{
	auto* F = FS.r_open(file_name);
	bool result = LoadDDS(F);
	FS.r_close(F);
	return result;
}

static u32 dds_convert_color(u32 c, u32 inbits, u32 outbits)
{
	if (inbits == 0)
	{
		return 0;
	}
	else if (inbits >= outbits)
	{
		return c >> (inbits - outbits);
	}
	else
	{
		return (c << (outbits - inbits)) | dds_convert_color(c, inbits, outbits - inbits);
	}
}

bool XRayDDSNewLoader::LoadDDS(IReader* F)
{
	if (F->r_u32() == MAKEFOURCC('D', 'D', 'S', ' '))
	{
		const size_t size = F->r_u32();
		DDS_HEADER header{};
		FillMemory(&header, 0, sizeof(DDS_HEADER));
		F->seek(4);
		if (size > sizeof(header) || size < 4)
			return false;
		F->r(&header, static_cast<int>(size));
		if (!(header.dwHeaderFlags & dds_wight) || !(header.dwHeaderFlags & dds_height) || !(header.dwHeaderFlags & dds_pixel_format))
			return false;
		m_depth = header.dwHeaderFlags & dds_depth ? header.dwDepth : 1;
		m_mips = header.dwHeaderFlags & dds_mipmap_count ? header.dwMipMapCount : 1;
		m_h = header.dwHeight;
		m_w = header.dwWidth;
		m_Cube = !!(header.dwCubemapFlags & DDS_CUBEMAP_ALLFACES);
		if (!m_mips)
		{
			return false;
		}
		if (header.dds_pf.dwFlags == dds_four_cc)
		{
			const bool alpha = header.dds_pf.dwFlags & (dds_rgba & (~dds_rgb));
			if (header.dds_pf.dwRGBBitCount % 8 || header.dds_pf.dwRGBBitCount > 64)
			{
				return false;
			}
			const size_t byte_size_pixel = header.dds_pf.dwRGBBitCount / 8;
			size_t size_bit[4], shift_bit[4];
			for (size_t x = 0; x < 4; x++)
				mask_shift_and_size(header.dds_pf.dwBitsMask[x], shift_bit + x, size_bit + x);
			if (header.dds_pf.dwRBitMask && header.dds_pf.dwGBitMask && header.dds_pf.dwBBitMask)
				m_pixel_texture = alpha ? TPF_R8G8B8A8 : TPF_R8G8B8;
			else	if (header.dds_pf.dwRBitMask && header.dds_pf.dwGBitMask)
				m_pixel_texture = TPF_R8G8;
			else
				m_pixel_texture = TPF_R8;
			u8 coutComp = get_count_comp_dds(m_pixel_texture);
			u32 pixel = 0;
			CreateStartDDS();
			for (size_t d = 0; d < m_depth; d++)
			{
				for (size_t m = 0; m < m_mips; m++)
				{
					size_t h = get_dds_mip(m_h, m);
					size_t w = get_dds_mip(m_w, m);
					u8* data = get_dds_image(m_dds_data, m_w, m_h, m_mips, d, m, m_pixel_texture);
					for (size_t x = 0; x < w * h; x++)
					{
						F->r(&pixel, static_cast<int>(byte_size_pixel));
						for (size_t a = 0; a < coutComp; a++)
						{
							*get_dds_pixel_uint8(x, 0, 0, coutComp, a, data) = static_cast<u8>(dds_convert_color((pixel & header.dds_pf.dwBitsMask[a]) >> shift_bit[a], static_cast<u32>(size_bit[a]), 8));
						}
					}
				}
			}
			return true;
		}
		else
		{
			if (header.dds_pf.dwFourCC == MAKEFOURCC('D', 'X', '1', '0'))
			{
				switch (header.Header10.dxgiFormat)
				{
				case DXGI_FORMAT_R32G32B32A32_FLOAT:
					m_pixel_texture = TPF_R32G32B32A32F;
					break;
				case DXGI_FORMAT_R32G32B32_FLOAT:
					m_pixel_texture = TPF_R32G32B32F;
					break;
				case DXGI_FORMAT_R32G32_FLOAT:
					m_pixel_texture = TPF_R32G32F;
					break;
				case DXGI_FORMAT_R32_FLOAT:
					m_pixel_texture = TPF_R32F;
					break;
				case DXGI_FORMAT_R8G8B8A8_UNORM:
					m_pixel_texture = TPF_R8G8B8A8;
					break;
				case DXGI_FORMAT_R8G8_UNORM:
					m_pixel_texture = TPF_R8G8;
					break;
				case DXGI_FORMAT_R8_UNORM:
					m_pixel_texture = TPF_R8;
					break;
				case DXGI_FORMAT_BC1_UNORM:
					m_pixel_texture = TPF_BC1;
					break;
				case DXGI_FORMAT_BC2_UNORM:
					m_pixel_texture = TPF_BC2;
					break;
				case DXGI_FORMAT_BC3_UNORM:
					m_pixel_texture = TPF_BC3;
					break;
				case DXGI_FORMAT_BC4_UNORM:
					m_pixel_texture = TPF_BC4;
					break;
				case DXGI_FORMAT_BC5_UNORM:
					m_pixel_texture = TPF_BC5;
					break;
				case DXGI_FORMAT_BC6H_SF16:
					m_pixel_texture = TPF_BC6;
					break;
				case DXGI_FORMAT_BC7_UNORM:
					m_pixel_texture = TPF_BC7;
					break;
				default:
					Clear();
					return false;
				}
			}
			else
			{
				switch (header.dds_pf.dwFourCC)
				{
				case  MAKEFOURCC('D', 'X', 'T', '1'):
					m_pixel_texture = TPF_BC1;
					break;
				case  MAKEFOURCC('D', 'X', 'T', '2'):
				case  MAKEFOURCC('D', 'X', 'T', '3'):
					m_pixel_texture = TPF_BC2;
					break;
				case  MAKEFOURCC('D', 'X', 'T', '4'):
				case  MAKEFOURCC('D', 'X', 'T', '5'):
					m_pixel_texture = TPF_BC3;
					break;
				case  MAKEFOURCC('A', 'T', 'I', '1'):
					m_pixel_texture = TPF_BC4;
					break;
				case  MAKEFOURCC('A', 'T', 'I', '2'):
					m_pixel_texture = TPF_BC5;
					break;
				default:
					Clear();
					return false;
				}
			}
			CreateStartDDS();
			F->r(m_dds_data, static_cast<int>(get_dds_size_memory()));
			return true;
		}
	}
	return false;
}
