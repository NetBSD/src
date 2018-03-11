/*	$NetBSD: libmain.c,v 1.4 2018/03/11 18:32:10 christos Exp $	*/

/* libmain - flex run-time support library "main" function */

/*  This file is part of flex. */

/*  Redistribution and use in source and binary forms, with or without */
/*  modification, are permitted provided that the following conditions */
/*  are met: */

/*  1. Redistributions of source code must retain the above copyright */
/*     notice, this list of conditions and the following disclaimer. */
/*  2. Redistributions in binary form must reproduce the above copyright */
/*     notice, this list of conditions and the following disclaimer in the */
/*     documentation and/or other materials provided with the distribution. */

/*  Neither the name of the University nor the names of its contributors */
/*  may be used to endorse or promote products derived from this software */
/*  without specific prior written permission. */

/*  THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR */
/*  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR */
/*  PURPOSE. */
#ifndef HAVE_NBTOOL_CONFIG_H
#include <sys/cdefs.h>
#ifdef __RCSID
__RCSID("$NetBSD: libmain.c,v 1.4 2018/03/11 18:32:10 christos Exp $");
#endif
#endif

#include <stdlib.h>

extern int yylex (void);

int     main (int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	while (yylex () != 0) ;

	exit(0);
}
