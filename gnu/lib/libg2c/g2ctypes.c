/*	$NetBSD: g2ctypes.c,v 1.1 1998/08/18 17:25:33 tv Exp $	*/

/*
 * Not actually C; just fed through the preprocessor.
 */

#define TREE_CODE 1
#include "proj.h"
#define FFECOM_DETERMINE_TYPES 1
#include "com.h"

#if FFECOM_f2cINTEGER == FFECOM_f2ccodeLONG
s/@F2C_INTEGER@/long int/g
#elif FFECOM_f2cINTEGER == FFECOM_f2ccodeINT
s/@F2C_INTEGER@/int/g
#else
#  error "Cannot find a suitable type for F2C_INTEGER"
#endif

#if FFECOM_f2cLONGINT == FFECOM_f2ccodeLONG
s/@F2C_LONGINT@/long int/g
#elif FFECOM_f2cLONGINT == FFECOM_f2ccodeLONGLONG
s/@F2C_LONGINT@/long long int/g
#else
#  error "Cannot find a suitable type for F2C_LONGINT"
#endif
