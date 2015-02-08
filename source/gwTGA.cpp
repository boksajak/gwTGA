#include "gwTGA.h"
#include <cstring> // memcpy
#include <fstream>  

namespace gw {          
	namespace tga {

		using namespace details;

		TGAImage LoadTga(char* fileName) {
			return LoadTga(fileName, GWTGA_OPTIONS_NONE);
		}

		TGAImage LoadTga(char* fileName, TGAOptions options) {
			TGALoaderListener<> listener(options & GWTGA_RETURN_COLOR_MAP);
			return LoadTga(fileName, &listener, options);
		}

		TGAImage LoadTga(char* fileName, ITGALoaderListener* listener) {
			return LoadTga(fileName, listener, GWTGA_OPTIONS_NONE);
		}

		TGAImage LoadTga(char* fileName, ITGALoaderListener* listener, TGAOptions options) 
		{
			TGAImage result;

			std::ifstream fileStream;
			fileStream.open(fileName, std::ifstream::in | std::ifstream::binary);

			if (fileStream.fail()) {
				result.error = GWTGA_CANNOT_OPEN_FILE; 
				return result;
			}

			result = LoadTga(fileStream, listener, options);

			fileStream.close();

			return result;
		}

		TGAImage LoadTga(std::istream &stream) {
			return LoadTga(stream, GWTGA_OPTIONS_NONE);
		}

		TGAImage LoadTga(std::istream &stream, TGAOptions options) {
			TGALoaderListener<> listener(options & GWTGA_RETURN_COLOR_MAP);
			return LoadTga(stream, &listener, options);
		}

