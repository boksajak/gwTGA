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
	test("Testing 24-bit RGB image uncompressed...", "mandrill_24.tga", "mandrill_24.tga.test");
	test("Testing 24-bit RGB RLE compressed image...", "mandrill_24rle.tga", "mandrill_24rle.tga.test");
	test("Testing 24-bit RGB image with 8 bit palette...", "mandrill_24_palette8.tga", "mandrill_24_palette8.tga.test");
	test("Testing 24-bit RGB image with 8 bit palette RLE compressed...", "mandrill_24rle_palette8.tga", "mandrill_24rle_palette8.tga.test");
}