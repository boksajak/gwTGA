#include "gwTGA.h"
#include <cstring> // memcpy

namespace gw {          
	namespace tga {
		
		using namespace details;

		TGAImage LoadTgaFromFile(char* fileName) {

			TGAImage result;

			std::ifstream fileStream;
			fileStream.open(fileName, std::ifstream::in | std::ifstream::binary);

			if (fileStream.fail()) {
				result.error = GWTGA_CANNOT_OPEN_FILE; 
				return result;
			}

			TGALoaderListener listener;
			result = LoadTga(fileStream, &listener);

			fileStream.close();

			return result;
		}

		TGAImage LoadTga(std::istream &stream, ITGALoaderListener* listener) {
			// TODO: TGA is little endian. Make sure reading from stream is little endian

			TGAImage resultImage;

			// Read header
			TGAHeader header;

			stream.read((char*)&header.iDLength, sizeof(header.iDLength));
			stream.read((char*)&header.colorMapType, sizeof(header.colorMapType));
			stream.read((char*)&header.ImageType, sizeof(header.ImageType));
			stream.read((char*)&header.colorMapSpec.firstEntryIndex, sizeof(header.colorMapSpec.firstEntryIndex));
			stream.read((char*)&header.colorMapSpec.colorMapLength, sizeof(header.colorMapSpec.colorMapLength));
			stream.read((char*)&header.colorMapSpec.colorMapEntrySize, sizeof(header.colorMapSpec.colorMapEntrySize));
			stream.read((char*)&header.imageSpec.xOrigin, sizeof(header.imageSpec.xOrigin));
			stream.read((char*)&header.imageSpec.yOrigin, sizeof(header.imageSpec.yOrigin));
			stream.read((char*)&header.imageSpec.width, sizeof(header.imageSpec.width));
			stream.read((char*)&header.imageSpec.height, sizeof(header.imageSpec.height));
			stream.read((char*)&header.imageSpec.bitsPerPixel, sizeof(header.imageSpec.bitsPerPixel));
			stream.read((char*)&header.imageSpec.imgDescriptor, sizeof(header.imageSpec.imgDescriptor));

			if (stream.fail()) {
				// Reading of header failed
				resultImage.error = GWTGA_IO_ERROR;
				return resultImage;
			}

			resultImage.width = header.imageSpec.width;
			resultImage.height = header.imageSpec.height;

			if (header.colorMapSpec.colorMapLength == 0) {
				resultImage.bitsPerPixel = header.imageSpec.bitsPerPixel;
			} else {
				resultImage.bitsPerPixel = header.colorMapSpec.colorMapEntrySize;
			}

			resultImage.attributeBitsPerPixel = header.imageSpec.imgDescriptor & 0x0F;

			switch (header.imageSpec.imgDescriptor & 0x30) {
			case 0x00:
				resultImage.origin = GWTGA_BOTTOM_LEFT;
				break;
			case 0x10:
				resultImage.origin = GWTGA_BOTTOM_RIGHT;
				break;
			case 0x20:
				resultImage.origin = GWTGA_TOP_LEFT;
				break;
			case 0x30:
				resultImage.origin = GWTGA_TOP_RIGHT;
				break;
			}

			// TODO: We do not need pixelFormat anymore
			// Leave this as a check for supported formats
			//switch (resultImage.bitsPerPixel) {
			//case 8:
			//	resultImage.pixelFormat = TGAFormat::LUMINANCE_U8;
			//	break;
			//case 16:
			//	resultImage.pixelFormat = TGAFormat::BGRA5551_U16;
			//	break;
			//case 24:
			//	resultImage.pixelFormat = TGAFormat::BGR_U24;
			//	break;
			//case 32:
			//	resultImage.pixelFormat = TGAFormat::BGRA_U32;
			//	break;
			//case 96:
			//	resultImage.pixelFormat = TGAFormat::BGR_F96;
			//	break;
			//default:
			//	// TODO: Unknown pixel format
			//	break;
			//}

			if (resultImage.bitsPerPixel > 16 * 8) {
				// Too many bits per pixel
				resultImage.error = GWTGA_UNSUPPORTED_PIXEL_DEPTH;
				return resultImage;
			}

			switch (header.ImageType) {
			case 1:
			case 2:
			case 9:
			case 10:
				resultImage.colorType = GWTGA_RGB;
				break;
			case 3:
			case 11:
				resultImage.colorType = GWTGA_GREYSCALE;
				break;
			}

			// Read image iD - skip this, we do not use image id now
			stream.seekg(header.iDLength, std::ios_base::cur);

			// Read color map
			char* colorMap = NULL;
			if (header.colorMapSpec.colorMapLength > 0) {

				colorMap = (*listener)(header.colorMapSpec.colorMapEntrySize, header.colorMapSpec.colorMapLength, 1, GWTGA_COLOR_PALETTE);

				if (!colorMap) {
					// Could not allocate memory for color map
					resultImage.error = GWTGA_MALLOC_ERROR;
					return resultImage;
				}

				resultImage.colorPaletteBytes = colorMap;
				resultImage.colorMapLength = header.colorMapSpec.colorMapLength;
				resultImage.colorPaletteBPP = header.imageSpec.bitsPerPixel;

				size_t size = header.colorMapSpec.colorMapLength * (header.colorMapSpec.colorMapEntrySize / 8);
				stream.read(colorMap, size);

				if (stream.fail()) {
					// Could not read color map from stream
					resultImage.error = GWTGA_IO_ERROR;
					return resultImage;
				}
			}

			// Read image data
			size_t pixelsNumber = header.imageSpec.width * header.imageSpec.height;

			if ((resultImage.bitsPerPixel & 0x07) != 0) {
				// Bits per pixel has to be divisible by 8
				resultImage.error = GWTGA_UNSUPPORTED_PIXEL_DEPTH;
				return resultImage;
			}

			size_t bytesPerPixel = resultImage.bitsPerPixel / 8;
			size_t imgDataSize = pixelsNumber * bytesPerPixel;

			// Allocate memory for image data
			resultImage.bytes = (*listener)(resultImage.bitsPerPixel, header.imageSpec.width, header.imageSpec.height, GWTGA_IMAGE_DATA);

			if (!resultImage.bytes) {
				resultImage.error = GWTGA_MALLOC_ERROR;
				return resultImage;
			}

			// Read pixel data
			if (header.ImageType == 2 || header.ImageType == 3) {
				// 2 - Uncompressed, RGB images
				// 3 - Uncompressed, black and white images.

				stream.read(resultImage.bytes, imgDataSize);

				if (stream.fail()) {
					resultImage.error = GWTGA_IO_ERROR;
					return resultImage;
				}

			} else if (header.ImageType == 10 || header.ImageType == 11) {

				// 10 - Runlength encoded RGB images
				// 11 - Runlength encoded black and white images.
				decompressRLE<fetchPixelUncompressed, fetchPixelsUncompressed>(resultImage.bytes, pixelsNumber, bytesPerPixel, stream, NULL, bytesPerPixel);

			}  else if (header.ImageType == 1 || header.ImageType == 9 ) {
				// 1  -  Uncompressed, color-mapped images
				// 9  -  Runlength encoded color-mapped images

				if (header.imageSpec.bitsPerPixel != 8 && header.imageSpec.bitsPerPixel != 16 && header.imageSpec.bitsPerPixel != 24) {
					// Unsupported color map entry length
					resultImage.error = GWTGA_UNSUPPORTED_PIXEL_DEPTH;
					return resultImage;
				}

				if (header.colorMapType != 1 || colorMap == NULL) {
					// Color map not present in file
					resultImage.error = GWTGA_INVALID_DATA;
					return resultImage;
				}

				if (header.ImageType == 1) {

					// 1  -  Uncompressed, color-mapped images
					fetchPixelsColorMap(resultImage.bytes, stream, header.imageSpec.bitsPerPixel / 8, colorMap, bytesPerPixel, pixelsNumber);
				} else if (header.ImageType == 9) {

					// 9  -  Runlength encoded color-mapped images
					if (!decompressRLE<fetchPixelColorMap, fetchPixelsColorMap>(resultImage.bytes, pixelsNumber, header.imageSpec.bitsPerPixel / 8, stream, colorMap, bytesPerPixel)) {
						// Error while reading compressed image data
						resultImage.error = GWTGA_IO_ERROR;
						return resultImage;
					}
				}
			}

			// TODO: if we got memory from user, do not delete it here
			if (colorMap != NULL) delete[] colorMap;

			return resultImage;
		}

