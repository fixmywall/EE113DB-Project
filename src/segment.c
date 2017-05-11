/* 
*	Responsible for segmenting a deskewed binary document into individual characters.
*	After sucessful segmentation, the characters are ready to be passed into the OCR egnine
*/
#include "segment.h"

/*
*	Returns an image with lines corresponding to the gaps between lines and characters
*	When superimposed with the original document, the lines should identify each indvidual character and space.
*/

static const double HOR_THRESHOLD = 0.02;		//horizontal slice considered a space if proportion of foreground pixels <= this
static const double VERT_THRESHOLD = 0;
static const int PIXEL_BUFFER = 1;

/*
*
*/
void VerticalSegmentation(BinaryDocument* bd, unsigned char* mask, int* vpp, int min_y, int max_y) {
	int width = bd->width;
	int line_height = max_y - min_y + 1;

	if (line_height == 0) return;

	int text_run_start = 0;		// beginning of run of text
	int text_run_end = 0;		// end of run of text
	int in_text_run = 0;
	
	int x, y;
	for (x = 0; x < bd->width; x++) {
		double pct_text = (double)vpp[x] / line_height;

		// find beginning of a run of text
		if (!in_text_run) {
			if ( pct_text > VERT_THRESHOLD) {		// find text in histogram
				// signal beginning of run
				text_run_start = x - PIXEL_BUFFER;
				if (text_run_start < 0) text_run_start = 0;
				in_text_run = 1;
			}
		}

		// find the end of a run of text
		else {			
			if (pct_text <= VERT_THRESHOLD) {			
				in_text_run = 0;
				
				text_run_end = x + PIXEL_BUFFER;
				if (text_run_end >= width) text_run_end = width - 1;

				// draw vertical lines surrounding character
				for (y = min_y; y <= max_y; y++) {
					mask[text_run_start + y * width] = 1;
					mask[text_run_end + y * width] = 1;
				}
			}
		}
	}
}

unsigned char* GetMask(BinaryDocument* bd) {
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
				text_run_start = y - PIXEL_BUFFER;
				if (text_run_start < 0) text_run_start = 0;

				in_text_run = 1;
			}
		}

		// find the end of a run of text
		else {
			if (pct_text <= HOR_THRESHOLD) {
				in_text_run = 0;
				text_run_end = y + PIXEL_BUFFER;
				if (text_run_end >= bd->height) text_run_end = height - 1;		// boundary check


				// draw horizontal line
				for (x = 0; x < bd->width; x++) {
					mask[x + text_run_start * width] = 1;
					mask[x + text_run_end * width] = 1;
				}

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
				VerticalSegmentation(bd, mask, vpp, text_run_start, text_run_end);
				
			}
		}
	}
	return mask;
}

