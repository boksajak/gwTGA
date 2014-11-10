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

		enum TGAImageOrigin {
			GWTGA_UNDEFINED = 0,
			GWTGA_BOTTOM_LEFT,
			GWTGA_BOTTOM_RIGHT,
			GWTGA_TOP_LEFT,
			GWTGA_TOP_RIGHT
		};

		enum TGAError {
			GWTGA_NONE = 0,
			GWTGA_CANNOT_OPEN_FILE,
			GWTGA_IO_ERROR,
			GWTGA_MALLOC_ERROR,
			GWTGA_INVALID_DATA,
			GWTGA_UNSUPPORTED_PIXEL_DEPTH
		};

		enum TGAColorType {
			GWTGA_UNKNOWN = 0,
			GWTGA_GREYSCALE,
			GWTGA_RGB
		};

		enum TGAMemoryType {
			GWTGA_IMAGE_DATA,
			GWTGA_COLOR_PALETTE
		};

		class ITGALoaderListener { 
		public: 
			virtual char* operator()(const unsigned int &bitsPerPixel, const unsigned int &width, const unsigned int &height, TGAMemoryType mType) = 0; 

		}; 

		struct TGAImage {

			TGAImage() :bytes(NULL), width(0), height(0), bitsPerPixel(0), attributeBitsPerPixel(0), origin(GWTGA_UNDEFINED), error(GWTGA_NONE), colorType(GWTGA_UNKNOWN), colorPaletteBytes(NULL), colorPaletteBPP(0), colorMapLength(0) { }

			char*			bytes;
			unsigned int	width;
			unsigned int	height;
			unsigned char	bitsPerPixel;
			unsigned char	attributeBitsPerPixel;
			TGAImageOrigin	origin;
			TGAError		error;
			TGAColorType	colorType;
			char*			colorPaletteBytes;
			unsigned char	colorPaletteBPP; //< TODO: change value with bitsPerPixel when colorMap is present???
			unsigned int	colorMapLength;

			//TGAFormat pixelFormat;
		};

		TGAImage LoadTgaFromFile(char* fileName);
		TGAImage LoadTga(std::istream &stream);

		TGAImage LoadTgaFromFile(char* fileName, ITGALoaderListener* listener);
		TGAImage LoadTga(std::istream &stream, ITGALoaderListener* listener);

		namespace details {
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
			//enum TGAFormat {
			//	LUMINANCE_U8, //< 1 byte greyscale value per pixel
			//	BGRA5551_U16, //< 5-bit BGR + 1-byte alpha
			//	BGR_U24,      //< 1 byte per color channel (BGR)
			//	BGRA_U32,     //< 1 byte per color channel (BGRA)
			//	BGR_F96       //< 1 32-bit float per color channel (BGR)
			//};

			class TGALoaderListener : public ITGALoaderListener {
			public: 
				char* operator()(const unsigned int &bitsPerPixel, const unsigned int &width, const unsigned int &height, TGAMemoryType mType) {						if (mType == GWTGA_IMAGE_DATA) {
						return new char[(bitsPerPixel / 8) * (height * width)];
					} else {
						return new char[(bitsPerPixel / 8) * (height * width)];
					}
				}

			};

			typedef void(*fetchPixelFunc)(void* target, void* input, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel);
			typedef bool(*fetchPixelsFunc)(char* target, std::istream &stream, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel, size_t count);

			void fetchPixelUncompressed(void* target, void* input, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel);
			void fetchPixelColorMap(void* target, void* input, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel);

			bool fetchPixelsUncompressed(char* target, std::istream &stream, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel, size_t count);
			bool fetchPixelsColorMap(char* target, std::istream &stream, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel, size_t count);

			template<fetchPixelFunc, fetchPixelsFunc>
			bool decompressRLE(char* target, size_t pixelsNumber, size_t bytesPerInputPixel, std::istream &stream, char* colorMap, size_t bytesPerOutputPixel);

		}
	} 
}