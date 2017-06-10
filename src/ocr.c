#include "ocr.h"
#include "preprocess.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "preprocess.h"
#include "segment.h"
#include "system.h"

static const int RESIZED_CHAR_DIM = 40;		// dimension of resized character image for feature extraction
static const int CHAR_ZONE_COUNT = 16;
static const char* TRAINING_SET_FILE =
#if LCDK == 0
	"data/training_set.bin";
#else
	"C:/ti/OMAPL138_StarterWare_1_10_04_01/build/c674x/cgt_ccs/omapl138/lcdkOMAPL138/usb_host_msc/training_set.bin";
#endif

/******************************************************
*	PRIVATE functions
*	reallocates new block for DataSet's Data member
*	Data is an array of DataPoint pointers
*******************************************************/
void ReallocateDataSet(DataSet* ts) {
	int size = ts->Size;
	int allocated = ts->Allocated;
	if (ts->Allocated == 0) {
		ts->Allocated += TRAINING_SET_ALLOCATE_BLOCK;
		ts->Data = MemAllocate(sizeof(DataPoint*) * TRAINING_SET_ALLOCATE_BLOCK);
	}

	else {
		ts->Allocated += TRAINING_SET_ALLOCATE_BLOCK;

#if LCDK == 0
		ts->Data = realloc(ts->Data, ts->Allocated * sizeof(DataPoint*));
#else	// LCDK has no support for realloc
		DataPoint** new_block = MemAllocate(sizeof(DataPoint*) * ts->Allocated);
		int i;
		for (i = 0; i < ts->Allocated; i++) {
			new_block[i] = ts->Data[i];
		}
		FreeMemory(ts->Data);
		ts->Data = new_block;
#endif
	}
	allocated = ts->Allocated;
}

DataSet* InitTrainingSet() {
	DataSet* ts = MemAllocate(sizeof(DataSet));
	ts->Allocated = 0;
	ts->Size = 0;

	FILE* fp;
	fp = fopen(TRAINING_SET_FILE, "rb");
	if (fp) {			// parse file if it exists
		while (!feof(fp)) {		// parse until end of file
			// parse the file and obtain the feature vectors and class labels for each data point
			// everything is stored contiguously and bytewise (no buffers, newlines, etc)
			double* feature_vector = MemAllocate(sizeof(double) * FEATURE_VECTOR_LENGTH);
			fread(feature_vector, sizeof(double), FEATURE_VECTOR_LENGTH, fp);
			char class_label = (char)fgetc(fp);
			
			DataPoint* dp = NewDataPoint(class_label, feature_vector);

			if (class_label != EOF)
				AddTrainingData(ts, dp);
		}
		fclose(fp);
	}
	return ts;
}


// assumes all DataPoint objects stored in ds are DYNAMICALLY allocated
// and thus stored on the heap
void FreeDataSet(DataSet* ds) {
	if (!ds) return;

	int i;
	for (i = 0; i < ds->Size; i++) {			// free each individual DataPoint object
		if (ds->Data[i]->FeatureVector)		FreeMemory(ds->Data[i]->FeatureVector);
		if (ds->Data[i])					FreeMemory(ds->Data[i]);
	}
	FreeMemory(ds);		// free the entire DataSet at the very end
}

// allocates and returns an empty DataSet object
DataSet* EmptyDataSet() {
	DataSet* ds = (DataSet*)MemAllocate(sizeof(DataSet));
	ds->Allocated = 0;
	ds->Size = 0;

	return ds;
}

// allocates and returns a DataPoint object
// feature_vector: array of features that must exist on the heap before passed
DataPoint* NewDataPoint(char class_label, double* feature_vector) {
	DataPoint* td = MemAllocate(sizeof(DataPoint));
	td->ClassLabel = class_label;
	td->FeatureVector = feature_vector;
	return td;
}

