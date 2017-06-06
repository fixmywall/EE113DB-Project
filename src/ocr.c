#include "ocr.h"
#include "preprocess.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "preprocess.h"

static const int RESIZED_CHAR_DIM = 40;		// dimension of resized character image for feature extraction
static const int CHAR_ZONE_COUNT = 16;
static const char* TRAINING_SET_FILE = "data/training_set.bin";

DataSet* InitTrainingSet() {
	DataSet* ts = malloc(sizeof(DataSet));
	ts->Allocated = 0;
	ts->Size = 0;

	FILE* fp;
	fp = fopen(TRAINING_SET_FILE, "rb");
	if (fp) {			// parse file if it exists
		while (!feof(fp)) {		// parse until end of file
			// parse the file and obtain the feature vectors and class labels for each data point
			// everything is stored contiguously and bytewise (no buffers, newlines, etc)
			double* feature_vector = malloc(sizeof(double) * FEATURE_VECTOR_LENGTH);
			char class_label = '\0';
			fread(feature_vector, sizeof(double), FEATURE_VECTOR_LENGTH, fp);
			fread(&class_label, sizeof(char), 1, fp);
			
			DataPoint* dp = NewDataPoint(class_label, feature_vector);
			AddTrainingData(ts, dp);
		}
		fclose(fp);
	}
	return ts;
}


// assumes all DataPoint objects stored in ds are DYNAMICALLY allocated
// and thus stored on the heap
void FreeDataSet(DataSet* ds) {
	int i;
	for (i = 0; i < ds->Size; i++) {			// free each individual DataPoint object
		free(ds->Data[i]->FeatureVector);
		free(ds->Data[i]);
	}
	free(ds);		// free the entire DataSet at the very end
}

// allocates and returns an empty DataSet object
DataSet* EmptyDataSet() {
	DataSet* ds = malloc(sizeof(DataSet));
	ds->Allocated = 0;
	ds->Size = 0;

	return ds;
}

// allocates and returns a DataPoint object
// feature_vector: array of features that must exist on the heap before passed
DataPoint* NewDataPoint(char class_label, double* feature_vector) {
	DataPoint* td = malloc(sizeof(DataPoint));
	td->ClassLabel = class_label;
	td->FeatureVector = feature_vector;
	return td;
}

void AddTrainingData(DataSet* ts, DataPoint* td) {
	if (ts->Size == ts->Allocated) {		// if size has reached allocated limit, reallocate
		if (ts->Allocated == 0) {
			ts->Allocated += TRAINING_SET_ALLOCATE_BLOCK;
			ts->Data = malloc(sizeof(DataPoint*) * TRAINING_SET_ALLOCATE_BLOCK);
		}

		else {
			ts->Allocated += TRAINING_SET_ALLOCATE_BLOCK;
			ts->Data = realloc(ts->Data, ts->Allocated * sizeof(DataPoint*));
		}
	}
	ts->Data[ts->Size] = td;
	ts->Size++;
}

// trains the training set referenced by ts using the input Binary Document and class labels
void TrainTrainingSet(DataSet* ts, BinaryDocument* bd, char* class_labels, int num_labels) {
	SegmentText(ts, bd, class_labels, num_labels);
}

// writes training set to a text file
void WriteTrainingSet(DataSet* ts) {
	FILE* fp;
	fp = fopen(TRAINING_SET_FILE, "wb");
	int i, j;
	for (i = 0; i < ts->Size; i++) {
		fwrite(ts->Data[i]->FeatureVector, sizeof(double), FEATURE_VECTOR_LENGTH, fp);
		fwrite(&(ts->Data[i]->ClassLabel), sizeof(char), 1, fp);
	}
	fclose(fp);
}

/******************************************************************************
*	classifies data point using the K-nearest neighbors algorithm and 
*	returns the label of the resulting class
*
*	ts: pointer to the training set to classify from
*	dp: pointer to data point object
*	k: parameter for K-nearest neighbors classification
*******************************************************************************/
char ClassifyDataPoint(DataSet* ts, DataPoint* dp, int k) {
	
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
	int zone_pixel_count = zone_length * zone_length;		// number of pixels in each zone

	int x, y;
	for (y = 0; y < RESIZED_CHAR_DIM; y++) {
		for (x = 0; x < RESIZED_CHAR_DIM; x++) {
			int normalized_pix = resized_image[x + y * RESIZED_CHAR_DIM];
			int zone_index = (x / zone_length) + (y / zone_length) * zone_count_width;
			
			if (normalized_pix == 0) {
				feature_vector[zone_index] += 1.0 / zone_pixel_count;		// add the density contribution of a single dark pixel
			}
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