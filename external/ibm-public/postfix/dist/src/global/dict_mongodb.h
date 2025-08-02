/*	$NetBSD: dict_mongodb.h,v 1.2.4.2 2025/08/02 05:50:06 perseant Exp $	*/

#ifndef _DICT_MONGODB_INCLUDED_
#define _DICT_MONGODB_INCLUDED_

/*++
/* NAME
/*	dict_mongodb 3h
/* SUMMARY
/*	dictionary interface to mongodb databases
/* SYNOPSIS
/*	#include <dict_mongodb.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_MONGODB "mongodb"

extern DICT *dict_mongodb_open(const char *, int, int);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Hamid Maadani (hamid@dexo.tech)
/*	Dextrous Technologies, LLC
/*
/*	Edited by:
/*	Wietse Venema
/*	porcupine.org
/*
/*	Based on prior work by:
/*	Stephan Ferraro
/*	Aionda GmbH
/*--*/

#endif
