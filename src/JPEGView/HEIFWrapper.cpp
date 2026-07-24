#include "stdafx.h"

#include "HEIFWrapper.h"
#include "MaxImageDef.h"
#include "ICCProfileTransform.h"
#include "libheif/heif_decoding.h"

void * HeifReader::ReadImage(int &width,
					   int &height,
					   int &nchannels,
					   int &frame_count,
					   void* &exif_chunk,
					   bool &outOfMemory,
					   int frame_index,
					   const void *buffer,
					   int sizebytes)
{
	outOfMemory = false;
	width = height = 0;
	nchannels = 4;

	unsigned char* pPixelData = NULL;
	exif_chunk = NULL;

	heif::Context context;
	context.read_from_memory_without_copy(buffer, sizebytes);
	frame_count = context.get_number_of_top_level_images();
	heif_item_id item_id = context.get_list_of_top_level_image_IDs().at(frame_index);
	heif::ImageHandle handle = context.get_image_handle(item_id);

	// Use C API directly with decoding options for speed improvements
	heif_decoding_options* decode_opts = heif_decoding_options_alloc();
	if (decode_opts) {
		decode_opts->ignore_transformations = 1;
		decode_opts->convert_hdr_to_8bit = 1;
	}

	heif_image* c_image = NULL;
	heif_error err = heif_decode_image(handle.get_raw_image_handle(), &c_image,
		heif_colorspace_RGB, heif_chroma_interleaved_RGBA, decode_opts);
	heif_decoding_options_free(decode_opts);

	if (err.code != heif_error_Ok) {
		return NULL;
	}

	int stride;
	uint8_t* data = (uint8_t*)heif_image_get_plane_readonly(c_image, heif_channel_interleaved, &stride);
	width = heif_image_get_width(c_image, heif_channel_interleaved);
	height = heif_image_get_height(c_image, heif_channel_interleaved);

	if (width > MAX_IMAGE_DIMENSION || height > MAX_IMAGE_DIMENSION) {
		heif_image_release(c_image);
		return NULL;
	}
	if (abs((double)width * height) > MAX_IMAGE_PIXELS) {
		outOfMemory = true;
		heif_image_release(c_image);
		return NULL;
	}
	if (width < 1 || height < 1 || width * nchannels > stride) {
		heif_image_release(c_image);
		return NULL;
	}

	int size = width * nchannels * height;
	pPixelData = new(std::nothrow) unsigned char[size];
	if (pPixelData == NULL) {
		outOfMemory = true;
		heif_image_release(c_image);
		return NULL;
	}

	size_t profile_size = heif_image_handle_get_raw_color_profile_size(handle.get_raw_image_handle());
	std::vector<uint8_t> iccp;
	if (profile_size > 0) {
		iccp.resize(profile_size);
		heif_image_handle_get_raw_color_profile(handle.get_raw_image_handle(), iccp.data());
	}
	void* transform = ICCProfileTransform::CreateTransform(iccp.data(), (int)iccp.size(), ICCProfileTransform::FORMAT_RGBA);
	size_t i, j;
	if (!ICCProfileTransform::DoTransform(transform, data, pPixelData, width, height, stride)) {
		unsigned int* o = (unsigned int*)pPixelData;
		for (i = 0; i < height; i++) {
			unsigned int* p = (unsigned int*)(data + i * stride);
			for (j = 0; j < width; j++) {
				// RGBA -> BGRA conversion
				*o = _rotr(_byteswap_ulong(*p), 8);
				p++;
				o++;
			}
		}
	}
	ICCProfileTransform::DeleteTransform(transform);

	heif_image_release(c_image);

	std::vector<heif_item_id> exif_blocks = handle.get_list_of_metadata_block_IDs("Exif");

	if (!exif_blocks.empty()) {
		std::vector<uint8_t> exif = handle.get_metadata(exif_blocks[0]);
		// 65538 magic number comes from investigations by qbnu
		// see https://github.com/sylikc/jpegview/pull/213#pullrequestreview-1494451359 for more details
		/*
		* These libraries all have their own ideas about where to start the Exif data from.
		* JPEG Exif blocks are in the format
		  FF E1 SS SS 45 78 69 66 00 00 [data]

		  The SS SS is a big-endian unsigned short representing the size of everything after FF E1.

		  libjxl gives 00 00 00 00 [data]
		  libheif gives 00 00 00 00 45 78 69 66 00 00 [data]
		  libavif, libwebp and libpng give [data], so they have different limits for size.

		  If you want I can change it to 65536 + an offset and add notes explaining why.
		*/
		if (exif.size() > 8 && exif.size() < 65538) {
			exif_chunk = malloc(exif.size());
			if (exif_chunk != NULL) {
				memcpy(exif_chunk, exif.data(), exif.size());
				*((unsigned short*)exif_chunk) = _byteswap_ushort(0xFFE1);
				*((unsigned short*)exif_chunk + 1) = _byteswap_ushort(exif.size() - 2);
			}
		}
	}

	return (void*)pPixelData;
}
