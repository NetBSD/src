/*
 * This file is te-go32.h
 */

#define LOCAL_LABELS_DOLLAR 1
#define LOCAL_LABELS_FB 1

#define TARGET_FORMAT "coff-go32"

/* GAS should treat '.align value' as an alignment of 2**value */
#define USE_ALIGN_PTWO

/* these define interfaces */
#include "obj-format.h"
