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

/*
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
*/

unsigned char* ReadBMP(char* file_name, int* height, int* width) {
	unsigned char* out_image;
#if LCDK == 0
	BMP* bmp;
	bmp = BMP_ReadFile(file_name);

	if (BMP_GetError() != BMP_OK) {
		return NULL;
	}
	*width = BMP_GetWidth(bmp);
	*height = BMP_GetHeight(bmp);
	out_image = BMP_GetData(bmp);
#else
	out_image = usb_imread(file_name);
	*width = InfoHeader.Width;
	*height = InfoHeader.Height;
#endif
	return out_image;
}


void OCRTest(int k, int write) {
	unsigned char* image_rgb;
	char* buffer;
	int length;
	int i;
	int height, width;
	char file_name[40];

	do {
		puts("Name of file to scan:");
		char buffer[30];

		if (fgets(buffer, 30, stdin) != NULL)
		{
			strtok(buffer, "\n");			// remove trailing newline
			strcpy(file_name, "data/");
			strcat(file_name, buffer);
		}
		image_rgb = ReadBMP(file_name, &height, &width);


		if (image_rgb == NULL) {		// if error reading input image
			puts("File not found");
		}
	} while (image_rgb == NULL);

	//convert to binary image
	BinaryDocument bd = Binarize(image_rgb, height, width);

	//deskew the document
	Deskew(&bd);

	DataSet* training_set = InitTrainingSet();
	DataSet* test_set = SegmentText(training_set, &bd, NULL, 0);
	char* output = ClassifyTestSet(training_set, test_set, k);

	puts("Output:");
	printf("%s\n", output);

	if (write) {
		//write output to file
		FILE* fp3;
		fp3 = fopen("data/output.txt", "w");
		for (i = 0; i < height*width; i++) {
			fprintf(fp3, "%d\n", (int)bd.image[i]);
		}
		fclose(fp3);

		FILE* fp2;
		fp2 = fopen("data/mask_output.txt", "w");
		for (i = 0; i < height*width; i++) {
			fprintf(fp2, "%d\n", (int)bd.boundaries[i]);
		}
		fclose(fp2);

		//write output to file
		FILE* fp;
		fp = fopen("data/ocr_output.txt", "w");
		for (i = 0; i < test_set->Size; i++) {
			fprintf(fp, "%c", output[i]);
		}
		fclose(fp);
	}



	// free allocated memory
	BinaryDocument_Free(&bd);
	FreeDataSet(training_set);
	FreeDataSet(test_set);
	FreeMemory(output);
}


void TrainFromFile(DataSet* ts, char* input_file) {
	//convert to binary image
	int height, width;

	unsigned char* bmp_rgb = ReadBMP(input_file, &height, &width);


	BinaryDocument binary_doc = Binarize(bmp_rgb, height, width);

	//deskew the document
	//Deskew(&binary_doc);

	char* class_labels = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz?!";

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
	int size;
	TrainFromFile(ts, "data/training/tahoma.bmp");
	TrainFromFile(ts, "data/training/verdana.bmp");
	TrainFromFile(ts, "data/training/roboto.bmp");
	TrainFromFile(ts, "data/training/arial.bmp");
	TrainFromFile(ts, "data/training/calibri.bmp");
	TrainFromFile(ts, "data/training/open_sans.bmp");

	WriteTrainingSet(ts);
	FreeDataSet(ts);
}


//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
int
main(void)
{
#if LCDK == 1
	msc_inti();
	mem_init();
#endif

	//TrainingTest();
	OCRTest(3, 1);
}
