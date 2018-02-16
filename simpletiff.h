#pragma once

#include <cstdint>
#include <memory>

enum class TYPE : uint16_t
{
	None = 0,
	BYTE, ASCII, SHORT, LONG, RATIONAL,
	SBYTE, UNDEF, SSHORT, SLONG, SRATIONAL,
	FLOAT, DOUBLE
};

enum class TAG : uint16_t
{
	None = 0,
	NewSubFileType = 254, SubFileType, ImageWidth, ImageLength, BitsPerSample, Compression,
	PhotometricInterpretation = 262,
	Make = 271, Model, StripOffsets, Orientation,
	SamplesPerPixel = 277, RowsPerStrip, StripByteCounts, MinSampleValue, MaxSampleValue, XResolution, YResolution, PlanarConfiguration
};

struct TiffTag
{
	TAG      tag   = TAG::None;
	TYPE     type  = TYPE::None;
	uint32_t count = 0;
	uint32_t data  = 0;
};

template <typename T>
void toTIFF(const char* filename, const T* data, uint32_t width, uint32_t height, uint32_t channels)
{
	toTIFF(filename, data, width, height, channels, sizeof(T));
}

void toTIFF(const char* filename, const void* _data, uint32_t width, uint32_t height, uint32_t channels, uint32_t elsize)
{
	const uint8_t* data = (uint8_t*)_data;
	FILE* fptr = fopen(filename, "wb");
	std::unique_ptr<FILE, int(*)(FILE*)> closer(fptr, &fclose);

	// write little endian header and directory offset
	uint32_t val = 0x002a4949;
	fwrite(&val, sizeof(val), 1, fptr);
	val = width * height * channels * elsize + 8;
	fwrite(&val, sizeof(val), 1, fptr);

	// write data
	fwrite(data, 1, val-8, fptr);

	// write IFD
	uint16_t numEntries = 8;
	fwrite(&numEntries, sizeof(numEntries), 1, fptr);

	TiffTag tag;
	
	tag = { TAG::ImageWidth, TYPE::LONG, 1, width };
	fwrite(&tag, sizeof(tag), 1, fptr);

	tag = { TAG::ImageLength, TYPE::LONG, 1, height };
	fwrite(&tag, sizeof(tag), 1, fptr);

	tag = { TAG::BitsPerSample, TYPE::SHORT, 1, elsize * 8 };
	fwrite(&tag, sizeof(tag), 1, fptr);

	tag = { TAG::SamplesPerPixel, TYPE::SHORT, 1, channels };
	fwrite(&tag, sizeof(tag), 1, fptr);

	tag = { TAG::PhotometricInterpretation, TYPE::SHORT, 1, channels == 3 ? 2u : 32803 };
	fwrite(&tag, sizeof(tag), 1, fptr);

	tag = { TAG::StripByteCounts, TYPE::LONG, 1, width*height*channels*elsize };
	fwrite(&tag, sizeof(tag), 1, fptr);

	tag = { TAG::RowsPerStrip, TYPE::LONG, 1, width };
	fwrite(&tag, sizeof(tag), 1, fptr);

	tag = { TAG::StripOffsets, TYPE::LONG, 1, 8 }; // data comes right after file header
	fwrite(&tag, sizeof(tag), 1, fptr);

	uint32_t offsetNextIFD = 0;
	fwrite(&offsetNextIFD, sizeof(offsetNextIFD), 1, fptr);
}