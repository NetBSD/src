/*	$NetBSD: g2ctypes.c,v 1.2 1998/09/13 20:32:04 tv Exp $	*/

/*
 * Not actually C; just fed through the preprocessor.
 */

#define TREE_CODE 1
#include "proj.h"
#include "tconfig.j"
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
