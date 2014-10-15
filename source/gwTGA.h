//----------------------------------------------------------------------------------------
/**
* \file       gwTGA.h
* \author     Jakub Boksansky (jakub.boksansky@gmail.com)
* \date       2013/30/12
* \brief      TGA Image Reader/Writer
*
*  Utility functions for reading and writing TARGA image files
*
*/
//----------------------------------------------------------------------------------------
#pragma once

#include <stdio.h>
#include <iostream>
#include <fstream>  
#include <stdint.h>

namespace gw {          
	namespace tga {

		struct TGAColorMapSpec {
			uint16_t firstEntryIndex;
			uint16_t colorMapLength;
			uint8_t colorMapEntrySize;
		};

		struct TGAImageSpec {
			uint16_t xOrigin;
			uint16_t yOrigin;
			uint16_t width;
			uint16_t height;
			uint8_t bitsPerPixel;
			uint8_t imgDescriptor;
		};

		struct TGAHeader {
			uint8_t iDLength;
			uint8_t colorMapType;
			uint8_t ImageType;
			TGAColorMapSpec colorMapSpec;
			TGAImageSpec imageSpec;
		};

		struct TGAExtensionArea {
			uint16_t extensionSize; // always 495
			char authorName[41];
			char authorComment[324];
			char dateTimeStamp[12];
			char jobId[41];
			char jobTime[6];
			char softwareId[41];
			char softwareVersion[3];
			uint32_t keyColor;
			uint32_t pixelAspectRatio;
			uint32_t gammaValue;
			uint32_t colorCorrectionOffset;
			uint32_t postageStampOffset;
			uint32_t scanLineOffset;
			uint8_t attributesType;
		};

		struct TGAFooter {
			uint16_t extensionOffset; 
			uint16_t devAreaOffset; 
			char signature[16];
			uint8_t dot;
			uint8_t null;
		};

		// Pixel format of TGA image, names correspond to OpenGL pixel format
		enum TGAFormat {
			LUMINANCE_U8, //< 1 byte greyscale value per pixel
			BGRA5551_U16, //< 5-bit BGR + 1-byte alpha
			BGR_U24,      //< 1 byte per color channel (BGR)
			BGRA_U32,     //< 1 byte per color channel (BGRA)
			BGR_F96       //< 1 32-bit float per color channel (BGR)
		};

		struct TGAImage {
			// TODO: constructor and destructor - destroy allocated bytes if any

			char* bytes;
			unsigned int width;
			unsigned int height;
			TGAFormat pixelFormat;
			unsigned char bitsPerPixel;
		};

		class ITGALoaderListener { 
		public: 
			virtual char* newTexture(const TGAImage &image) = 0; 
		}; 

		class TGALoaderListener : public ITGALoaderListener {
		public: 
			char* newTexture(const TGAImage &image) {
				this->image = image;
				return new char[(image.bitsPerPixel / 8) * (image.height * image.width)];
			}

			TGAImage image;
		};

		TGAImage LoadTgaFromFile(char* fileName);
		TGAImage LoadTga(std::istream &stream);

		void LoadTgaFromFile(char* fileName, ITGALoaderListener* listener);
		void LoadTga(std::istream &stream, ITGALoaderListener* listener);

		TGAImage LoadTgaFromFile(char* fileName) {

			std::ifstream fileStream;
			fileStream.open(fileName, std::ifstream::in | std::ifstream::binary);

			// TODO: check file stream for errors
			TGALoaderListener listener;
			LoadTga(fileStream, &listener);

			fileStream.close();

			return listener.image;
		}

		void LoadTga(std::istream &stream, ITGALoaderListener* listener) {
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
					return;
				}
				stream.read(colorMap, size);
			}

			// Read image data
			size_t pixelsNumber = header.imageSpec.width * header.imageSpec.height;
			// TODO: Bits per pixel has to be divisible by 8, do some checking before!
			size_t bytesPerPixel = resultImage.bitsPerPixel / 8;
			size_t imgDataSize = pixelsNumber * bytesPerPixel;

			// Allocate memory for image data
			char* imgData = listener->newTexture(resultImage);
			if (!imgData) {
					// TODO: malloc error
				return;
			}

			// Read pixels
			if (header.ImageType == 2 || header.ImageType == 3) {
				// 2 - Uncompressed, RGB images
				// 3 - Uncompressed, black and white images.

				stream.read(imgData, imgDataSize);
			}
		}
	} 
}