		TGAImage LoadTga(std::istream &stream, ITGALoaderListener* listener, TGAOptions options) {
			// TODO: TGA is little endian. Make sure reading from stream is little endian

			// Parse options
			bool flipVertically = ((options & GWTGA_FLIP_VERTICALLY) == GWTGA_FLIP_VERTICALLY);
			bool flipHorizontally = ((options & GWTGA_FLIP_HORIZONTALLY) == GWTGA_FLIP_HORIZONTALLY);
			bool returnColorMap = ((options & GWTGA_RETURN_COLOR_MAP) == GWTGA_RETURN_COLOR_MAP);

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
			resultImage.xOrigin = header.imageSpec.xOrigin;
			resultImage.yOrigin = header.imageSpec.yOrigin;

			if (header.colorMapSpec.colorMapLength == 0 || returnColorMap) {
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

				// Pick temporary memory (possibly from stack or preallocated) when we dont need color palette anymore after loading the image
				TGAMemoryType colorMapType = returnColorMap ? GWTGA_COLOR_PALETTE : GWTGA_COLOR_PALETTE_TEMPORARY;

				colorMap = (*listener)(header.colorMapSpec.colorMapEntrySize, header.colorMapSpec.colorMapLength, 1, colorMapType);

				if (!colorMap) {
					// Could not allocate memory for color map
					resultImage.error = GWTGA_MALLOC_ERROR;
					return resultImage;
				}

				if (returnColorMap) {
					resultImage.colorMap.bytes = colorMap;
					resultImage.colorMap.length = header.colorMapSpec.colorMapLength;
					resultImage.colorMap.bitsPerPixel = header.colorMapSpec.colorMapEntrySize;
				}

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
			if (header.ImageType == 2 || header.ImageType == 3 || 
				(header.ImageType == 1 && returnColorMap)) { // color mapped, but do not resolve palette is specified

					// 2 - Uncompressed, RGB images
					// 3 - Uncompressed, black and white images.

					if (!flipVertically && !flipHorizontally) {
						// NO PROCESSING
						stream.read(resultImage.bytes, imgDataSize);

					} else if (flipVertically) {
						if (!flipHorizontally) {
							// PROCESSING - Vertical Flip
							int stride = resultImage.width * bytesPerPixel;
							processToArray<processPassThrough, fetchXPlusY>(stream, resultImage.bytes, resultImage.width, resultImage.height, 0, 1, 1, (resultImage.height - 1) * stride, -stride, -stride, stride);

						} else {
							// PROCESSING - Vertical and horizontal Flip
							int strideX = bytesPerPixel;
							int strideY = resultImage.width * bytesPerPixel;
							processToArray<processPassThrough, fetchXPlusY>(stream, resultImage.bytes, resultImage.width, resultImage.height, (resultImage.height - 1) * strideY, -strideY, -strideY, (resultImage.width - 1) * strideX, -strideX, -strideX , strideX);
						}
					} else if (flipHorizontally) {
						// PROCESSING - Horizontal Flip
						int strideX = bytesPerPixel;
						int strideY = resultImage.width * bytesPerPixel;
						processToArray<processPassThrough, fetchXPlusY>(stream, resultImage.bytes, resultImage.width, resultImage.height, 0, strideY, (resultImage.height) * strideY, (resultImage.width - 1) * strideX, -strideX, -strideX , strideX);

					} else {
						// TODO
					}

					if (stream.fail()) {
						resultImage.error = GWTGA_IO_ERROR;
						return resultImage;
					}

			} else if (header.ImageType == 10 || header.ImageType == 11 ||
				(header.ImageType == 9 && returnColorMap)) {  // color mapped, RLE compressed, but do not resolve palette is specified

					// 10 - Runlength encoded RGB images
					// 11 - Runlength encoded black and white images.

					bool perPixelProcessing;
					size_t stride;
					flipFunc flipFuncType;

					getFlipFunction(flipFuncType, stride, flipVertically, flipHorizontally, resultImage.width, resultImage.height);

					if (stride == resultImage.width * resultImage.height) {
						perPixelProcessing = false;
					} else {
						perPixelProcessing = true;
					}

					if (!decompressRLE<fetchPixelUncompressed, fetchPixelsUncompressed>(resultImage.bytes, pixelsNumber, bytesPerPixel, stream, NULL, bytesPerPixel, resultImage.width, resultImage.height, flipFuncType, perPixelProcessing)) {
						// Error while reading compressed image data
						resultImage.error = GWTGA_IO_ERROR;
						return resultImage;
					}

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
					size_t stride;
					flipFunc flipFuncType;

					getFlipFunction(flipFuncType, stride, flipVertically, flipHorizontally, resultImage.width, resultImage.height);

					for (size_t i = 0; i < resultImage.width * resultImage.height; i += stride) {
						fetchPixelsColorMap(&resultImage.bytes[flipFuncType(i, resultImage.width, resultImage.height, bytesPerPixel)], stream, header.imageSpec.bitsPerPixel / 8, colorMap, bytesPerPixel, stride);
					}

				} else if (header.ImageType == 9) {

					bool perPixelProcessing;
					size_t stride;
					flipFunc flipFuncType;

					getFlipFunction(flipFuncType, stride, flipVertically, flipHorizontally, resultImage.width, resultImage.height);

					if (stride == resultImage.width * resultImage.height) {
						perPixelProcessing = false;
					} else {
						perPixelProcessing = true;
					}

					// 9  -  Runlength encoded color-mapped images
					if (!decompressRLE<fetchPixelColorMap, fetchPixelsColorMap>(resultImage.bytes, pixelsNumber, header.imageSpec.bitsPerPixel / 8, stream, colorMap, bytesPerPixel, resultImage.width, resultImage.height, flipFuncType, perPixelProcessing)) {
						// Error while reading compressed image data
						resultImage.error = GWTGA_IO_ERROR;
						return resultImage;
					}
				}
			}

			return resultImage;
		}

		TGAError SaveTga(char* fileName, const TGAImage &image) {
			return SaveTga(fileName, image, GWTGA_OPTIONS_NONE);
		}