void AddTrainingData(DataSet* ts, DataPoint* td) {
	int size = ts->Size;
	if (ts->Size == ts->Allocated) {		// if size has reached allocated limit, reallocate
		ReallocateDataSet(ts);
	}
	ts->Data[ts->Size] = td;
	ts->Size++;
	size =ts->Size;
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


/**********************************************************************************
*	Classifies a test set using data from training set "train" using K-nearest neighbors
*	The test set will contain points with both null and non-null labels.
*	This algorithm will only use the points with null labels for Classification. The rest 
*	of the points will have their class label automatically output
*
*	train: training set
*	test: test set
**********************************************************************************/
char* ClassifyTestSet(DataSet* train, DataSet* test, int k) {
	int test_size = test->Size;
	char* output = MemAllocate(sizeof(char) * test_size);

	int i;
	for (i = 0; i < test_size; i++) {
		DataPoint* test_point = test->Data[i];
		char output_char;
		if (test_point->ClassLabel == '\0') {		// only classify test poinnts with null labels
			output_char = ClassifyDataPoint(train, test_point, k);
		}
		
		else {
			output_char = test_point->ClassLabel;
		}
		output[i] = output_char;
	}
	return output;
}


/*****************************************************************************
*	Neighbor struct and accompanying functions are only used for the purposes
*	of function ClassifyDataPoint()
*****************************************************************************/
typedef struct {
	double DistSquared;
	char ClassLabel;
} Neighbor;			// struct containing a neighbor's distance squared and class label

int CompareNeighbor(Neighbor* a, Neighbor* b) {			// for qsort (increasing order)
	if (a->DistSquared < b->DistSquared) {
		return -1;
	}
	else if (a->DistSquared > b->DistSquared) {
		return 1;
	}
	else return 0;
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
	if (!ts || ! dp || ts->Size == 0 || k <= 0) return '\0';

	int i;
	Neighbor* neighbor_vector;			// vector of neighbor structs for ALL datapoints in ts
	neighbor_vector = MemAllocate(sizeof(Neighbor) * ts->Size);
	for (i = 0; i < ts->Size; i++) {	// iterate through all datapoints in training set
		DataPoint* train_point = ts->Data[i];

		double dist_squared = 0;
		int j;
		for (j = 0; j < FEATURE_VECTOR_LENGTH; j++) {
			dist_squared += pow(dp->FeatureVector[j] - train_point->FeatureVector[j], 2.0);
		}
		Neighbor neighbor;
		neighbor.ClassLabel = train_point->ClassLabel;
		neighbor.DistSquared = dist_squared;
		neighbor_vector[i] = neighbor;
	}

	// sort neighbor vector in order of increasing distance squared
	qsort(neighbor_vector, ts->Size, sizeof(Neighbor), CompareNeighbor);

	// find the most frequent class of the K-nearest ones
	int votes[255];				// contains vote count for the class of the K-nearest neighbors
	for (i = 0; i < 255; i++) {
		votes[i] = 0;
	}
	for (i = 0; i < k; i++) {
		char class_label = neighbor_vector[i].ClassLabel;
		votes[(int)class_label]++;
	}
	int max = votes[0];
	char max_char = neighbor_vector[0].ClassLabel;		// most frequent class in K-nearest neighbors
	for (i = 0; i < k; i++) {		// get character with the most frequent votes
		char class_label = neighbor_vector[i].ClassLabel;
		int votes_i = votes[(int)class_label];
		if (votes_i > max) {
			max = votes_i;
			max_char = class_label;
		}
	}
	
	FreeMemory(neighbor_vector);
	return max_char;
}

/*	takes in a GRAYSCALE (8 bpp) image of a character and computes the feature vector
*	feature vector is determined by dividing the resized image into 16 zones
*	and computing the average pixel value in those zones
*/
double* GetFeatureVector(unsigned char* char_pixels, int height, int width, int doc_width) {
	if (height == 0 || width == 0) return;
	unsigned char* resized_image = ResizeCharacter(char_pixels, height, width, RESIZED_CHAR_DIM, RESIZED_CHAR_DIM, doc_width);
	//WriteToFile("data/resized_char.txt", resized_image, 40, 40);
	double* feature_vector = (double*)MemAllocate(sizeof(double) * CHAR_ZONE_COUNT);
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
	FreeMemory(resized_image);

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
	if (out_height == 0 || out_width == 0) return NULL;

	unsigned char* output_image = MemAllocate(sizeof(unsigned char) * out_height * out_width);

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
