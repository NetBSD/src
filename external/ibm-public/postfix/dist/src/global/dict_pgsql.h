/*	$NetBSD: dict_pgsql.h,v 1.1.1.1.2.2 2009/09/15 06:02:40 snj Exp $	*/

#ifndef _DICT_PGSQL_INCLUDED_
#define _DICT_PGSQL_INCLUDED_

/*++
/* NAME
/*	dict_pgsql 3h
/* SUMMARY
/*	dictionary manager interface to Postgresql files
/* SYNOPSIS
/*	#include <dict_pgsql.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_PGSQL "pgsql"

extern DICT *dict_pgsql_open(const char *name, int unused_flags, int dict_flags);

/* AUTHOR(S)
/*	Aaron Sethman
/*	androsyn@ratbox.org
/*
/*	Based upon dict_mysql.c by
/*
/*	Scott Cotton
/*	IC Group, Inc.
/*	scott@icgroup.com
/*
/*	Joshua Marcus
/*	IC Group, Inc.
/*	josh@icgroup.com
/*--*/

#endif