		TGAError SaveTga(std::ostream &stream, const TGAImage &image) {
			return SaveTga(stream, image, GWTGA_OPTIONS_NONE);
		}

		TGAError SaveTga(char* fileName, const TGAImage &image, TGAOptions options) {

			std::ofstream fileStream;
			fileStream.open(fileName, std::ofstream::out | std::ofstream::binary);

			if (fileStream.fail()) {
				return GWTGA_CANNOT_OPEN_FILE; 
			}

			TGAError err = SaveTga(fileStream, image, options);

			fileStream.close();

			return err;
		}

		TGAError SaveTga(std::ostream &stream, const TGAImage &image, TGAOptions options) {

			if (image.hasError()) {
				// Input image is in error state
				return GWTGA_INVALID_DATA;
			}

			// Assert height and width and origin coords is 16-bit unsigned int
			if (image.width > 0xFFFF || image.height > 0xFFFF || image.xOrigin > 0xFFFF || image.yOrigin > 0xFFFF) {
				// Invalid image dimensions
				return GWTGA_INVALID_DATA;
			}

			// Assert height and width are non-null
			if (image.width == 0 || image.height == 0) {
				// Invalid image dimensions
				return GWTGA_INVALID_DATA;
			}

			if ((image.bitsPerPixel & 0x07) != 0) {
				// Bits per pixel has to be divisible by 8
				return GWTGA_UNSUPPORTED_PIXEL_DEPTH;
			}

			if (image.hasColorMap() && (image.colorMap.bitsPerPixel & 0x07) != 0) {
				// Bits per pixel has to be divisible by 8 for color map too
				return GWTGA_UNSUPPORTED_PIXEL_DEPTH;
			}

			// Parse options
			bool useRLEcompression = ((options & GWTGA_COMPRESS_RLE) == GWTGA_COMPRESS_RLE);
			bool flipVertically = ((options & GWTGA_FLIP_VERTICALLY) == GWTGA_FLIP_VERTICALLY);
			bool flipHorizontally = ((options & GWTGA_FLIP_HORIZONTALLY) == GWTGA_FLIP_HORIZONTALLY);

			// Write header
			TGAHeader header;
			header.iDLength = 0;
			header.colorMapType = image.hasColorMap() ? 1 : 0;

			switch (image.colorType) {
			case GWTGA_RGB:
				if (header.colorMapType == 1) {
					if (useRLEcompression) {
						header.ImageType = 9; //< Runlength encoded color-mapped image
					} else {
						header.ImageType = 1; //< Uncompressed color-mapped image
					}
				} else {
					if (useRLEcompression) {
						header.ImageType = 10; //< Runlength encoded RGB
					} else {
						header.ImageType = 2; //< Uncompressed RGB 
					}
				}
				break;
			case GWTGA_GREYSCALE:
				if (header.colorMapType == 1) {
					// TGA does not support greyscale color mapped images
					return GWTGA_INVALID_DATA;
				} else {
					if (useRLEcompression) {
						header.ImageType = 11; //< Runlength encoded black and white image
					} else {
						header.ImageType = 3; //< Uncompressed greyscale
					}
				}
				break;
			default:
				// Unknown color type provided
				return GWTGA_INVALID_DATA;
			}

			header.colorMapSpec.colorMapEntrySize = image.colorMap.bitsPerPixel;
			header.colorMapSpec.colorMapLength = image.colorMap.length;
			header.colorMapSpec.firstEntryIndex = 0;

			header.imageSpec.xOrigin = image.xOrigin;
			header.imageSpec.yOrigin = image.yOrigin;
			header.imageSpec.width = image.width;
			header.imageSpec.height = image.height;
			header.imageSpec.bitsPerPixel = image.bitsPerPixel;

			switch (image.origin) {
			case GWTGA_BOTTOM_LEFT:
				header.imageSpec.imgDescriptor = 0x00;
				break;
			case GWTGA_BOTTOM_RIGHT:
				header.imageSpec.imgDescriptor = 0x10;
				break;
			case GWTGA_TOP_LEFT:
				header.imageSpec.imgDescriptor = 0x20;
				break;
			case GWTGA_TOP_RIGHT:
				header.imageSpec.imgDescriptor = 0x30;
				break;
			default:
				// Unknown origin provided
				return GWTGA_INVALID_DATA;
			}

			// Store "attribute bites per pixel" to LSB
			header.imageSpec.imgDescriptor |= image.attributeBitsPerPixel & 0x0F;

			// Write TGA header
			stream.write((char*)&header.iDLength, sizeof(header.iDLength));
			stream.write((char*)&header.colorMapType, sizeof(header.colorMapType));
			stream.write((char*)&header.ImageType, sizeof(header.ImageType));
			stream.write((char*)&header.colorMapSpec.firstEntryIndex, sizeof(header.colorMapSpec.firstEntryIndex));
			stream.write((char*)&header.colorMapSpec.colorMapLength, sizeof(header.colorMapSpec.colorMapLength));
			stream.write((char*)&header.colorMapSpec.colorMapEntrySize, sizeof(header.colorMapSpec.colorMapEntrySize));
			stream.write((char*)&header.imageSpec.xOrigin, sizeof(header.imageSpec.xOrigin));
			stream.write((char*)&header.imageSpec.yOrigin, sizeof(header.imageSpec.yOrigin));
			stream.write((char*)&header.imageSpec.width, sizeof(header.imageSpec.width));
			stream.write((char*)&header.imageSpec.height, sizeof(header.imageSpec.height));
			stream.write((char*)&header.imageSpec.bitsPerPixel, sizeof(header.imageSpec.bitsPerPixel));
			stream.write((char*)&header.imageSpec.imgDescriptor, sizeof(header.imageSpec.imgDescriptor));

			// Write color map
			if (image.hasColorMap()) {
				stream.write(image.colorMap.bytes, image.colorMap.length * (image.colorMap.bitsPerPixel / 8));
			}

			unsigned char bytesPerPixel = image.bitsPerPixel / 8;

			// Write pixel data
			if (!flipVertically && !flipHorizontally) {

				if (useRLEcompression) {
					if (!compressRLE(stream, image.bytes, image.width, image.height, bytesPerPixel)) {
						return GWTGA_IO_ERROR;
					}
				} else {
					// NO PROCESSING
					stream.write(image.bytes, image.width * image.height * bytesPerPixel);
				}
			} else if (useRLEcompression) {
				size_t stride;
				flipFunc flipFuncType;

				getFlipFunction(flipFuncType, stride, flipVertically, flipHorizontally, image.width, image.height);

				if (!compressRLE(stream, image.bytes, image.width, image.height, bytesPerPixel, flipFuncType)) {
					return GWTGA_IO_ERROR;
				}
			} else if (flipVertically) {
				if (!flipHorizontally) {
					// PROCESSING - Vertical Flip
					int stride = image.width * bytesPerPixel;
					processToStream<processPassThrough, fetchXPlusY>(stream, image.bytes, image.width, image.height, 0, 1, 1, (image.height - 1) * stride, -stride, -stride, stride);

				} else {
					// PROCESSING - Vertical and horizontal Flip
					int strideX = bytesPerPixel;
					int strideY = image.width * bytesPerPixel;
					processToStream<processPassThrough, fetchXPlusY>(stream, image.bytes, image.width, image.height, (image.height - 1) * strideY, -strideY, -strideY, (image.width - 1) * strideX, -strideX, -strideX , strideX);

				}
			} else if (flipHorizontally) {
				// PROCESSING - Horizontal Flip
				int strideX = bytesPerPixel;
				int strideY = image.width * bytesPerPixel;
				processToStream<processPassThrough, fetchXPlusY>(stream, image.bytes, image.width, image.height, 0, strideY, (image.height) * strideY, (image.width - 1) * strideX, -strideX, -strideX , strideX);

			} else {
				// TODO
			}

			if (stream.fail()) {
				return GWTGA_IO_ERROR;
			}

			return GWTGA_NONE;
		}

