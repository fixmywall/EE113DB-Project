/* 
*	Responsible for segmenting a deskewed binary document into individual characters.
*	After sucessful segmentation, the characters are ready to be passed into the OCR egnine
*/
#include "segment.h"

/*
*	Returns an image with lines corresponding to the gaps between lines and characters
*	When superimposed with the original document, the lines should identify each indvidual character and space.
*/
unsigned char* GetMask(BinaryDocument* bd) {

}