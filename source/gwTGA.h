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

#include <iostream>
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

		enum TGAOptions {
			GWTGA_OPTIONS_NONE = 0,
			GWTGA_RETURN_COLOR_MAP = 1,
			GWTGA_FLIP_VERTICALLY = 2,
			GWTGA_FLIP_HORIZONTALLY = 4,
			GWTGA_COMPRESS_RLE = 8
		};

		enum TGAColorType {
			GWTGA_UNKNOWN = 0,
			GWTGA_GREYSCALE,
			GWTGA_RGB
		};

		enum TGAMemoryType {
			GWTGA_IMAGE_DATA,
			GWTGA_COLOR_PALETTE,
			GWTGA_COLOR_PALETTE_TEMPORARY //< Used only to decode image, can be thrown away after loading the image
		};

		class ITGALoaderListener { 
		public: 
			virtual ~ITGALoaderListener() {}
			virtual char* operator()(const unsigned int &bitsPerPixel, const unsigned int &width, const unsigned int &height, TGAMemoryType mType) = 0; 
		}; 

		struct TGAColorMap {

			TGAColorMap() : bytes(NULL), length(0), bitsPerPixel(0) {}

			char* bytes;
			unsigned int length;
			unsigned char bitsPerPixel;
		};

		struct TGAImage {

			TGAImage() :bytes(NULL), width(0), height(0), bitsPerPixel(0), attributeBitsPerPixel(0), origin(GWTGA_UNDEFINED), xOrigin(0), yOrigin(0), error(GWTGA_NONE), colorType(GWTGA_UNKNOWN) {}

			char*			bytes;

			unsigned int	width;
			unsigned int	height;
			unsigned char	bitsPerPixel;

			unsigned char	attributeBitsPerPixel;

			TGAImageOrigin	origin;
			unsigned int	xOrigin;
			unsigned int	yOrigin;

			TGAError		error;
			TGAColorType	colorType;

			TGAColorMap		colorMap;

			bool hasError() const { return error != GWTGA_NONE; }
			bool hasColorMap() const { return colorMap.bytes != NULL && colorMap.length != 0 && colorMap.bitsPerPixel != 0; }
		};

		// -------------------------------------------------------------------------------------
		//  Load overloads
		// -------------------------------------------------------------------------------------

		TGAImage LoadTga(char* fileName);
		TGAImage LoadTga(std::istream &stream);

		TGAImage LoadTga(char* fileName, ITGALoaderListener* listener);
		TGAImage LoadTga(std::istream &stream, ITGALoaderListener* listener);

		TGAImage LoadTga(char* fileName, TGAOptions options);
		TGAImage LoadTga(std::istream &stream, TGAOptions options);

		TGAImage LoadTga(char* fileName, ITGALoaderListener* listener, TGAOptions options);
		TGAImage LoadTga(std::istream &stream, ITGALoaderListener* listener, TGAOptions options);

		// -------------------------------------------------------------------------------------
		//  Save overloads
		// -------------------------------------------------------------------------------------

		TGAError SaveTga(char* fileName, unsigned int width, unsigned int height, unsigned char bitsPerPixel, char* pixels, TGAColorType colorType, TGAImageOrigin origin, unsigned int xOrigin, unsigned int yOrigin);
		TGAError SaveTga(std::ostream &stream, unsigned int width, unsigned int height, unsigned char bitsPerPixel, char* pixels, TGAColorType colorType, TGAImageOrigin origin, unsigned int xOrigin, unsigned int yOrigin);

		TGAError SaveTga(char* fileName, const TGAImage &image, TGAOptions options);
		TGAError SaveTga(std::ostream &stream, const TGAImage &image, TGAOptions options);

		TGAError SaveTga(char* fileName, const TGAImage &image);
		TGAError SaveTga(std::ostream &stream, const TGAImage &image);

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

			template<size_t tempMemorySize = 768>
			class TGALoaderListener : public ITGALoaderListener {
			public: 
				TGALoaderListener(bool persistentColorMapMemory);
				~TGALoaderListener();
				char* operator()(const unsigned int &bitsPerPixel, const unsigned int &width, const unsigned int &height, TGAMemoryType mType);

			private:
				char tempMemory[tempMemorySize];
				char* colorMapMemory;
				bool persistentColorMapMemory;
			};

			// -------------------------------------------------------------------------------------
			//  Pixel data reading 
			// -------------------------------------------------------------------------------------
			typedef size_t(*flipFunc)(size_t i, size_t width, size_t height, size_t bpp);
			void getFlipFunction(flipFunc &flipFuncType, size_t &stride, bool flipVertically, bool flipHorizontally, size_t width, size_t height);

			typedef void(*fetchPixelFunc)(char* target, char* input, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel);
			typedef bool(*fetchPixelsFunc)(char* target, std::istream &stream, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel, size_t count);

			void fetchPixelUncompressed(char* target, char* input, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel);
			void fetchPixelColorMap(char* target, char* input, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel);

			bool fetchPixelsUncompressed(char* target, std::istream &stream, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel, size_t count);
			bool fetchPixelsColorMap(char* target, std::istream &stream, size_t bytesPerInputPixel, char* colorMap, size_t bytesPerOutputPixel, size_t count);

			template<fetchPixelFunc, fetchPixelsFunc>
			bool decompressRLE(char* target, size_t pixelsNumber, size_t bytesPerInputPixel, std::istream &stream, char* colorMap, size_t bytesPerOutputPixel, size_t imgWidth, size_t imgHeight, flipFunc flip, bool perPixelProcessing);

			// -------------------------------------------------------------------------------------
			//  Image processing
			// -------------------------------------------------------------------------------------

			size_t flipFuncPass(size_t i, size_t width, size_t height, size_t bpp);
			size_t flipFuncVertical(size_t i, size_t width, size_t height, size_t bpp);
			size_t flipFuncHorizontal(size_t i, size_t width, size_t height, size_t bpp);
			size_t flipFuncHorizontalAndVertical(size_t i, size_t width, size_t height, size_t bpp);

			typedef char*(*fetchFunc)(char* source, unsigned int x, unsigned int y, unsigned int imgWidth, unsigned int imgHeight);
			typedef char*(*processFunc)(char* target, char* source);

			template<processFunc process, fetchFunc fetch>
			void processToStream(std::ostream &stream, char* source, unsigned int imgWidth, unsigned int imgHeight, int beginX, int strideX, int endX, int beginY, int strideY, int endY, size_t resultSize);

			template<processFunc process, fetchFunc fetch>
			void processToArray(std::istream &stream, char* target, unsigned int imgWidth, unsigned int imgHeight, int beginX, int strideX, int endX, int beginY, int strideY, int endY, size_t resultSize);

			char* fetchXPlusY(char* source, unsigned int x, unsigned int y, unsigned int imgWidth, unsigned int imgHeight);
			char* processPassThrough(char* target, char* source);

			bool cmpPixels(char* pa, char* pb, char bytesPerPixel);

			bool compressRLE(std::ostream &stream, char* source, size_t imgWidth, size_t imgHeight, size_t bytesPerInputPixel);
			bool compressRLE(std::ostream &stream, char* source, size_t imgWidth, size_t imgHeight, size_t bytesPerInputPixel, flipFunc flipFuncType);
		}
	} 
}