		namespace details {

			template<size_t tempMemorySize>
			char* TGALoaderListener<tempMemorySize>::operator()(const unsigned int &bitsPerPixel, const unsigned int &width, const unsigned int &height, TGAMemoryType mType) {						

				if (mType == GWTGA_COLOR_PALETTE_TEMPORARY && width * height * (bitsPerPixel / 8) <= tempMemorySize) {
					return tempMemory;
				} else if (mType == GWTGA_IMAGE_DATA) {
					return new char[(bitsPerPixel / 8) * (height * width)];
				} else {
					colorMapMemory = new char[(bitsPerPixel / 8) * (height * width)];
					return colorMapMemory;
				}
			}

			template<size_t tempMemorySize>
			TGALoaderListener<tempMemorySize>::TGALoaderListener(bool persistentColorMapMemory) {
				this->persistentColorMapMemory = persistentColorMapMemory;
				this->colorMapMemory = NULL;
			}

			template<size_t tempMemorySize>
			TGALoaderListener<tempMemorySize>::~TGALoaderListener() {
				if (!persistentColorMapMemory && colorMapMemory != NULL) {
					delete[] colorMapMemory;
					colorMapMemory = NULL;
				}
			}

			void getFlipFunction(flipFunc &flipFuncType, size_t &stride, bool flipVertically, bool flipHorizontally, size_t width, size_t height) {

				if (!flipVertically && !flipHorizontally) {
					// NO PROCESSING
					flipFuncType = flipFuncPass;
					stride = width * height;

				} else if (flipVertically) {
					if (!flipHorizontally) {
						// PROCESSING - Vertical Flip
						flipFuncType = flipFuncVertical;
						stride = width;

					} else {
						// PROCESSING - Vertical and horizontal Flip
						flipFuncType = flipFuncHorizontalAndVertical;
						stride = 1;
					}
				} else if (flipHorizontally) {
					// PROCESSING - Horizontal Flip
					flipFuncType = flipFuncHorizontal;
					stride = 1;
				} 
			}

