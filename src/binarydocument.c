#include "binarydocument.h"
#include <windows.h>
#include <math.h>

static const double PI = 3.1415927;

//frees the members of a BinaryDocument struct
void BinaryDocument_Free(BinaryDocument* doc) {
	free(doc->image);
}

/************************************************************
*	-BINARYROTATE-
*	This algorithm rotates the image counterclockwise at an angle (radians) specified as an input.
*	It iterates through the DESTINATION image and computes the inverse rotation for each index to find
*	the corresponding index at the SOURCE image.
*	By looping through the DESTINATION image, the result will no longer have gaps due to aliasing
*	binary_doc: pointer to BinaryDocument object to modify
*	angle_deg: rotation angle in degrees
*************************************************************/
void Rotate(BinaryDocument* bd, double angle_deg) {
	//for very small rotation angles, no need to modify image
	if (angle_deg < 0.1) {
		return;
	}

	double angle_rad = angle_deg * PI / 180.0;
	UCHAR* og_image = bd->image;
	int fg_color = !bd->background_color;			//foreground color (0 or 1)
	int width = bd->width;
	int height = bd->height;
	int x_center = width / 2;
	int y_center = height / 2;
	int x_dif, y_dif;
	int total_pixels = width * height;
	double sin_val = sin(angle_rad);
	double cos_val = cos(angle_rad);
	int i;
	int og_x, og_y;						// indices to the original unrotated image

										//allocate memory for rotated image
	UCHAR* output_image = malloc(sizeof(UCHAR) * total_pixels);

	//fill output image with the background color
	for (i = 0; i < total_pixels; i++) {
		output_image[i] = bd->background_color;
	}

	//iterate through the original image row-by-row, transforming each pixel to its rotated position
	int x, y;
	for (y = 0; y < bd->height; y++) {
		y_dif = y_center - y;
		for (x = 0; x < bd->width; x++) {
			//calculate x index from original image
			x_dif = x_center - x;
			og_x = x_center + (int)(-x_dif * cos_val - y_dif * sin_val);	//check boundaries
			if (og_x < 0 || og_x > width - 1) continue;

			og_y = y_center + (int)(-y_dif * cos_val + x_dif * sin_val);	//check boundaries
			if (og_y < 0 || og_y > height - 1) continue;

			//check color of original image at calculated index
			//if it corresponds to foreground color, set the pixel in the output image
			if (og_image[og_x + og_y*width] == fg_color) {
				output_image[x + y*width] = fg_color;
			}
		}
	}

	//set new image
	bd->image = output_image;

	//deallocate original image
	free(og_image);
}