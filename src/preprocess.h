#ifndef PREPROCESS_H
#define PREPROCESS_H
#define _CRT_SECURE_NO_DEPRECATE
/*
*	Defines the structure for handling binary bitmap images
*/

#define BLACK_PIXEL 0
#define WHITE_PIXEL 1

#include <stdlib.h>
#include "qdbmp.h"

typedef struct _BinaryDocument {
	unsigned char* image;			// each element corresponds to one pixel for computation
	unsigned char* boundaries;	
	int background_color;	// 0 for white background, 1 for black background
	int height;				// height of the image in pixels
	int width;				// width of the image in pixels
} BinaryDocument;

unsigned char* ConvertImageToGrayscale(BMP* input_bmp, int height, int width);

//frees the members of a BinaryDocument struct
void BinaryDocument_Free(BinaryDocument* doc);

// binarizes an an RGB image with file name as a parameter
BinaryDocument Binarize(unsigned char* input_file);

/************************************************************
*	-BINARYROTATE-
*	This algorithm rotates the image clockwise at an angle (radians) specified as an input.
*	It iterates through the DESTINATION image and computes the inverse rotation for each index to find
*	the corresponding index at the SOURCE image.
*	By looping through the DESTINATION image, the result will no longer have gaps due to aliasing
*	binary_doc: pointer to BinaryDocument object to modify
*	angle_deg: rotation angle in degrees
*************************************************************/
void Rotate(BinaryDocument* bd, double angle_deg);


/**************************************************************
*	de-skews the input binary document
*	Computes a skew angle using the Hough Transform
*	Corrects the skew by calling the Rotate() method
***************************************************************/
void Deskew(BinaryDocument* bd);

void WriteToFile(char* file_path, unsigned char* image, int height, int width);



#endif
