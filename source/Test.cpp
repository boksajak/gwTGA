#include "gwTGA.h"


int main(int argc, char *argv[]) {
	gw::tga::TGAImage img = gw::tga::LoadTgaFromFile("mandrill_24.tga");
	gw::tga::TGAImage img2 = gw::tga::LoadTgaFromFile("mandrill_24rle.tga");
	gw::tga::TGAImage img3 = gw::tga::LoadTgaFromFile("mandrill_24_palette8.tga");
	gw::tga::TGAImage img4 = gw::tga::LoadTgaFromFile("mandrill_24rle_palette8.tga");
}