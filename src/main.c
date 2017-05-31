#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "qdbmp.h"
#include "segment.h"
#include "preprocess.h"
#include "ocr.h"

#define PI 3.1415927


// input_bmp: pointer to a BMP struct as defined by qdbmp.h
UCHAR* ConvertImageToGrayscale(BMP* input_bmp, int height, int width) {
	//allocate memory for output grayscale bitmap (1 byte per pixel)
	UCHAR* bitmap_grayscale = (UCHAR*)malloc(sizeof(UCHAR) * height * width);

	//convert RGB bmp image to grayscale using the luminosity 
	int x, y;
	for (x = 0; x < width; x++) {
		for (y = height-1; y >= 0 ; y--) {
			UCHAR pixel_value = 0;
			UCHAR r, g, b;
			BMP_GetPixelRGB(input_bmp, x, y, &r, &g, &b);

			if (BMP_GetBitsPerPixel(input_bmp) == 8) {
				pixel_value = b;
			}
			else {
				pixel_value = (UCHAR)(0.21*r + 0.72*g + 0.07*b);
			}

			bitmap_grayscale[x + (height - 1 - y) * width] = pixel_value;
		}
	}

	//free input color image
	BMP_Free(input_bmp);

	return bitmap_grayscale;
}


void ResizeCharacterTest() {
	int resized_height = 80;
	int resized_width = 80;

	char* input_file = "data/test_char.bmp";
	BMP* bmp;
	UCHAR* bmp_grayscale;
	bmp = BMP_ReadFile(input_file);
	BMP_CHECK_ERROR(stderr, -1); /* If an error has occurred, notify and exit */

	int width = BMP_GetWidth(bmp);
	int height = BMP_GetHeight(bmp);

	bmp_grayscale = ConvertImageToGrayscale(bmp, height, width);
	BinaryDocument bd = Binarize(bmp_grayscale, height, width);

	UCHAR* resized_character = ResizeCharacter(bd.image, height, width, resized_height, resized_width, bd.width);

	//write output to file
	FILE* fp;
	fp = fopen("data/resized_char.txt", "w");
	int i;
	for (i = 0; i < resized_height * resized_width; i++) {
		fprintf(fp, "%d\n", resized_character[i]);
	}
	fclose(fp);

	free(resized_character);
	BinaryDocument_Free(&bd);
}


void DocumentTest() {
	char* input_file = "data/test_camera_1.bmp";
	BMP* bmp;
	UCHAR* bitmap_grayscale;

	//
	//	Read a BMP file from the mass storage device
	//
	bmp = BMP_ReadFile(input_file);
	BMP_CHECK_ERROR(stderr, -1); /* If an error has occurred, notify and exit */

	int width, height;
	width = BMP_GetWidth(bmp);
	height = BMP_GetHeight(bmp);

	// convert the BMP data into grayscale UCHAR array
	bitmap_grayscale = ConvertImageToGrayscale(bmp, height, width);

	//convert to binary image
	BinaryDocument binary_doc;
	binary_doc = Binarize(bitmap_grayscale, height, width);

	//deskew the document
	Deskew(&binary_doc);

	unsigned char* mask;
	mask = SegmentText(&binary_doc);

	//write output to file
	FILE* fp;
	fp = fopen("data/output.txt", "w");
	int i;
	for (i = 0; i < height*width; i++) {
		fprintf(fp, "%d\n", (int)binary_doc.image[i]);
	}
	fclose(fp);
	printf("Finished writing processed image.\n");

	FILE* fp2;
	fp2 = fopen("data/mask_output.txt", "w");
	for (i = 0; i < height*width; i++) {
		fprintf(fp2, "%d\n", (int)mask[i]);
	}
	fclose(fp2);

	//free allocated memory
	BinaryDocument_Free(&binary_doc);
	free(mask);
}

//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
int main(void)
{
	DocumentTest();
	return 0;
}