			void fetchPixelUncompressed(char* target, char* input, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel) { 
				memcpy(target, input, bytesPerInputPixel);
			}

			void fetchPixelColorMap(char* target, char* input, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel) { 
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

			size_t flipFuncPass(size_t i, size_t width, size_t height, size_t bpp) {
				return i * bpp;
			}

			size_t flipFuncHorizontal(size_t i, size_t width, size_t height, size_t bpp) {

				size_t rowNum = i / width;
				size_t colNum = i % width;

				return ((rowNum) * width  +(width - 1 - colNum)) * bpp;
			}

			size_t flipFuncVertical(size_t i, size_t width, size_t height, size_t bpp) {

				size_t rowNum = i / width;
				size_t colNum = i % width;

				return ((height - 1 - rowNum) * width  +colNum) * bpp;
			}

			size_t flipFuncHorizontalAndVertical(size_t i, size_t width, size_t height, size_t bpp) {
				return (( width * height - 1) - i) * bpp;
			}

			template<fetchPixelFunc fetchPixel, fetchPixelsFunc fetchPixels>
			bool decompressRLE(char* target, size_t pixelsNumber, size_t bytesPerInputPixel, std::istream &stream, char* colorMap, size_t bytesPerOutputPixel, size_t imgWidth, size_t imgHeight, flipFunc flip, bool perPixelProcessing) {

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
							fetchPixel(&target[flip(readPixels, imgWidth, imgHeight, bytesPerOutputPixel)], colorValues, bytesPerInputPixel, colorMap, bytesPerOutputPixel);
							readPixels++;
						}

					} else {
						// RAW packet

						if (perPixelProcessing) {
							// Emit repetitionCount times upcoming color value
							for (int i = 0; i < repetitionCount; i++) {
								stream.read(colorValues, bytesPerInputPixel);
								fetchPixel(&target[flip(readPixels, imgWidth, imgHeight, bytesPerOutputPixel)], colorValues, bytesPerInputPixel, colorMap, bytesPerOutputPixel);
								readPixels++;
							}
						} else {
							// Emit repetitionCount times upcoming color values
							// TODO: Check if this is inlined
							if (!fetchPixels(&target[flip(readPixels, imgWidth, imgHeight, bytesPerOutputPixel)], stream, bytesPerInputPixel, colorMap, bytesPerOutputPixel, repetitionCount)) {
								return false;
							}
							readPixels += repetitionCount;
						}
					}
				}

