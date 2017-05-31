#ifndef SEGMENT_H
#define SEGMENT_H

#include "preprocess.h"

void CharSegment(BinaryDocument* bd, unsigned char* mask, int* vpp, int min_y, int max_y);
unsigned char* SegmentText(BinaryDocument* bd);

#endif

