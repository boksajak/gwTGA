#include "gwTGA.h"


bool cmpArrays(char* a1, size_t n1, char* a2, size_t n2){

	if (n1 != n2) return false;

	for (size_t i = 0; i < n1; i++) {
		if (a1[i] != a2[i]) return false;
	}

	return true;
}

bool test(char* testName, char* tgaFileName, char* testFileName) {

	// load tga image
	gw::tga::TGAImage img = gw::tga::LoadTgaFromFile(tgaFileName);

	// load reference file
	std::ifstream ifs;

	ifs.open(testFileName, std::ifstream::in | std::ifstream::ate | std::ifstream::binary);

	if (ifs.fail()) {
		std::cout << testName << "error! Cannot load reference image" << std::endl;
		// TODO Error
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

void printImageInfo(gw::tga::TGAImage img) {

	std::cout << "Width: " << (int) img.width << std::endl;
	std::cout << "Height: " << (int) img.height << std::endl;
	std::cout << "Bits per pixel: " << (int) img.bitsPerPixel << std::endl;
	std::cout << "Attribute bits per pixel: " << (int) img.attributeBitsPerPixel << std::endl;
	std::cout << "Origin: ";

	switch (img.origin) {
	case gw::tga::TGAImageOrigin::BOTTOM_LEFT:
		std::cout << "Bottom left";
		break;
	case gw::tga::TGAImageOrigin::BOTTOM_RIGHT:
		std::cout << "Bottom right";
		break;
	case gw::tga::TGAImageOrigin::TOP_LEFT:
		std::cout << "Top left";
		break;
	case gw::tga::TGAImageOrigin::TOP_RIGHT:
		std::cout << "Top right";
		break;
	}
	std::cout << std::endl;

}

int main(int argc, char *argv[]) {

	test("Testing 8-bit greyscale image uncompressed...", "mandrill_8.tga", "mandrill_8.tga.test");
	test("Testing 8-bit greyscale RLE compressed image...", "mandrill_8rle.tga", "mandrill_8rle.tga.test");
	test("Testing 8-bit greyscale image with 8 bit palette...", "mandrill_8_palette8.tga", "mandrill_8_palette8.tga.test");
	test("Testing 8-bit greyscale image with 8 bit palette RLE compressed...", "mandrill_8rle_palette8.tga", "mandrill_8rle_palette8.tga.test");

	test("Testing 16-bit RGB image uncompressed...", "mandrill_16.tga", "mandrill_16.tga.test");
	test("Testing 16-bit RGB RLE compressed image...", "mandrill_16rle.tga", "mandrill_16rle.tga.test");
	test("Testing 16-bit RGB image with 8 bit palette...", "mandrill_16_palette8.tga", "mandrill_16_palette8.tga.test");
	test("Testing 16-bit RGB image with 8 bit palette RLE compressed...", "mandrill_16rle_palette8.tga", "mandrill_16rle_palette8.tga.test");

	test("Testing 24-bit RGB image uncompressed...", "mandrill_24.tga", "mandrill_24.tga.test");
	test("Testing 24-bit RGB RLE compressed image...", "mandrill_24rle.tga", "mandrill_24rle.tga.test");
	test("Testing 24-bit RGB image with 8 bit palette...", "mandrill_24_palette8.tga", "mandrill_24_palette8.tga.test");
	test("Testing 24-bit RGB image with 8 bit palette RLE compressed...", "mandrill_24rle_palette8.tga", "mandrill_24rle_palette8.tga.test");

	test("Testing 32-bit RGB image uncompressed...", "mandrill_32.tga", "mandrill_32.tga.test");
	test("Testing 32-bit RGB RLE compressed image...", "mandrill_32rle.tga", "mandrill_32rle.tga.test");
	test("Testing 32-bit RGB image with 8 bit palette...", "mandrill_32_palette8.tga", "mandrill_32_palette8.tga.test");
	test("Testing 32-bit RGB image with 8 bit palette RLE compressed...", "mandrill_32rle_palette8.tga", "mandrill_32rle_palette8.tga.test");

	std::cout << std::endl;

	printImageInfo(gw::tga::LoadTgaFromFile("mandrill_32.tga"));

	std::cout << std::endl;
	std::cout << "Press any key to end";

	getchar();

}