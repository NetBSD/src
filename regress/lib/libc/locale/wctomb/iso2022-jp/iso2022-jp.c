/*	$Id: iso2022-jp.c,v 1.1 2004/09/26 03:45:10 yamt Exp $	*/

const char teststring[] =
	"\x1b$B"	/* JIS X 0208-1983 */
	"\x46\x7c\x4b\x5c\x38\x6c" /* "nihongo" */
	"\x1b(B"	/* ISO 646 */
	"ABC"
	"\x1b(I"	/* JIS X 0201 katakana */
	"\xb1\xb2\xb3"	/* "aiu" */
	"\x1b(B"	/* ISO 646 */
	;
const int teststring_wclen = 3 + 3 + 3;
const char teststring_loc[] = "ja_JP.ISO2022-JP";
const int teststring_mblen[] = { 3+2, 2, 2, 3+1, 1, 1, 3+1, 1, 1, 3+1 };

#include "test.c"
