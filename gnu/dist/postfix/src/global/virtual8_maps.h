#ifndef _VIRTUAL8_MAPS_H_INCLUDED_
#define _VIRTUAL8_MAPS_H_INCLUDED_

/*++
/* NAME
/*	virtual8_maps 3h
/* SUMMARY
/*	virtual delivery agent compatibility
/* SYNOPSIS
/*	#include <virtual8_maps.h>
/* DESCRIPTION
/* .nf

 /*
  * Global library.
  */
#include <maps.h>

 /*
  * External interface.
  */
#define virtual8_maps_create(title, map_names, flags) \
	maps_create((title), (map_names), (flags))
extern const char *virtual8_maps_find(MAPS *, const char *);
#define virtual8_maps_free(maps)	maps_free((maps))

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
