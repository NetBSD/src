#ifndef _NAME_MASK_H_INCLUDED_
#define _NAME_MASK_H_INCLUDED_

/*++
/* NAME
/*	name_mask 3h
/* SUMMARY
/*	map names to bit mask
/* SYNOPSIS
/*	#include <name_mask.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
typedef struct {
    const char *name;
    int     mask;
} NAME_MASK;

extern int name_mask(const char *, NAME_MASK *, const char *);
extern const char *str_name_mask(const char *, NAME_MASK *, int);

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
