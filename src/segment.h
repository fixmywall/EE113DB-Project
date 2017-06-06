#ifndef SEGMENT_H
#define SEGMENT_H

#include "preprocess.h"
#include "ocr.h"

DataSet* CharSegment(	DataSet* ts, BinaryDocument* bd, unsigned char* mask, int* vpp, int min_y,
							int max_y, char* labels, int* char_index, int max_labels);

void SegmentText( DataSet* ts, BinaryDocument* bd, char* labels, int num_labels);

#endif

