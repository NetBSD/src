/*	$NetBSD: mock_dict.h,v 1.2 2026/05/09 18:49:21 christos Exp $	*/

#ifndef _MOCK_DICT_H_INCLUDED_
#define _MOCK_DICT_H_INCLUDED_

/*++
/* NAME
/*	mock_dict 3h
/* SUMMARY
/*	mock tables
/* SYNOPSIS
/*	#include <mock_dict.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern void setup_mock_cdb(const char *);
extern void setup_mock_lmdb(const char *);

/* LICENSE
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
