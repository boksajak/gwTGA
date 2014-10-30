#include "gwTGA.h"


namespace gw {          
	namespace tga {
		
		using namespace details;

		TGAImage LoadTgaFromFile(char* fileName) {

			TGAImage result;

			std::ifstream fileStream;
			fileStream.open(fileName, std::ifstream::in | std::ifstream::binary);

			if (fileStream.fail()) {
				// TODO Error
				return result;
			}

			// TODO: check file stream for errors
			TGALoaderListener listener;
			result = LoadTga(fileStream, &listener);

			fileStream.close();

			return result;
		}

		TGAImage LoadTga(std::istream &stream, ITGALoaderListener* listener) {
			// TODO: assert the stream is binary and readable

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
				resultImage.origin = TGAImageOrigin::BOTTOM_LEFT;
				break;
			case 0x10:
				resultImage.origin = TGAImageOrigin::BOTTOM_RIGHT;
				break;
			case 0x20:
				resultImage.origin = TGAImageOrigin::TOP_LEFT;
				break;
			case 0x30:
				resultImage.origin = TGAImageOrigin::TOP_RIGHT;
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
			
			// Read image iD - skip this, we do not use image id now
			stream.seekg(header.iDLength, std::ios_base::cur);

			// Read color map
			char* colorMap = NULL;
			if (header.colorMapSpec.colorMapLength > 0) {
				size_t size = header.colorMapSpec.colorMapLength * (header.colorMapSpec.colorMapEntrySize / 8);
				colorMap = new char[size];
				if (!colorMap) {
					// TODO: malloc error
					/*file.close();
					return TGALoaderError::MALLOC_ERROR;*/
					return resultImage;
				}
				stream.read(colorMap, size);
			}

			// Read image data
			size_t pixelsNumber = header.imageSpec.width * header.imageSpec.height;

			// TODO: Bits per pixel has to be divisible by 8, do some checking before!
			size_t bytesPerPixel = resultImage.bitsPerPixel / 8;
			size_t imgDataSize = pixelsNumber * bytesPerPixel;

			// Allocate memory for image data
			resultImage.bytes = listener->newTexture(resultImage.bitsPerPixel, header.imageSpec.width, header.imageSpec.height);

			if (!resultImage.bytes) {
				// TODO: malloc error
				return resultImage;
			}

			// Read pixel data
			if (header.ImageType == 2 || header.ImageType == 3) {
				// 2 - Uncompressed, RGB images
				// 3 - Uncompressed, black and white images.

				stream.read(resultImage.bytes, imgDataSize);

			} else if (header.ImageType == 10 || header.ImageType == 11) {

				// 10 - Runlength encoded RGB images
				// 11 - Runlength encoded black and white images.
				decompressRLE<fetchPixelUncompressed, fetchPixelsUncompressed>(resultImage.bytes, pixelsNumber, bytesPerPixel, stream, NULL, bytesPerPixel);

			}  else if (header.ImageType == 1 || header.ImageType == 9 ) {
				// 1  -  Uncompressed, color-mapped images
				// 9  -  Runlength encoded color-mapped images

				// Check color map entry length
				if (header.imageSpec.bitsPerPixel != 8 && header.imageSpec.bitsPerPixel != 16) {
					// TODO ERROR("Unsupported color map entry length");
					return resultImage;
				}

				// Check color map presence
				if (header.colorMapType != 1 || colorMap == NULL) {
					// TODO ERROR("Color map not present in file");
					return resultImage;
				}

				if (header.ImageType == 1) {

					// 1  -  Uncompressed, color-mapped images
					//decompressColorMap(resultImage.bytes, pixelsNumber, bytesPerPixel, header.colorMapSpec.colorMapEntrySize / 8, colorMap, stream);
					fetchPixelsColorMap(resultImage.bytes, stream, header.imageSpec.bitsPerPixel / 8, colorMap, bytesPerPixel, pixelsNumber);
				} else if (header.ImageType == 9) {

					// 9  -  Runlength encoded color-mapped images
					decompressRLE<fetchPixelColorMap, fetchPixelsColorMap>(resultImage.bytes, pixelsNumber, header.imageSpec.bitsPerPixel / 8, stream, colorMap, bytesPerPixel);
				}
			}

			if (colorMap != NULL) delete[] colorMap;

			return resultImage;
		}

