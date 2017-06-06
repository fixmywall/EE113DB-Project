#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "qdbmp.h"
#include "segment.h"
#include "preprocess.h"
#include "ocr.h"

#define PI 3.1415927

void ResizeCharacterTest() {
	int resized_height = 80;
	int resized_width = 80;

	char* input_file = "data/test_char.bmp";
	BinaryDocument bd = Binarize(input_file);

	UCHAR* resized_character = ResizeCharacter(bd.image, bd.height, bd.width, resized_height, resized_width, bd.width);

	//write output to file
	FILE* fp;
	fp = fopen("data/resized_char.txt", "w");
	int i;
	for (i = 0; i < resized_height * resized_width; i++) {
		fprintf(fp, "%d\n", resized_character[i]);
	}
	fclose(fp);

	free(resized_character);
	BinaryDocument_Free(&bd);
}


void TrainingTest() {
	char* input_file = "data/ocr_training_set.bmp";


	//convert to binary image
	BinaryDocument binary_doc;
	binary_doc = Binarize(input_file);
	int height = binary_doc.height;
	int width = binary_doc.width;

	//deskew the document
	Deskew(&binary_doc);

	unsigned char* mask;

	char class_labels[CHAR_COUNT] = { 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
										'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a',
										'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', '1',
										'2', '3', '4', '5', '6', '7', '8', '9', '0', 'A', 'B', 'C', 'D',
										'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N' };

	TrainTrainingSet(ts, &binary_doc, class_labels, CHAR_COUNT);

	//write output to file
	FILE* fp;
	fp = fopen("data/output.txt", "w");
	int i;
	for (i = 0; i < height*width; i++) {
		fprintf(fp, "%d\n", (int)binary_doc.image[i]);
	}
	fclose(fp);
	printf("Finished writing processed image.\n");

	FILE* fp2;
	fp2 = fopen("data/mask_output.txt", "w");
	for (i = 0; i < height*width; i++) {
		fprintf(fp2, "%d\n", (int)binary_doc.boundaries[i]);
	}
	fclose(fp2);

	WriteTrainingSet(ts);
	FreeDataSet(ts);

	//free allocated memory
	BinaryDocument_Free(&binary_doc);
}

//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
int main(void)
{
	TrainingTest();
	return 0;
}
