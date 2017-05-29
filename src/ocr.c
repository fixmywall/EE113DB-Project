#include "ocr.h"
#include <math.h>
#include "binarydocument.h"

static const int RESIZED_CHAR_DIM = 40;		// dimension of resized character image for feature extraction
static const int CHAR_ZONE_COUNT = 16;

void TrainCharacter(int* features, char c) {

}

char ClassifyCharacter(int* features) {

}

/*	takes in a GRAYSCALE (8 bpp) image of a character and computes the feature vector
*	feature vector is determined by dividing the resized image into 16 zones
*	and computing the average pixel value in those zones
*/
double* GetFeatureVector(char* char_pixels, int height, int width, int doc_width) {
	if (height == 0 || width == 0) return;
	unsigned char* resized_image = ResizeCharacter(char_pixels, height, width, RESIZED_CHAR_DIM, RESIZED_CHAR_DIM, doc_width);
	WriteToFile("data/resized_char.txt", resized_image, 40, 40);
	double* feature_vector = (double*)malloc(sizeof(double) * CHAR_ZONE_COUNT);
	int i;
	for (i = 0; i < CHAR_ZONE_COUNT; i++) {
		feature_vector[i] = 0.0;
	}

	// divide resized character image into zones
	int zone_count_width = (int)sqrt(CHAR_ZONE_COUNT);		// number of zones along width
	int zone_length = RESIZED_CHAR_DIM / zone_count_width;	// length of zone (in pixels) along one dimension in 
	int zone_pixel_count = (int)pow(zone_length, 2);		// number of pixels in each zone

	int x, y;
	for (y = 0; y < RESIZED_CHAR_DIM; y++) {
		for (x = 0; x < RESIZED_CHAR_DIM; x++) {
			int normalized_pix = resized_image[x + y * width];
			int zone_index = (x / zone_length) + (y / zone_length) * zone_count_width;
			feature_vector[zone_index] += 
				(1.0 - normalized_pix) / zone_pixel_count;		// add the contribution of single pixel to the density
		}
	}

	// free allocated memory
	free(resized_image);

	return feature_vector;
}

// interpolates points from a 2-D grid (used to resize a character image to a standard size)
// qij: value of pixel at x_i, y_j
float BilinearInterpolation(float q11, float q12, float q21, float q22, float x1, float x2, float y1, float y2, float x, float y) {
	float x2x1, y2y1, x2x, y2y, yy1, xx1;
	x2x1 = x2 - x1;
	y2y1 = y2 - y1;
	x2x = x2 - x;
	y2y = y2 - y;
	yy1 = y - y1;
	xx1 = x - x1;

	return 1.0 / (x2x1 * y2y1) * (
		q11 * x2x * y2y +
		q21 * xx1 * y2y +
		q12 * x2x * yy1 +
		q22 * xx1 * yy1
		);
}

//resizes a character represented by a binary image to an arbitrary size
//output size is given by parameters output_height and output_width
unsigned char* ResizeCharacter(unsigned char* image, int height, int width, int out_height, int out_width, int doc_width) {
	if (out_height == 0 || out_width == 0) return;

	unsigned char* output_image = (unsigned char*)malloc(sizeof(unsigned char) * out_height * out_width);

	int x, y;
	for (y = 0; y < out_height; y++) {
		for (x = 0; x < out_width; x++) {
			// get the x and y coordinates of the interpolated point
			float x_interp = ((float)x / out_width) * width;
			float y_interp = ((float)y / out_height) * height;
			int x_source = round(x_interp);
			int y_source = round(y_interp);
			if (x_source >= width) x_source = width - 1;
			if (y_source >= height) y_source = height - 1;

			// value of interpolated pixel
			int interp_val = image[x_source + y_source * doc_width];
			
			//if (interp_val < 0.5) {
			//	interp_val = 0;
			//}
			//else {
			//	interp_val = 1;
			//}
			output_image[x + y * out_width] = interp_val;
		}
	}
	return output_image;
}