				return true;
			}

			char* fetchXPlusY(char* source, unsigned int x, unsigned int y, unsigned int imgWidth, unsigned int imgHeight) {
				return source + x + y; 
			}

			char* processPassThrough(char* target, char* source) {
				return source;
			}

			template<processFunc process, fetchFunc fetch>
			void processToStream(std::ostream &stream, char* source, unsigned int imgWidth, unsigned int imgHeight, int beginX, int strideX, int endX, int beginY, int strideY, int endY, size_t resultSize) {

				if (beginX == endX || beginY == endY) return;

				char buffer[4]; //< TODO assert it is more than max result size;

				int x = beginX;
				do {
					// outer loop
					int y = beginY;
					do {
						// inner loop
						stream.write(process(buffer, fetch(source, x, y, imgWidth, imgHeight)), resultSize);

						y+= strideY;
					} while (y != endY);

					x += strideX;
				} while (x != endX);
			}

			template<processFunc process, fetchFunc fetch>
			void processToArray(std::istream &stream, char* target, unsigned int imgWidth, unsigned int imgHeight, int beginX, int strideX, int endX, int beginY, int strideY, int endY, size_t resultSize) {

				if (beginX == endX || beginY == endY) return;

				char buffer[4]; //< TODO assert it is more than max result size;

				int x = beginX;
				do {
					// outer loop
					int y = beginY;
					do {

						// inner loop
						stream.read(process(buffer, fetch(target, x, y, imgWidth, imgHeight)), resultSize);
						y+= strideY;
					} while (y != endY);

					x += strideX;
				} while (x != endX);
			}


			bool cmpPixels(char* pa, char* pb, char bytesPerPixel) {

				uint32_t colorA;
				uint32_t colorB;

				// Read color index (TODO: make sure this works on little/big endian machines)
				switch (bytesPerPixel) {
				case 1:
					colorA = *((uint8_t*) pa);
					colorB = *((uint8_t*) pb);
					break;
				case 2:
					colorA = *((uint16_t*) pa);
					colorB = *((uint16_t*) pb);
					break;
				case 3:
					// TODO: Segfault - accesses behind array boundary
					colorA = *pa + (*(pa+1) << 8) + (*(pa+2) << 16);
					colorB = *pb + (*(pb+1) << 8) + (*(pb+2) << 16);
					break;
				default:
					// TODO: Error
					break;
				}

				return colorA == colorB;
			}

