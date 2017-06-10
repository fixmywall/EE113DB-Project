/* 
*	Responsible for segmenting a deskewed binary document into individual characters.
*	After sucessful segmentation, the characters are ready to be passed into the OCR egnine
*/
#include "segment.h"
#include "ocr.h"
#include "preprocess.h"
#include "system.h"

/*
*	Returns an image with lines corresponding to the gaps between lines and characters
*	When superimposed with the original document, the lines should identify each indvidual character and space.
*/

static const double HOR_THRESHOLD = 0.003;			//horizontal slice considered a space if proportion of foreground pixels <= this
static const double VERT_THRESHOLD = 0;
static const double PUNCTUATION_THRESHOLD = 0.2;	//if the proportion of height of the character to the line width is below this, classify as a punctuation symbol
static const double SPACE_THRESHOLD = 0.6;			// if gap larger than this times avg char width, classify gap as a space

static int total_char_width = 0;	// running sum of the width of the segmented characters
static double avg_char_width = 0;	// running average of the width of the segmented characters
/*
*	min_y:	lowest row that contains text pixels
*	max_y:	highest row that contains text pixels
*	Segments characters from the line specified by parameters min_y and max_y and performs feature extraction on
*	them
*/
void CharSegment(	DataSet* test_set, DataSet* ts, BinaryDocument* bd, unsigned char* mask, int* vpp, int min_y,
					int max_y, char* labels, int* char_index, int max_labels) {
	int width = bd->width;
	int line_height = max_y - min_y + 1;
	if (line_height == 0) return;

	int char_min_x = 0;		// pixel left of beginning of run of text
	int char_max_x = 0;		// end of run of text
	int text_found = 0;		// set to 1 once first characcter in the line is found
	int in_text_run = 0;
	
	int x, y;
	for (x = 0; x < bd->width; x++) {
		double pct_text = (double)vpp[x] / line_height;

		// find beginning of a run of text
		if (!in_text_run) {
			if ( pct_text > VERT_THRESHOLD) {		// find text in histogram

				// signal beginning of run
				if (!text_found) {
					text_found = 1;
				}
				else {	// try to see if space between this and previous character
					int horiz_gap = x - char_max_x;
					if (horiz_gap >= SPACE_THRESHOLD * avg_char_width) {
						// create space character
						DataPoint* space = NewDataPoint(' ', NULL);
						AddTrainingData(test_set, space);
					}
				}

				in_text_run = 1;
				char_min_x = x - 1;		

				// if the previous char_max_x is near to the average character width, classify as a space

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

				double* feature_vector;
				
				/*	If the segmented character is classified as punctuation (period, etc.)*/
				if (is_small_punct) {		
					feature_vector = (double*)MemAllocate(1 * sizeof(double));
				}

				/*	If the segmented character is classified as a regular alphanumeric character	*/
				else {	
					// extract feature for small punctuation
					feature_vector = GetFeatureVector(bd->image + char_pos, char_height, char_width, bd->width);
					
					// figure out if point is training data
					int isTrainingData = 0;
					if (*char_index < max_labels) {
						isTrainingData = 1;
					}
					else {
						isTrainingData = 0;
					}

					// create and add a training data object if the current character is part of the training set
					if (isTrainingData) {
						char training_label = labels[*char_index];
						DataPoint* training_data = NewDataPoint(training_label, feature_vector);
						int size = ts->Size;
						int allocated = ts->Allocated;
						AddTrainingData(ts, training_data);
					}

					// otherwise, the data object is part of the test set 
					// store the feature vector in a dataset to perform KNN classification on later
					else {
						DataPoint* dp = NewDataPoint((char)0, feature_vector);	// use null char to signify points that have not been classified yet
						AddTrainingData(test_set, dp);
					}

					(*char_index)++;

					// get running sum of widths
					total_char_width += char_width;
					avg_char_width = total_char_width / (*char_index);
				}
			}
		}
	}
}

/*
*	Parses the entire document image and attempts to segment individual characters
*/
DataSet* SegmentText(DataSet* training, BinaryDocument* bd, char* symbols, int num_symbols) {
	avg_char_width = 0;
	total_char_width = 0;

	DataSet* output_set = EmptyDataSet();
	int foo = training->Size;
	int char_index = 0;

	int height = bd->height;
	int width = bd->width;
	int* hpp = (int*)MemAllocate(sizeof(int)*height);		// horizontal projection profile for the entire image
	int* vpp = (int*)MemAllocate(sizeof(int)*width);			// vertical projection profile for a single line of text
	int i;
	for (i = 0; i < height; i++) hpp[i] = 0;
	for (i = 0; i < width; i++) vpp[i] = 0;

	//allocate space for mask image
	unsigned char* mask = (unsigned char*)MemAllocate(sizeof(unsigned char) * height * width);
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
				CharSegment(	output_set, training, bd, mask, vpp, text_run_start, 
								text_run_end, symbols, &char_index, num_symbols);		// segment individual characters
				// insert newline character 
				DataPoint* new_line = NewDataPoint('\n', NULL);
				AddTrainingData(output_set, new_line);

			}
		}
	}
	bd->boundaries = mask;

	return output_set;
}