		namespace details {

			void fetchPixelUncompressed(void* target, void* input, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel) { 
				memcpy(target, input, bytesPerInputPixel);
			}

			void fetchPixelColorMap(void* target, void* input, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel) { 
				size_t colorIdx = 0;
				// Read color index (TODO: make sure this works on little/big endian machines)
				switch (bytesPerInputPixel) {
				case 1:
					colorIdx = *((uint8_t*) input);
					break;
				case 2:
					colorIdx = *((uint16_t*) input);
					break;
				case 3:
					colorIdx = *((uint32_t*) input) & 0x00FFFFFF;
					break;
				}
				memcpy(target, (void*) &colorMap[colorIdx * bytesPerOutputPixel], bytesPerOutputPixel);
			}

			bool fetchPixelsUncompressed(char* target, std::istream &stream, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel, size_t count) { 
				stream.read(target, bytesPerInputPixel * count);

				if (stream.fail()) {
					return false;
				}

				return true;
			}

			bool fetchPixelsColorMap(char* target, std::istream &stream, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel, size_t count) { 

				size_t colorIdx = 0;

				for (size_t i = 0; i < count * bytesPerOutputPixel; i += bytesPerOutputPixel) {

					// Read color index (TODO: make sure this works on little/big endian machines)
					stream.read((char*) &colorIdx, bytesPerInputPixel);

					if (stream.fail()) {
						return false;
					}

					memcpy((void*) &target[i], (void*) &colorMap[colorIdx * bytesPerOutputPixel], bytesPerOutputPixel);
				}

				return true;
			}