			bool compressRLE(std::ostream &stream, char* source, size_t imgWidth, size_t imgHeight, size_t bytesPerInputPixel, flipFunc flipFuncType) {

				size_t current = 0;
				size_t nextDifferent = 0;
				size_t pixelCount = imgWidth * imgHeight;
				char packetHeader = 0;

				// find longest sequence of same values
				while (nextDifferent < pixelCount) {

					size_t repetitionCount = 0;

					while (cmpPixels(&source[flipFuncType(current, imgWidth, imgHeight, bytesPerInputPixel)], 
						&source[flipFuncType(nextDifferent, imgWidth, imgHeight, bytesPerInputPixel)], bytesPerInputPixel)) { 

							nextDifferent++;
							repetitionCount++;

							if (repetitionCount == 128 || nextDifferent == pixelCount) break; // TODO: Do we need to split packets on row end?
					}

					// if at least 2 subsequent values are equal, emit RLE packet
					if (repetitionCount > 1) {

						packetHeader = 0x80 + (repetitionCount - 1); // & 0x7F - cannot be more than 127

						stream.write(&packetHeader, sizeof(packetHeader));

						// emit repeated value
						stream.write(&source[flipFuncType(current, imgWidth, imgHeight, bytesPerInputPixel)], sizeof(char) * bytesPerInputPixel);

					} else {
						// otherwise emit RAW packet

						// find longest sequence of non-repeating values
						while (true) {
							if (repetitionCount == 128 || nextDifferent + 1 >= pixelCount) break; // TODO: Do we need to split packets on row end?
							if (cmpPixels(&source[flipFuncType(nextDifferent, imgWidth, imgHeight, bytesPerInputPixel)], 
								&source[flipFuncType(nextDifferent + 1, imgWidth, imgHeight, bytesPerInputPixel)], bytesPerInputPixel)) break; 

							nextDifferent++;
							repetitionCount++;
						}

						packetHeader = (repetitionCount - 1); // & 0x7F - cannot be more than 127

						stream.write(&packetHeader, sizeof(packetHeader));

						// emit RAW values
						for (size_t i = 0; i < repetitionCount; i++) {
							stream.write(&source[flipFuncType(current + i, imgWidth, imgHeight, bytesPerInputPixel)], sizeof(char) * bytesPerInputPixel);
						}

					}

					current = nextDifferent;
				}
				// TODO: If image is 1 pixel long, encode this one pixel somehow

				if (stream.fail()) {
					return false;
				}
				return true;
			}

			// Faster version without flipping
			bool compressRLE(std::ostream &stream, char* source, size_t imgWidth, size_t imgHeight, size_t bytesPerInputPixel) {

				size_t current = 0;
				size_t nextDifferent = 0;
				size_t pixelCount = imgWidth * imgHeight * bytesPerInputPixel;
				char packetHeader = 0;

				// find longest sequence of same values
				while (nextDifferent < pixelCount) {

					size_t repetitionCount = 0;

					while (cmpPixels(&source[current], &source[nextDifferent], bytesPerInputPixel)) { 

						nextDifferent+=bytesPerInputPixel;
						repetitionCount++;

						if (repetitionCount == 128 || nextDifferent == pixelCount) break; // TODO: Do we need to split packets on row end?
					}

					// if at least 2 subsequent values are equal, emit RLE packet
					if (repetitionCount > 1) {

						packetHeader = 0x80 + (repetitionCount - 1); // & 0x7F - cannot be more than 127

						stream.write(&packetHeader, sizeof(packetHeader));

						// emit repeated value
						stream.write(&source[current], sizeof(char) * bytesPerInputPixel);

					} else {
						// otherwise emit RAW packet

						// find longest sequence of non-repeating values
						while (true) {
							if (repetitionCount == 128 || nextDifferent+bytesPerInputPixel >= pixelCount) break; // TODO: Do we need to split packets on row end?
							if (cmpPixels(&source[nextDifferent], &source[nextDifferent + bytesPerInputPixel], bytesPerInputPixel)) break; 

							nextDifferent+=bytesPerInputPixel;
							repetitionCount++;
						}

						packetHeader = (repetitionCount - 1); // & 0x7F - cannot be more than 127

						stream.write(&packetHeader, sizeof(packetHeader));

						// emit RAW values
						stream.write(&source[current], sizeof(char) * repetitionCount * bytesPerInputPixel);
					}

					current = nextDifferent;
				}
				// TODO: If image is 1 pixel long, encode this one pixel somehow

				if (stream.fail()) {
					return false;
				}
				return true;
			}
		}
	} 
}
