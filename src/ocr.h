#ifndef OCR_H
#define OCR_H
#include "binarydocument.h"

#define CHAR_COUNT 53

void TrainCharacter(int* features, char c);

char ClassifyCharacter(int* features);									// classifies the character from its feature vector

int* GetFeatureVector(char* char_pixels, int height, int width);		// returns the feature vector for a character



#endif
