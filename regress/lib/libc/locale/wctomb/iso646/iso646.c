/*	$Id: iso646.c,v 1.1 2004/09/26 03:45:10 yamt Exp $	*/

const char teststring[] =
	"ABC"
	;
const int teststring_wclen = 3;
const char teststring_loc[] = "C";
const int teststring_mblen[] = { 1, 1, 1, 1 };

#include "test.c"