		namespace details {

			void fetchPixelUncompressed(void* target, void* input, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel) { 
				memcpy(target, input, bytesPerInputPixel);
			}

			void fetchPixelColorMap(void* target, void* input, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel) { 
				//// read index in little endian
				size_t colorIdx = 0;
				// Read color index (TODO: make sure this works on little/big endian machines)
				//// TODO: assert that bytesPerColorMapEntry is always 1, 2 or 3
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

			void fetchPixelsUncompressed(char* target, std::istream &stream, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel, size_t count) { 
				stream.read(target, bytesPerInputPixel * count);
			}

			void fetchPixelsColorMap(char* target, std::istream &stream, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel, size_t count) { 

				size_t colorIdx = 0;

				for (size_t i = 0; i < count * bytesPerOutputPixel; i += bytesPerOutputPixel) {

					// Read color index (TODO: make sure this works on little/big endian machines)
					//// TODO: assert that bytesPerColorMapEntry is always 1, 2 or 3
					stream.read((char*) &colorIdx, bytesPerInputPixel);

					//// read index in little endian
					//switch (bytesPerColorMapEntry) {
					//case 2:
					//	colorIdx = ((colorIdx & 0xFF00) >> 8) | ((colorIdx & 0x00FF) << 8);
					//	break;
					//case 3:
					//	colorIdx = ((colorIdx & 0xFF0000) >> 16) | (colorIdx & 0x00FF00) | ((colorIdx & 0x0000FF) << 16);
					//	break;
					//}

					memcpy((void*) &target[i], (void*) &colorMap[colorIdx * bytesPerOutputPixel], bytesPerOutputPixel);
				}
			}

			template<fetchPixelFunc fetchPixel, fetchPixelsFunc fetchPixels>
			void decompressRLE(char* target, size_t pixelsNumber, size_t bytesPerInputPixel, std::istream &stream, char* colorMap, size_t bytesPerOutputPixel) {

				// number of pixels read so far
				size_t readPixels = 0;

				// buffer for pixel (on stack)
				char* colorValues = (char*) alloca(bytesPerInputPixel);
				uint8_t packetHeader;
				uint8_t repetitionCount;

				// Read packets until all pixels have been read
				while (readPixels < pixelsNumber) {

					// Read packet type (RLE compressed or RAW data)
					stream.read((char*)&packetHeader, sizeof(packetHeader));

					repetitionCount = (packetHeader & 0x7F) + 1;

					if ((packetHeader & 0x80) == 0x80) {
						// RLE packet

						// Read repeated color value
						stream.read(colorValues, bytesPerInputPixel);

						// Emit repetitionCount times given color value
						for (int i = 0; i < repetitionCount; i++) {
							// TODO: Check if this is inlined
							fetchPixel((void*) &target[readPixels * bytesPerOutputPixel], (void*) colorValues, bytesPerInputPixel, colorMap, bytesPerOutputPixel);
							//memcpy((void*) &target[readPixels * bytesPerPixel], (void*) colorValues, bytesPerPixel);
							readPixels++;
						}

					} else {
						// RAW packet

						// Emit repetitionCount times upcoming color values
						// TODO: Check if this is inlined
						fetchPixels(&target[readPixels * bytesPerOutputPixel], stream, bytesPerInputPixel, colorMap, bytesPerOutputPixel, repetitionCount);
						//stream.read(&target[readPixels * bytesPerPixel], bytesPerPixel * repetitionCount);
						readPixels += repetitionCount;
					}
				}
			}

		}
	} 
}
