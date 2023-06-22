#pragma once
#include <Windows.h>

constexpr auto dds_four_cc = 0x00000004;
constexpr auto dds_rgb = 0x00000040;
constexpr auto dds_rgba = 0x00000041;
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

struct dds_format_pixel {
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwFourCC;
	DWORD dwRGBBitCount;
	union {
		struct
		{
			DWORD dwRBitMask;
			DWORD dwGBitMask;
			DWORD dwBBitMask;
			DWORD dwABitMask;
		};
		DWORD dwBitsMask[4];
	};
};

const dds_format_pixel dds_pf_dxt1 =
{ sizeof(dds_format_pixel), dds_four_cc, MAKEFOURCC('D','X','T','1'), 0, {0, 0, 0, 0 } };
const dds_format_pixel dds_pf_dxt2 =
{ sizeof(dds_format_pixel), dds_four_cc, MAKEFOURCC('D','X','T','2'), 0, {0, 0, 0, 0}};
const dds_format_pixel dds_pf_dxt3 =
{ sizeof(dds_format_pixel), dds_four_cc, MAKEFOURCC('D','X','T','3'), 0, {0, 0, 0, 0 } };
const dds_format_pixel dds_pf_dxt4 =
{ sizeof(dds_format_pixel), dds_four_cc, MAKEFOURCC('D','X','T','4'), 0, {0, 0, 0, 0 } };
const dds_format_pixel dds_pf_dxt5 =
{ sizeof(dds_format_pixel), dds_four_cc, MAKEFOURCC('D','X','T','5'), 0, {0, 0, 0, 0 } };
const dds_format_pixel dds_pf_dxt6 =
{ sizeof(dds_format_pixel), dds_four_cc, MAKEFOURCC('D', 'X', '1', '0'), 0, {0, 0, 0, 0 } };
const dds_format_pixel dds_pf_ati1 =
{ sizeof(dds_format_pixel), dds_four_cc, MAKEFOURCC('A', 'T', 'I', '1'), 0, {0, 0, 0, 0 } };
const dds_format_pixel dds_pf_ati2 =
{ sizeof(dds_format_pixel), dds_four_cc, MAKEFOURCC('A', 'T', 'I', '2'), 0, {0, 0, 0, 0 } };
const dds_format_pixel dds_pf_a8_r8_g8_b8 =
{ sizeof(dds_format_pixel), dds_rgba, 0, 32, {0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 } };
const dds_format_pixel dds_pf_a1_r5_g5_b5 =
{ sizeof(dds_format_pixel), dds_rgba, 0, 16, {0x00007c00, 0x000003e0, 0x0000001f, 0x00008000 } };
const dds_format_pixel dds_pf_a4_r4_g4_b4 =
{ sizeof(dds_format_pixel), dds_rgba, 0, 16, {0x0000f000, 0x000000f0, 0x0000000f, 0x0000f000 } };
const dds_format_pixel dds_pf_r8_g8_b8 =
{ sizeof(dds_format_pixel), dds_rgb, 0, 24, {0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 } };
const dds_format_pixel dds_pf_r5_g6_b5 =
{ sizeof(dds_format_pixel), dds_rgb, 0, 16, {0x0000f800, 0x000007e0, 0x0000001f, 0x00000000 } };
const dds_format_pixel dds_pf_dx10 =
{ sizeof(dds_format_pixel), dds_four_cc, MAKEFOURCC('D','X','1','0'), 0, {0, 0, 0, 0 } };

enum
{
	DDS_HEADER_FLAGS_TEXTURE = 0x00001007,
	// DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT 
	DDS_HEADER_FLAGS_MIPMAP = 0x00020000,
	// DDSD_MIPMAPCOUNT
	DDS_HEADER_FLAGS_VOLUME = 0x00800000,
	// DDSD_DEPTH
	DDS_HEADER_FLAGS_PITCH = 0x00000008,
	// DDSD_PITCH
	DDS_HEADER_FLAGS_LINEARSIZE = 0x00080000,
	// DDSD_LINEARSIZE
	DDS_SURFACE_FLAGS_TEXTURE = 0x00001000,
	// DDSCAPS_TEXTURE
	DDS_SURFACE_FLAGS_MIPMAP = 0x00400008,
	// DDSCAPS_COMPLEX | DDSCAPS_MIPMAP
	DDS_SURFACE_FLAGS_CUBEMAP = 0x00000008,
	// DDSCAPS_COMPLEX
	DDS_CUBEMAP_POSITIVEX = 0x00000600,
	// DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
	DDS_CUBEMAP_NEGATIVEX = 0x00000a00,
	// DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
	DDS_CUBEMAP_POSITIVEY = 0x00001200,
	// DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
	DDS_CUBEMAP_NEGATIVEY = 0x00002200,
	// DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
	DDS_CUBEMAP_POSITIVEZ = 0x00004200,
	// DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
	DDS_CUBEMAP_NEGATIVEZ = 0x00008200, // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

	DDS_CUBEMAP_ALLFACES = (DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |
		DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |
		DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ),

	DDS_FLAGS_VOLUME = 0x00200000
};
struct DDSHeader10
{
	DWORD dxgiFormat;
	DWORD resourceDimension;
	DWORD miscFlag;
	DWORD arraySize;
	DWORD reserved;
};
struct DDS_HEADER
{
	DWORD dwSize;
	DWORD dwHeaderFlags;
	DWORD dwHeight;
	DWORD dwWidth;
	DWORD dwPitchOrLinearSize;
	DWORD dwDepth; // only if DDS_HEADER_FLAGS_VOLUME is set in dwHeaderFlags
	DWORD dwMipMapCount;
	DWORD dwReserved1[11];
	dds_format_pixel dds_pf;
	DWORD dwSurfaceFlags;
	DWORD dwCubemapFlags;
	DWORD dwReserved2[3];
	DDSHeader10 Header10;
};
enum DDS_HEADER_FLAGS
{
	dds_caps = 1 << 0,
	dds_height = 1 << 1,
	dds_wight = 1 << 2,
	dds_pitch = 1 << 3,
	dds_pixel_format = 1 << 12,
	dds_mipmap_count = 1 << 17,
	dds_linear_size = 1 << 19,
	dds_depth = 1 << 23,
};

//DDS Format:
enum DDSTexturePixelFormat
{
	TPF_R8 = 0,
	TPF_R8G8,
	TPF_R8G8B8,
	TPF_R8G8B8A8,
	TPF_R32F,
	TPF_R32G32F,
	TPF_R32G32B32F,
	TPF_R32G32B32A32F,
	TPF_DXT_1,
	TPF_DXT_1_alpha,
	TPF_DXT_3,
	TPF_DXT_5,
	TPF_BC1 = TPF_DXT_1,
	TPF_BC1a = TPF_DXT_1_alpha,
	TPF_BC2 = TPF_DXT_3,
	TPF_BC3 = TPF_DXT_5,
	TPF_BC4,
	TPF_BC5,
	TPF_BC6,
	TPF_BC7
};