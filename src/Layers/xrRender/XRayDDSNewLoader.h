#pragma once
#include "DDSMain.h"

class XRayDDSNewLoader final 
{
	static size_t get_count_mips(size_t w, size_t h)
	{
		size_t max_size = _max(w, h);
		return static_cast<size_t>(log2(static_cast<double>(max_size)) + 1);
	}

	static void mask_shift_and_size(size_t mask, size_t* shift, size_t* size)
	{
		if(!mask)
		{
			*shift = 0;
			*size = 0;
			return;
		}

		*shift = 0;
		while((mask & 1) == 0)
		{
			++(*shift);
			mask >>= 1;
		}

		*size = 0;
		while((mask & 1) == 0)
		{
			++(*size);
			mask >>= 1;
		}
	}

	static u8 get_count_comp_dds(DDSTexturePixelFormat format)
	{
		switch (format)
		{
		case TPF_R8:
			return 1;
		case TPF_R8G8:
			return 2;
		case TPF_R8G8B8:
			return 3;
		case TPF_R8G8B8A8:
			return 4;
		case TPF_R32F:
			return 1;
		case TPF_R32G32F:
			return 2;
		case TPF_R32G32B32F:
			return 3;
		case TPF_R32G32B32A32F:
			return 4;
		default:
			NODEFAULT;
		}
	}
	static bool is_dds_compressor(DDSTexturePixelFormat format)
	{
		switch (format)
		{
		case TPF_R8:
		case TPF_R8G8:
		case TPF_R8G8B8:
		case TPF_R8G8B8A8:
		case TPF_R32F:
		case TPF_R32G32F:
		case TPF_R32G32B32F:
		case TPF_R32G32B32A32F:
			return false;
		case TPF_DXT_1:
		case TPF_DXT_1_alpha:
		case TPF_DXT_3:
		case TPF_DXT_5:
		case TPF_BC4:
		case TPF_BC5:
		case TPF_BC6:
		case TPF_BC7:
			return true;
		default:
			break;
		}
		return false;
	}
	static size_t get_dds_size_pixel(DDSTexturePixelFormat format)
	{
		switch (format)
		{
		case TPF_R8:
			return 1;
		case TPF_R8G8:
			return 2;
		case TPF_R8G8B8:
			return 3;
		case TPF_R8G8B8A8:
			return 4;
		case TPF_R32F:
			return 4;
		case TPF_R32G32F:
			return 8;
		case TPF_R32G32B32F:
			return 12;
		case TPF_R32G32B32A32F:
			return 16;
		default:
			NODEFAULT;
		}
	}
	static size_t get_dds_size_block(DDSTexturePixelFormat format)
	{
		switch (format)
		{
		case TPF_DXT_1:
			return 8;
		case TPF_DXT_1_alpha:
			return 8;
		case TPF_DXT_3:
			return 16;
		case TPF_DXT_5:
			return 16;
		case TPF_BC4:
			return 8;
		case TPF_BC5:
			return 16;
		case TPF_BC6:
			return 16;
		case TPF_BC7:
			return 16;
		default:
			NODEFAULT;
		}
	}
	static size_t get_dds_count_block(size_t w)
	{
		return (w + 3) / 4;
	}
	static size_t get_dds_mip(size_t w, size_t level)
	{
		const auto mip = static_cast<size_t>((w) / pow(2, static_cast<double>(level)));
		return _max(mip, static_cast<size_t>(1));
	}
	static size_t get_dds_size_in_memory(size_t w, size_t h, size_t mips, DDSTexturePixelFormat format)
	{
		size_t result = 0;
		if (!is_dds_compressor(format))
		{
			const size_t pixel_size = get_dds_size_pixel(format);
			//result += w * h * pixel_size;
			for (size_t i = 0; i < mips; i++)
			{
				const size_t mip_w = get_dds_mip(w, i);
				const size_t mip_h = get_dds_mip(h, i);
				result += mip_w * mip_h * pixel_size;
			}
		}
		else
		{
			const size_t size_block = get_dds_size_block(format);
			//	result += GetCountBlock(w)*GetCountBlock(h)*size_block;
			for (size_t i = 0; i < mips; i++)
			{
				const size_t mip_w = get_dds_mip(w, i);
				const size_t mip_h = get_dds_mip(h, i);
				result += get_dds_count_block(mip_w) * get_dds_count_block(mip_h) * size_block;
			}
		}
		return result;
	}
	static u8* get_dds_image(u8* data, size_t w, size_t h, size_t mips, size_t depth, size_t mip, DDSTexturePixelFormat format)
	{
		return data + get_dds_size_in_memory(w, h, mips, format) * depth + get_dds_size_in_memory(w, h, mip, format);
	}
	static u8* get_dds_pixel_uint8(size_t x, size_t y, size_t w, size_t comps, size_t comp_id, u8* data)
	{
		return  &data[(x + (y * w)) * comps + comp_id];
	}

	bool LoadDDS(IReader* F);
	void CreateStartDDS();
    [[nodiscard]] size_t get_dds_size_memory() const
	{
		return get_dds_size_in_memory(m_w, m_h, m_mips, m_pixel_texture) * m_depth;
	}
private:
	bool m_Cube;
	DDSTexturePixelFormat m_pixel_texture{};
	u8* m_dds_data;
	size_t m_h, m_w, m_mips, m_depth;

public:
	XRayDDSNewLoader();
	~XRayDDSNewLoader();
	void Clear();
	bool Load(const char* file_name);

	[[nodiscard]] bool isCube()const { return m_Cube; }
};