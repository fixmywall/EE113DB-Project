/* 
*	Responsible for segmenting a deskewed binary document into individual characters.
*	After sucessful segmentation, the characters are ready to be passed into the OCR egnine
*/
#include "segment.h"
#include "ocr.h"
#include "preprocess.h"

/*
*	Returns an image with lines corresponding to the gaps between lines and characters
*	When superimposed with the original document, the lines should identify each indvidual character and space.
*/

static const double HOR_THRESHOLD = 0.0045;			//horizontal slice considered a space if proportion of foreground pixels <= this
static const double VERT_THRESHOLD = 0;
static const double PUNCTUATION_THRESHOLD = 0.2;	//if the proportion of height of the character to the line width is below this, classify as a punctuation symbol

/*
*	min_y:	lowest row that contains text pixels
*	max_y:	highest row that contains text pixels
*	Segments characters from the line specified by parameters min_y and max_y and performs feature extraction on
*	them
*/
void CharSegment(BinaryDocument* bd, unsigned char* mask, int* vpp, int min_y, int max_y) {
	int width = bd->width;

	int line_height = max_y - min_y + 1;

	if (line_height == 0) return;

	int char_min_x = 0;		// pixel left of beginning of run of text
	int char_max_x = 0;		// end of run of text
	int in_text_run = 0;
	
	int x, y;
	for (x = 0; x < bd->width; x++) {
		double pct_text = (double)vpp[x] / line_height;

		// find beginning of a run of text
		if (!in_text_run) {
			if ( pct_text > VERT_THRESHOLD) {		// find text in histogram
				// signal beginning of run
				in_text_run = 1;
				char_min_x = x - 1;			
				if (char_min_x < 0) char_min_x = 0;	// boundary check
			}
		}

		// find the end of a run of text
		else {			
			if (pct_text <= VERT_THRESHOLD) {		// space to right of character detected
				in_text_run = 0;
				char_max_x = x;

				int char_width = char_max_x - char_min_x - 1;
				int char_min_y = min_y - 1;
				int char_max_y = max_y + 1;

				// find the horizontal boundaries for the character (char_max_y and char_min_y
				int pixel_row_found = 0;
				int text_encountered = 0;		// flag that sets to 1 when a horz slice containing text is encountered
				for (y = min_y - 1; y <= max_y + 1; y++) {
					int fg_pixel_count = 0;			// pixel count of a horizontal slice of the text region
					for (x = char_min_x + 1; x < char_max_x; x++) {
						if (bd->image[x + y * width] == !bd->background_color) {
							fg_pixel_count++;
						}
					}

					if (fg_pixel_count > 0) {
						//find lowest black pixel in the character
						if (!text_encountered) {
							text_encountered = 1;
							char_min_y = y - 1;
							char_max_y = y - 1;
						}

						// find highest black pixel in character 
						else {
							char_max_y = y + 1;
						}
					}
				}
				if (char_min_y < 0) char_min_y = 0;							// boundary check
				if (char_max_y >= bd->height) char_max_y = bd->height - 1;	// boundary check

				int char_height = char_max_y - char_min_y - 1;

				// draw vertical lines in the mask
				for (y = char_min_y; y <= char_max_y; y++) {
					mask[char_min_x + y * width] = 1;
					mask[char_max_x + y * width] = 1;
				}

				// draw horizontal lines in the mask
				for (x = char_min_x; x <= char_max_x; x++) {
					mask[x + char_min_y * width] = 1;
					mask[x + char_max_y * width] = 1;
				}

				// from the character's pixels, obtain the feature vector	
				int char_pos = (char_min_x + 1) + (char_min_y + 1) * bd->width;		// position of the beginning of the character (LLC) with respect to the entire document

				int is_small_punct = 0;			// character is small punctuation (period, comma, quote, etc)
				if ((double)char_width / line_height <= PUNCTUATION_THRESHOLD) {
					int line_mid = (min_y + max_y) / 2;
					if (char_min_y > line_mid || char_max_y < line_mid) {
						is_small_punct = 1;
					}
				}

				int* feature_vector;
				if (!is_small_punct) {		// for regular characters and big punctuation
					feature_vector = GetFeatureVector(bd->image + char_pos, char_height, char_width, bd->width);
				}
				else {
					// extract feature for small punctuation
					feature_vector = (double*)malloc(1 * sizeof(double));
				}

				// do some classification (k-nearest neighbors) with the feature vector to get the actual character	

				//free feature vector
				free(feature_vector);
			}
		}
	}
}

/*
*	Parses the entire document image and attempts to segment individual characters
*/
unsigned char* SegmentText(BinaryDocument* bd) {
	int height = bd->height;
	int width = bd->width;
	int* hpp = (int*)malloc(sizeof(int)*height);		// horizontal projection profile for the entire image
	int* vpp = (int*)malloc(sizeof(int)*width);			// vertical projection profile for a single line of text
	int i;
	for (i = 0; i < height; i++) hpp[i] = 0;
	for (i = 0; i < width; i++) vpp[i] = 0;

	//allocate space for mask image
	unsigned char* mask = (unsigned char*)malloc(sizeof(unsigned char) * height * width);
	for (i = 0; i < height*width; i++) {
		mask[i] = 0;
	}

	// populate horizontal projection profile with number of foreground pixels
	int x, y;
	for (y = 0; y < height;	y++) {
		for (x = 0; x < width; x++) {
			if (bd->image[x + y * width] != bd->background_color) {
				hpp[y]++;
			}
		}
	}

	// determine horizontal lines the in the mask
	// spaces between lines are classified as horizontal slices where the % of foreground pixels is less than HOR_THRESHOLD
	//int horz_text_encountered = 0;		// during row analysis, set to one once the program finds an image row that crosses through text
	int text_run_start = 0;				// beginning of run of rows including text
	int text_run_end = 0;
	int in_text_run = 0;						// signals whether in the middle of a current run
	for (y = 0; y < bd->height; y++) {
		double pct_text = (double)hpp[y] / bd->width;

		// find beginning of a run of text
		if (!in_text_run) {
			if (pct_text > HOR_THRESHOLD) {		// find white space in histogram
													// signal beginning of run
				text_run_start = y;
				in_text_run = 1;
			}
		}

		// find the end of a run of text
		else {
			if (pct_text <= HOR_THRESHOLD) {
				in_text_run = 0;
				text_run_end = y - 1;

				// do character segmentation on the row
				// compute vertical projection profile
				for (x = 0; x < bd->width; x++) {
					vpp[x] = 0;
					for (y = text_run_start; y <= text_run_end; y++) {
						if (bd->image[x + y * width] == !bd->background_color) {
							vpp[x]++;
						}
					}
				}
				CharSegment(bd, mask, vpp, text_run_start, text_run_end);		// segment individual characters
				
			}
		}
	}
	return mask;
}

