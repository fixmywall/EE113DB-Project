#ifndef OCR_H
#define OCR_H

#define CHAR_COUNT 62
#define FEATURE_VECTOR_LENGTH 16
#define TRAINING_SET_ALLOCATE_BLOCK 200

#include "preprocess.h"

typedef struct _DataPoint {
	char ClassLabel;
	double* FeatureVector;
} DataPoint;

typedef struct _DataSet {
	int Allocated;
	int Size;
	DataPoint** Data;		// array of TrainingData pointers
} DataSet;

DataSet* InitTrainingSet();

DataSet* EmptyDataSet();

void FreeDataSet(DataSet* ds);

DataPoint* NewDataPoint(char class_label, double* feature_vector);

void TrainTrainingSet(DataSet* ts, BinaryDocument* bd, char* class_labels, int num_labels);

void WriteTrainingSet(DataSet* ts);

void AddTrainingData(DataSet* ts, DataPoint* td);

double* GetFeatureVector(unsigned char* char_start, int height, int width, int doc_width);		// returns the feature vector for a character

char* ClassifyTestSet(DataSet* train, DataSet* test, int k);

char ClassifyDataPoint(DataSet* ts, DataPoint* dp, int k);

float BilinearInterpolation(float q11, float q12, float q21, float q22, float x1, float x2, float y1, float y2, float x, float y);

unsigned char* ResizeCharacter(unsigned char* image, int height, int width, int output_height, int output_width, int doc_width);


#endif
