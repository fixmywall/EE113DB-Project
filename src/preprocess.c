#include "preprocess.h"
#include "qdbmp.h"
#include "system.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>



#define THETA_DELTA_DEG 0.5		//accuracy of the deskew algorithm
#define MAX_R_BINS 2000
#define MAX_SKEW_ANGLE_DEG 30		// maximum angle to consider. for practical purposes, this is far less than 180 degrees

static const double PI = 3.1415927;

// input_bmp: 24 BPP bitmap
unsigned char* ConvertImageToGrayscale(unsigned char* input_bmp, int height, int width) {
	//allocate memory for output grayscale bitmap (1 byte per pixel)
	unsigned char* bitmap_grayscale = MemAllocate(sizeof(unsigned char) * height * width);

	//convert RGB bmp image to grayscale using the luminosity 
	int x, y;
	//for (x = 0; x < width; x++) {
	//	for (y = 0; y < height; y++) {
	//		UCHAR pixel_value = 0;
	//		UCHAR r, g, b;
	//		BMP_GetPixelRGB(input_bmp, x, y, &r, &g, &b);

	//		if (BMP_GetBitsPerPixel(input_bmp) == 8) {
	//			pixel_value = b;
	//		}
	//		else {
	//			pixel_value = (UCHAR)(0.21*r + 0.72*g + 0.07*b);
	//		}

	//		bitmap_grayscale[x + y * width] = pixel_value;
	//	}
	//}


	// using using LCDK's usb_imread, image starts in bottom left. we want to store starting from the top left
	int bytes_per_row = 4.0 * ceil(width * 3.0 / 4.0);
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++) {
#if LCDK == 0
			int col_index = (x * 3 + y * bytes_per_row);
#else
			int col_index = (x + y * width) * 3;
#endif
			// info stored in BGR order
			unsigned char r = input_bmp[col_index + 2];
			unsigned char g = input_bmp[col_index + 1];
			unsigned char b = input_bmp[col_index];
			unsigned char pixel_value = (unsigned char)(0.21*r + 0.72*g + 0.07*b);

			bitmap_grayscale[x + (height - y - 1) * width] = pixel_value;
		}
	}		

	//free input color image
	FreeMemory(input_bmp);

	return bitmap_grayscale;
}



//frees the members of a BinaryDocument struct
void BinaryDocument_Free(BinaryDocument* doc) {
	FreeMemory(doc->image);
	FreeMemory(doc->boundaries);
}

// Takes in a grayscale image and binarizes it (makes it black and white)
// Uses Otsu's method, a global thresholding algorithm
BinaryDocument Binarize(unsigned char* bmp_rgb, int height, int width) {
	unsigned char* image;
	int i;
	image = ConvertImageToGrayscale(bmp_rgb, height, width);

	int total_pixels = height * width;		// total number of pixels in the grayscale image
	int background_color;					// 0 for black, 1 for white background
	int black_pixel_count = 0;				// count of black pixels in imgae
	double total_intensity = 0; 			// sum of intensities of each pixel in the image
	int histogram[256]; 					// histogram of the intensities of the pixels


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
	unsigned char* output_image = MemAllocate(sizeof(unsigned char) * total_pixels);
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

	//free the grayscale image
	FreeMemory(image);

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
	unsigned char* og_image = bd->image;
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
	unsigned char* output_image = MemAllocate(sizeof(unsigned char) * total_pixels);

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
	FreeMemory(og_image);
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

/*************************************************************
*	DESKEW
*	If exists, finds the rotational skew angle via the Hough Transform and
*	rotates the image to "fix" the skew
*	bd: pointer to a BinaryDocument object
**************************************************************/
void Deskew(BinaryDocument* bd) {
	int max_theta_deg = 90 + MAX_SKEW_ANGLE_DEG;
	int min_theta_deg = 90 - MAX_SKEW_ANGLE_DEG;
	int height = bd->height;
	unsigned char* og_image = bd->image;							// original image
	int width = bd->width;
	int fg_color = !bd->background_color;							// foreground color
	int max_r = sqrt(pow(bd->height, 2) + pow(bd->width, 2));		// maximum possible value of r
	int r_bin_count = MAX_R_BINS;
	double r_delta = 2.0 * (double)max_r / r_bin_count;				// bin size for r
	int theta_bin_count = 2 * MAX_SKEW_ANGLE_DEG / THETA_DELTA_DEG + 1;	// number of bins for the theta dimension

	int* hough_votes = (int)calloc(r_bin_count * theta_bin_count, sizeof(int));	// stores votes for the Hough transform

																				// iterate through image, transforming all foreground pixels into their huff version
	int x, y;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			int pix_index = x + y * width;				// 1D index of the pixel
			if (og_image[pix_index] == fg_color) {		// check for a foreground pixel
				int theta_index;						// index for theta in the vote matrix
				for (theta_index = 0; theta_index < theta_bin_count; theta_index++) {
					double theta_deg = (theta_index - theta_bin_count / 2) * THETA_DELTA_DEG + 90;		// sweeps from 90-SKEW_MAX to 90+SKEW_MAX
					double r = x * cos(theta_deg * PI / 180.0) + y * sin(theta_deg * PI / 180.0);
					int r_index = (int)floor(r / r_delta) + r_bin_count / 2;	// index for the r dimension of the histogram

																				//update vote count
					hough_votes[theta_index + r_index * theta_bin_count]++;
				}
			}
		}
	}

	// find the skew angle by locating the theta with the maximum vote value
	int theta_i, r_i;			// indices for the vote matrix
	double skew_deg = 0;		// detected skew angle in degrees
	int max_vote = 0;			// keeps track of the highest observed vote count
	for (theta_i = 0; theta_i < theta_bin_count; theta_i++) {
		for (r_i = 0; r_i < r_bin_count; r_i++) {
			int vote_value = hough_votes[theta_i + r_i * theta_bin_count];
			if (vote_value > max_vote) {
				max_vote = vote_value;

				// convert from theta index to degrees
				skew_deg = (theta_i - theta_bin_count / 2) * THETA_DELTA_DEG + 90;		//degrees are in hough space, so horizontal lines have 90 degree skew

			}
		}
	}

	// rotate image in the opposite direction of the skew
	Rotate(bd, 90 - skew_deg);
}
