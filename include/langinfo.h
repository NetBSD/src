/*
 * Copyright (c) 1994 Winning Strategies, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Winning Strategies, Inc.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LANGINFO_H_
#define _LANGINFO_H_
#include <sys/cdefs.h>

#define D_T_FMT		0	/* String for formatting date and time */
#define D_FMT		1	/* Date format string */
#define	T_FMT		2	/* Time format string */
#define T_FMT_AMPM	3	/* Time format string with 12 hour clock */
#define AM_STR		4	/* Ante Meridiem afix */
#define PM_STR		5	/* Post Meridiem afix */

#define DAY_1		6	/* Name of the first day of the week */
#define DAY_2		7
#define DAY_3		8
#define DAY_4		9
#define DAY_5		10
#define DAY_6		11
#define DAY_7		12

#define ABDAY_1		13	/* Abbrev. name of the first day of the week */
#define ABDAY_2		14
#define ABDAY_3		15
#define ABDAY_4		16
#define ABDAY_5		17
#define ABDAY_6		18
#define ABDAY_7		19

#define MON_1		20	/* Name of the first month */
#define MON_2		21
#define MON_3		22
#define MON_4		23
#define MON_5		24
#define MON_6		25
#define MON_7		26
#define MON_8		27
#define MON_9		28
#define MON_10		29
#define MON_11		30
#define MON_12		31

#define ABMON_1		32	/* Abbrev. name of the first month */
#define ABMON_2		33
#define ABMON_3		34
#define ABMON_4		35
#define ABMON_5		36
#define ABMON_6		37
#define ABMON_7		38
#define ABMON_8		39
#define ABMON_9		40
#define ABMON_10	41
#define ABMON_11	42
#define ABMON_12	43

#define RADIXCHAR	44	/* Radix character */
#define THOUSEP		45	/* Separator for thousands */
#define YESSTR		46	/* Affirmitive response for yes/no queries */
#define YESEXPR		47	/* Affirmitive response for yes/no queries */
#define NOSTR		48	/* Negative response for yes/no queries */
#define NOEXPR		49	/* Negative response for yes/no queries */
#define CRNCYSTR	50	/* Currency symbol */

__BEGIN_DECLS
char *nl_langinfo __P((nl_item));
__END_DECLS

#endif	/* _LANGINFO_H_ */
