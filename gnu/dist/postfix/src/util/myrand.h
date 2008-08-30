#ifndef _MYRAND_H_INCLUDED_
#define _MYRAND_H_INCLUDED_

/*++
/* NAME
/*	myrand 3h
/* SUMMARY
/*	rand wrapper
/* SYNOPSIS
/*	#include <myrand.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
#ifndef RAND_MAX
#define RAND_MAX 0x7fffffff
#endif

extern void mysrand(int);
extern int myrand(void);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#endif
