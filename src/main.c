#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "qdbmp.h"
#include "segment.h"
#include "preprocess.h"
#include "ocr.h"
#include "system.h"

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

	FreeMemory(resized_character);
	BinaryDocument_Free(&bd);
}


void TrainFromFile(DataSet* ts, char* input_file) {
	//convert to binary image
	BinaryDocument binary_doc = Binarize(input_file);
	int height = binary_doc.height;
	int width = binary_doc.width;

	//deskew the document
	Deskew(&binary_doc);

	char* class_labels = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	TrainTrainingSet(ts, &binary_doc, class_labels, CHAR_COUNT);

	////write output to file
	//FILE* fp;
	//fp = fopen("data/output.txt", "w");
	//int i;
	//for (i = 0; i < height*width; i++) {
	//	fprintf(fp, "%d\n", (int)binary_doc.image[i]);
	//}
	//fclose(fp);
	//printf("Finished writing processed image.\n");

	//FILE* fp2;
	//fp2 = fopen("data/mask_output.txt", "w");
	//for (i = 0; i < height*width; i++) {
	//	fprintf(fp2, "%d\n", (int)binary_doc.boundaries[i]);
	//}
	//fclose(fp2);

	//free allocated memory
	BinaryDocument_Free(&binary_doc);
}

void TrainingTest() {
	DataSet* ts = EmptyDataSet();
	TrainFromFile(ts, "data/training_set_tahoma.bmp");
	TrainFromFile(ts, "data/training_set_arial.bmp");
	TrainFromFile(ts, "data/training_set_roboto.bmp");

	WriteTrainingSet(ts);
	FreeDataSet(ts);
}

void OCRTest(char* input_file, int k) {
	//convert to binary image
	BinaryDocument binary_doc = Binarize(input_file);
	int height = binary_doc.height;
	int width = binary_doc.width;

	//deskew the document
	Deskew(&binary_doc);

	DataSet* training_set = InitTrainingSet();
	DataSet* test_set = SegmentText(training_set, &binary_doc, NULL, 0);

	char* output = ClassifyTestSet(training_set, test_set, k);

	//write output to file
	FILE* fp;
	fp = fopen("data/ocr_output.txt", "w");
	int i;
	for (i = 0; i < test_set->Size; i++) {
		fprintf(fp, "%c", output[i]);
	}
	fclose(fp);

	// free allocated memory
	BinaryDocument_Free(&binary_doc);
	FreeDataSet(training_set);
	FreeDataSet(test_set);
	FreeMemory(output);
}

//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
int main(void)
{
#if LCDK == 1
	msc_inti();
	mem_init();
#endif
	
	OCRTest("data/test_set_1.bmp", 3);
	//TrainingTest();
	return 0;
}
