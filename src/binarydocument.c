#define _CRT_SECURE_NO_DEPRECATE

#include "binarydocument.h"
#include <windows.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

static const double PI = 3.1415927;

//frees the members of a BinaryDocument struct
void BinaryDocument_Free(BinaryDocument* doc) {
	free(doc->image);
}

// Takes in a grayscale image and binarizes it (makes it black and white)
// Uses Otsu's method, a global thresholding algorithm
BinaryDocument Binarize(unsigned char* image, int height, int width) {
	int total_pixels = height * width;		// total number of pixels in the grayscale image
	int background_color;					// 0 for black, 1 for white background
	int black_pixel_count = 0;				// count of black pixels in imgae
	double total_intensity = 0; 			// sum of intensities of each pixel in the image
	int histogram[256]; 					// histogram of the intensities of the pixels

	int i;
	for (i = 0; i < 256; i++) {				// set histogram array to zeros
		histogram[i] = 0;
	}

	int t;
	for (i = 0; i<total_pixels; i++) {
		histogram[(int)image[i]]++; 		// populate the color histogram
	}

	for (i = 0; i < 256; i++) {
		total_intensity += histogram[i] * i;
	}

	int optimal_threshold = 0;		// optimal threshold that will divide the pixels into background (dark) and foreground (light)
	double w_b = 0;					// background weight: fraction of pixels in the background class
	double w_f = 0; 				// foreground weight: fraction of pixels in the foreground class
	double sum_b = 0; 				// sum of background intensities
	double max_inter_var = 0; 		//maximum inter-class variance: defined as w_b * w_f * (u_b - u_f)^2

									// iterate through all possible threshold values to find the optimal one
									// the optimal threshold is the one that maximizes the between-class variance
	for (t = 0; t < 256; t++) {		// for each possible threshold (0 to 255)
		w_b += histogram[t];
		sum_b += t * histogram[t]; 			// update sum of BG intensities
		if (w_b == 0) continue; 			// if background is zero, go to next iteration

		w_f = total_pixels - w_b;
		if (w_f == 0) break;

		double mean_b = sum_b / w_b;			// background mean: mean of pixel intensities in background class
		double mean_f = (total_intensity - sum_b) / w_f;	// foreground mean
		double inter_var = w_b * w_f * pow(mean_b - mean_f, 2); 	// inter-class variance for current threshold

																	// check if a new inter-class variance maximum has been found
																	// if so, update the optimal threshold to the current value of t
		if (inter_var > max_inter_var) {
			max_inter_var = inter_var;
			optimal_threshold = t;
		}
	}

	//apply global threshold to newly allocated output image
	UCHAR* output_image = (UCHAR*)malloc(sizeof(UCHAR) * total_pixels);
	for (i = 0; i<height*width; i++) {
		int intens = image[i];
		if (intens >= optimal_threshold) {
			output_image[i] = WHITE_PIXEL; 			// set to white pixel (1 for binary image of 1 BPP)	
		}
		else {
			output_image[i] = BLACK_PIXEL;			// set to black pixel (0 for binary image of 1 BPP)
			black_pixel_count++;
		}
	}

	// set background color to the majority color count
	int white_pixel_count = total_pixels - black_pixel_count;

	if (white_pixel_count > black_pixel_count) {
		background_color = WHITE_PIXEL;
	}
	else background_color = BLACK_PIXEL;

	//define output struct
	BinaryDocument output_doc;
	output_doc.background_color = background_color;
	output_doc.height = height;
	output_doc.width = width;
	output_doc.image = output_image;

	//free the input image
	free(image);

	return output_doc;
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
	if (fabs(angle_deg) < 0.1) {
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

void WriteToFile(char* file_path, unsigned char* image, int height, int width) {
	FILE* fp;
	fp = fopen(file_path, "w");
	int i;
	for (i = 0; i < height*width; i++) {
		fprintf(fp, "%d\n", (int)image[i]);
	}
	fclose(fp);
}