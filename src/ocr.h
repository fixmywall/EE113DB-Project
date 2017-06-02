#ifndef OCR_H
#define OCR_H

#define CHAR_COUNT 62
#define FEATURE_VECTOR_LENGTH 16
#define TRAINING_SET_ALLOCATE_BLOCK 100

typedef struct _TrainingData {
	char ClassLabel;
	double* FeatureVector;
} TrainingData;

typedef struct _TrainingSet {
	int Allocated;
	int Size;
	TrainingData* Data;
} TrainingSet;

char class_labels[CHAR_COUNT];

TrainingSet InitTrainingSet();

void WriteTrainingSet(TrainingSet* ts);

void AddTrainingData(TrainingSet* ts, TrainingData* td);

void ReallocateTrainingSet(TrainingSet* ts);

double* GetFeatureVector(char* char_start, int height, int width, int doc_width);		// returns the feature vector for a character

float BilinearInterpolation(float q11, float q12, float q21, float q22, float x1, float x2, float y1, float y2, float x, float y);

unsigned char* ResizeCharacter(unsigned char* image, int height, int width, int output_height, int output_width, int doc_width);


#endif
