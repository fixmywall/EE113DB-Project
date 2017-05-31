#ifndef OCR_H
#define OCR_H

#define CHAR_COUNT 53

void TrainCharacter(int* features, char c);

char ClassifyCharacter(int* features);									// classifies the character from its feature vector

double* GetFeatureVector(char* char_pixels, int height, int width, int doc_width);		// returns the feature vector for a character

float BilinearInterpolation(float q11, float q12, float q21, float q22, float x1, float x2, float y1, float y2, float x, float y);

unsigned char* ResizeCharacter(unsigned char* image, int height, int width, int output_height, int output_width, int doc_width);


#endif
