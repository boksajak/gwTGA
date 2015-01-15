#include "gwTGA.h"
#include <fstream>  

void printImageInfo(gw::tga::TGAImage img) {

	if (img.hasError()) {
		std::cout << "error! Cannot load image" << std::endl;
		return;
	}

	std::cout << "Width: " << (int) img.width << std::endl;
	std::cout << "Height: " << (int) img.height << std::endl;
	std::cout << "X-Origin: " << (int) img.xOrigin << std::endl;
	std::cout << "Y-Origin: " << (int) img.yOrigin << std::endl;
	std::cout << "Bits per pixel: " << (int) img.bitsPerPixel << std::endl;
	std::cout << "Attribute bits per pixel: " << (int) img.attributeBitsPerPixel << std::endl;
	std::cout << "Origin: ";

	switch (img.origin) {
	case gw::tga::GWTGA_BOTTOM_LEFT:
		std::cout << "Bottom left";
		break;
	case gw::tga::GWTGA_BOTTOM_RIGHT:
		std::cout << "Bottom right";
		break;
	case gw::tga::GWTGA_TOP_LEFT:
		std::cout << "Top left";
		break;
	case gw::tga::GWTGA_TOP_RIGHT:
		std::cout << "Top right";
		break;
	}
	
	std::cout << std::endl;

	std::cout << "Type: ";
	switch (img.colorType) {
	case gw::tga::GWTGA_UNKNOWN:
		std::cout << "Unknown";
		break;
	case gw::tga::GWTGA_GREYSCALE:
		std::cout << "Greyscale";
		break;
	case gw::tga::GWTGA_RGB:
		std::cout << "RGB";
		break;
	}
	std::cout << std::endl;
}

bool cmpArrays(char* a1, size_t n1, char* a2, size_t n2){

	if (n1 != n2) return false;

	for (size_t i = 0; i < n1; i++) {
		if (a1[i] != a2[i]) return false;
	}

	return true;
}

bool test(char* testName, char* tgaFileName, char* testFileName) {

	// load tga image
	gw::tga::TGAImage img = gw::tga::LoadTga(tgaFileName);

	if (img.error != gw::tga::GWTGA_NONE) {
		std::cout << testName << "error! Cannot load image" << std::endl;
		return false;
	}

	// load reference file
	std::ifstream ifs;

	ifs.open(testFileName, std::ifstream::in | std::ifstream::ate | std::ifstream::binary);

	if (ifs.fail()) {
		std::cout << testName << "error! Cannot load reference image" << std::endl;
		return false;
	}

	size_t testFileSize = ifs.tellg(); 

	char* testImg = new char[testFileSize];
	ifs.seekg(0, std::ios::beg);

	ifs.read(testImg, testFileSize);

	ifs.close();

	// compare loaded image to reference
	bool result = cmpArrays(img.bytes, img.width * img.height * (img.bitsPerPixel / 8), testImg, testFileSize);

	delete[] testImg;

	// print result
	std::cout << testName;

	if (result) {
		std::cout << "OK" << std::endl;
	} else {
		std::cout << "fail!" << std::endl;
	}

	return result;
}

int main(int argc, char *argv[]) {

	test("Testing 8-bit greyscale image uncompressed...", "test_images\\mandrill_8.tga", "test_images\\mandrill_8.tga.test");
	test("Testing 8-bit greyscale RLE compressed image...", "test_images\\mandrill_8rle.tga", "test_images\\mandrill_8rle.tga.test");
	test("Testing 8-bit greyscale image with 8 bit palette...", "test_images\\mandrill_8_palette8.tga", "test_images\\mandrill_8_palette8.tga.test");
	test("Testing 8-bit greyscale image with 8 bit palette RLE compressed...", "test_images\\mandrill_8rle_palette8.tga", "test_images\\mandrill_8rle_palette8.tga.test");

	test("Testing 16-bit RGB image uncompressed...", "test_images\\mandrill_16.tga", "test_images\\mandrill_16.tga.test");
	test("Testing 16-bit RGB RLE compressed image...", "test_images\\mandrill_16rle.tga", "test_images\\mandrill_16rle.tga.test");
	test("Testing 16-bit RGB image with 8 bit palette...", "test_images\\mandrill_16_palette8.tga", "test_images\\mandrill_16_palette8.tga.test");
	test("Testing 16-bit RGB image with 8 bit palette RLE compressed...", "test_images\\mandrill_16rle_palette8.tga", "test_images\\mandrill_16rle_palette8.tga.test");

	test("Testing 24-bit RGB image uncompressed...", "test_images\\mandrill_24.tga", "test_images\\mandrill_24.tga.test");
	test("Testing 24-bit RGB RLE compressed image...", "test_images\\mandrill_24rle.tga", "test_images\\mandrill_24rle.tga.test");
	test("Testing 24-bit RGB image with 8 bit palette...", "test_images\\mandrill_24_palette8.tga", "test_images\\mandrill_24_palette8.tga.test");
	test("Testing 24-bit RGB image with 8 bit palette RLE compressed...", "test_images\\mandrill_24rle_palette8.tga", "test_images\\mandrill_24rle_palette8.tga.test");

	test("Testing 32-bit RGB image uncompressed...", "test_images\\mandrill_32.tga", "test_images\\mandrill_32.tga.test");
	test("Testing 32-bit RGB RLE compressed image...", "test_images\\mandrill_32rle.tga", "test_images\\mandrill_32rle.tga.test");
	test("Testing 32-bit RGB image with 8 bit palette...", "test_images\\mandrill_32_palette8.tga", "test_images\\mandrill_32_palette8.tga.test");
	test("Testing 32-bit RGB image with 8 bit palette RLE compressed...", "test_images\\mandrill_32rle_palette8.tga", "test_images\\mandrill_32rle_palette8.tga.test");

	std::cout << std::endl;

	printImageInfo(gw::tga::LoadTga("test_images\\guitar.tga"));

	gw::tga::TGAImage img = gw::tga::LoadTga("test_images\\guitar.tga");
	if (gw::tga::SaveTga("test_flipped.tga", img, (gw::tga::TGAOptions)(gw::tga::GWTGA_FLIP_VERTICALLY)) != gw::tga::GWTGA_NONE) {
		std::cout << "Error while saving TGA" << std::endl;
	}

	std::cout << std::endl;
	std::cout << "Press any key to end";

	getchar();

}