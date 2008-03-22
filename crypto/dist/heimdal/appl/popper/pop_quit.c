/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <popper.h>
__RCSID("$Heimdal: pop_quit.c 1057 1996-11-19 22:48:40Z assar $"
        "$NetBSD: pop_quit.c,v 1.2 2008/03/22 08:36:55 mlelstv Exp $");

/* 
 *  quit:   Terminate a POP session
 */

int
pop_quit (POP *p)
{
    /*  Release the message information list */
    if (p->mlp) free (p->mlp);

    return(POP_SUCCESS);
}
