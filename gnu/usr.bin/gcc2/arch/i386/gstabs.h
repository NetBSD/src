/*	$Id: gstabs.h,v 1.1.2.2 1993/08/23 09:21:48 cgd Exp $ */

#include "i386/gas.h"

/* We do not want to output SDB debugging information.  */

#undef SDB_DEBUGGING_INFO

/* We want to output DBX debugging information.  */

#define DBX_DEBUGGING_INFO
