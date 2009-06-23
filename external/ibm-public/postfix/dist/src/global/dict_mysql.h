/*	$NetBSD: dict_mysql.h,v 1.1.1.1 2009/06/23 10:08:45 tron Exp $	*/

#ifndef _DICT_MYSQL_H_INCLUDED_
#define _DICT_MYSQL_H_INCLUDED_

/*++
/* NAME
/*	dict_mysql 3h
/* SUMMARY
/*	dictionary manager interface to mysql databases
/* SYNOPSIS
/*	#include <dict_mysql.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_MYSQL	"mysql"

extern DICT *dict_mysql_open(const char *, int, int);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Scott Cotton
/*	IC Group, Inc.
/*	scott@icgroup.com
/*
/*	Joshua Marcus
/*	IC Group, Inc.
/*	josh@icgroup.com
/*--*/

#endif
