/*	$Id: m68881-ext.c,v 1.1 1994/01/28 12:41:45 pk Exp $ */
#include "ieee-float.h"

CONST struct ext_format ext_format_68881 = {
/* tot sbyte smask expbyte manbyte */
   12, 0,    0x80, 0,1,	   4,8		/* mc68881 */
};
