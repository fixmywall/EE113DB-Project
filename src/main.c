#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "qdbmp.h"

#define PI 3.14159
#define BLACK_PIXEL 0
#define WHITE_PIXEL 1

typedef struct _BinaryDocument {
	UCHAR* image;			// each element corresponds to one pixel for computation 
	int background_color;	// 0 for white background, 1 for black background
	int height;				// height of the image in pixels
	int width;				// width of the image in pixels
} BinaryDocument;

// input_bmp: pointer to a BMP struct as defined by qdbmp.h
UCHAR* ConvertImageToGrayscale(BMP* input_bmp, int height, int width) {
	//allocate memory for output grayscale bitmap (1 byte per pixel)
	UCHAR* bitmap_grayscale = (UCHAR*)malloc(sizeof(UCHAR) * height * width);

	//convert RGB bmp image to grayscale using the luminosity 
	int x, y;
	for (x = 0; x < width; x++) {
		for (y = 0; y < height; y++) {
			UCHAR pixel_value = 0;
			UCHAR r, g, b;
			BMP_GetPixelRGB(input_bmp, x, y, &r, &g, &b);

			if (BMP_GetBitsPerPixel(input_bmp) == 8) {
				pixel_value = b;
			}
			else {
				pixel_value = (UCHAR)(0.21*r + 0.72*g + 0.07*b);
			}

			bitmap_grayscale[x + y * width] = pixel_value;
		}
	}

	//free input color image
	BMP_Free(input_bmp);

	return bitmap_grayscale;
}

//frees the members of a BinaryDocument struct
void BinaryDocument_Free(BinaryDocument* doc) {
	free(doc->image);
}

// Takes in a grayscale image and binarizes it (makes it black and white)
// Uses Otsu's method, a global thresholding algorithm
BinaryDocument Binarize(UCHAR* image, int height, int width) {
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

//checks for a skew angle and returns a new image array with the skew corrected
int Deskew(UCHAR* binary_image, int height, int width) {

}

/************************************************************
*	-BINARYROTATE-
*	binary_doc: pointer to BinaryDocument object to modify
*	angle_deg: rotation angle in degrees
*************************************************************/
void BinaryRotate(BinaryDocument* binary_doc, float angle_rad) {
	//for very small rotation angles, no need to modify image
	if (angle_rad < 0.01) {
		return;
	}

	UCHAR* og_image = binary_doc->image;
	int fg_color = !binary_doc->background_color;			//foreground color (0 or 1)
	int width = binary_doc->width;
	int height = binary_doc->height;
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
		output_image[i] = binary_doc->background_color;
	}

	//iterate through the original image row-by-row, transforming each pixel to its rotated position
	int x, y;
	for (y = 0; y < binary_doc->height; y++) {
		y_dif = y_center - y;
		for (x = 0; x < binary_doc->width; x++) {
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
	binary_doc->image = output_image;

	//deallocate original image
	free(og_image);
}



//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
int main(void)
{
	BMP* bmp;
	UCHAR* bitmap_grayscale;

	//
	//	Read a BMP file from the mass storage device
	//
	bmp = BMP_ReadFile("data/ocr_test_receipt.bmp");
	BMP_CHECK_ERROR(stderr, -1); /* If an error has occurred, notify and exit */

	int width, height;
	width = BMP_GetWidth(bmp);
	height = BMP_GetHeight(bmp);
	
	// convert the BMP data into grayscale UCHAR array
	bitmap_grayscale = ConvertImageToGrayscale(bmp, height, width);

	//convert to binary image
	BinaryDocument binary_doc;
	binary_doc = Binarize(bitmap_grayscale, height, width);

	//rotate binary image
	BinaryRotate(&binary_doc, PI / 4);

	//write output to file
	FILE* fp;
	fp = fopen("data/output.txt", "w");
	int i;
	for (i = 0; i < height*width; i++) {
		fprintf(fp, "%d\n", (int)binary_doc.image[i]);
	}
	fclose(fp);
	printf("Finished writing processed image.\n");

	//free allocated memory
	BinaryDocument_Free(&binary_doc);

	return 0;
}
