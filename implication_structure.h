// Filename:	implication_strucutre.h
// Author:		Alex Nolan
// Date:		10/4/2018
// Description:	Header file which defines data structures for storing implications

#ifndef IMPLICATION_STRUCTURE
#define IMPLICATION_STRUCTURE

#include <unordered_set>

//a list of implications (msb is value, 1 or 0)
typedef std::unordered_set<uint32_t> ImplicationList;

#endif