			template<fetchPixelFunc fetchPixel, fetchPixelsFunc fetchPixels>
			bool decompressRLE(char* target, size_t pixelsNumber, size_t bytesPerInputPixel, std::istream &stream, char* colorMap, size_t bytesPerOutputPixel) {

				// number of pixels read so far
				size_t readPixels = 0;

				// buffer for pixel (on stack)
				char colorValues[16]; // max 16 bytes per pixel are supported (4 floats)
				uint8_t packetHeader;
				uint8_t repetitionCount;

				// Read packets until all pixels have been read
				while (readPixels < pixelsNumber) {

					// Read packet type (RLE compressed or RAW data)
					stream.read((char*)&packetHeader, sizeof(packetHeader));

					if (stream.fail()) {
						return false;
					}

					repetitionCount = (packetHeader & 0x7F) + 1;

					if ((packetHeader & 0x80) == 0x80) {
						// RLE packet

						// Read repeated color value
						stream.read(colorValues, bytesPerInputPixel);

						if (stream.fail()) {
							return false;
						}

						// Emit repetitionCount times given color value
						for (int i = 0; i < repetitionCount; i++) {
							// TODO: Check if this is inlined
							fetchPixel((void*) &target[readPixels * bytesPerOutputPixel], (void*) colorValues, bytesPerInputPixel, colorMap, bytesPerOutputPixel);
							readPixels++;
						}

					} else {
						// RAW packet

						// Emit repetitionCount times upcoming color values
						// TODO: Check if this is inlined
						if (!fetchPixels(&target[readPixels * bytesPerOutputPixel], stream, bytesPerInputPixel, colorMap, bytesPerOutputPixel, repetitionCount)) {
							return false;
						}
						readPixels += repetitionCount;
					}
				}

				return true;
			}

		}
	} 
}
