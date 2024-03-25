/*	$NetBSD: msg_356.c,v 1.3 2024/03/25 22:37:43 rillig Exp $	*/
# 3 "msg_356.c"

// Test for message: short octal escape '%.*s' followed by digit '%c' [356]

/* lint1-extra-flags: -X 351 */

// When counting backwards in octal, the number before \040 is not \039 but
// \037. This mistake sometimes happens when encoding the bit numbers for
// snprintb(3) format conversions.

char snprintb_fmt[] = "\020"
    "\0040bit32"		// 3-digit octal escapes are fine
    "\0039bit31"
    "\0038bit30"

    "\040bit32"
    /* expect+1: warning: short octal escape '\03' followed by digit '9' [356] */
    "\039bit31"
    /* expect+1: warning: short octal escape '\03' followed by digit '8' [356] */
    "\038bit30"

    "\40bit32"
    /* expect+1: warning: short octal escape '\3' followed by digit '9' [356] */
    "\39bit31"
    /* expect+1: warning: short octal escape '\3' followed by digit '8' [356] */
    "\38bit30"
    "\37bit29"
;

char ok[] = ""
    "\3\70"			// short octal followed by escaped '8'
    "\3\x38"			// short octal followed by escaped '8'
    "\3" "8"			// short octal and '8' are separated
;
