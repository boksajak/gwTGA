#include "gwTGA.h"


namespace gw {          
	namespace tga {


		TGAImage LoadTgaFromFile(char* fileName) {

			std::ifstream fileStream;
			fileStream.open(fileName, std::ifstream::in | std::ifstream::binary);

			// TODO: check file stream for errors
			TGALoaderListener listener;
			TGAImage result = LoadTga(fileStream, &listener);

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

			// TODO: We do not need pixelFormat anymore
			// Leave this as a check for supported formats
			switch (resultImage.bitsPerPixel) {
			case 8:
				resultImage.pixelFormat = TGAFormat::LUMINANCE_U8;
				break;
			case 16:
				resultImage.pixelFormat = TGAFormat::BGRA5551_U16;
				break;
			case 24:
				resultImage.pixelFormat = TGAFormat::BGR_U24;
				break;
			case 32:
				resultImage.pixelFormat = TGAFormat::BGRA_U32;
				break;
			case 96:
				resultImage.pixelFormat = TGAFormat::BGR_F96;
				break;
			default:
				// TODO: Unknown pixel format
				break;
			}

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
			resultImage.bytes = listener->newTexture(header.imageSpec.bitsPerPixel, header.imageSpec.width, header.imageSpec.height);

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
				decompressRLE(resultImage.bytes, pixelsNumber, bytesPerPixel, stream);

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

					decompressColorMap(resultImage.bytes, pixelsNumber, bytesPerPixel, header.colorMapSpec.colorMapEntrySize / 8, colorMap, stream);

				} else if (header.ImageType == 9) {
					// 9  -  Runlength encoded color-mapped images
				}
			}

			if (colorMap != NULL) delete[] colorMap;

			return resultImage;
		}

		void decompressRLE(char* target, size_t pixelsNumber, size_t bytesPerPixel, std::istream &stream) {

			// number of pixels read so far
			uint32_t readPixels = 0;

			// buffer for pixel (on stack)
			char* colorValues = (char*) alloca(bytesPerPixel);
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
					stream.read(colorValues, bytesPerPixel);

					// Emit repetitionCount times given color value
					for (int i = 0; i < repetitionCount; i++) {
						memcpy((void*) &target[readPixels * bytesPerPixel], (void*) colorValues, bytesPerPixel);
						readPixels++;
					}

				} else {
					// RAW packet

					// Emit repetitionCount times upcoming color values
					stream.read(&target[readPixels * bytesPerPixel], bytesPerPixel * repetitionCount);
					readPixels += repetitionCount;
				}
			}
		}

		void decompressColorMap(char* target, size_t pixelsNumber, size_t bytesPerPixel, size_t bytesPerColorMapEntry, char* colorMap, std::istream &stream) {

			for (size_t i = 0; i < pixelsNumber * bytesPerPixel; i += bytesPerPixel) {
				uint16_t colorIdx;

				// Read either 8 or 16 bit color index (TODO: make sure this works on little/big endian machines)
				stream.read((char*) &colorIdx, bytesPerColorMapEntry);

				memcpy((void*) &target[i], (void*)  colorMap[colorIdx * bytesPerPixel], bytesPerPixel);

			}
		}

	} 
}
