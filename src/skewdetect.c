#include "skewdetect.h"
#include <math.h>

#define THETA_DELTA_DEG 0.5		//accuracy of the deskew algorithm
#define MAX_R_BINS 2000
#define MAX_SKEW_ANGLE_DEG 30		// maximum angle to consider. for practical purposes, this is far less than 180 degrees

static const double PI = 3.1415927;

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
	int x,y;
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
	double skew_deg =  0;		// detected skew angle in degrees
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
	Rotate(bd, 90-skew_deg);
}