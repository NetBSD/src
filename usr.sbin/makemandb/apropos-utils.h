/*	$NetBSD: apropos-utils.h,v 1.5 2013/01/14 21:26:25 christos Exp $	*/
/*-
 * Copyright (c) 2011 Abhinav Upadhyay <er.abhinav.upadhyay@gmail.com>
 * All rights reserved.
 *
 * This code was developed as part of Google's Summer of Code 2011 program.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef APROPOS_UTILS_H
#define APROPOS_UTILS_H

#include "sqlite3.h"

#define MANCONF "/etc/man.conf"
#define SECMAX 9

/* Flags for opening the database */
#define MANDB_READONLY SQLITE_OPEN_READONLY
#define MANDB_WRITE SQLITE_OPEN_READWRITE
#define MANDB_CREATE SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE

#define APROPOS_SCHEMA_VERSION 20120507

/*
 * Used to identify the section of a man(7) page.
 * This is similar to the enum mdoc_sec defined in mdoc.h from mdocml project.
 */
enum man_sec {
	MANSEC_NAME = 0,
	MANSEC_SYNOPSIS,
	MANSEC_LIBRARY,
	MANSEC_ERRORS,
	MANSEC_FILES,
	MANSEC_RETURN_VALUES,
	MANSEC_EXIT_STATUS,
	MANSEC_DESCRIPTION,
	MANSEC_ENVIRONMENT,
	MANSEC_DIAGNOSTICS,
	MANSEC_EXAMPLES,
	MANSEC_STANDARDS,
	MANSEC_HISTORY,
	MANSEC_BUGS,
	MANSEC_AUTHORS,
	MANSEC_COPYRIGHT,
	MANSEC_NONE
};

typedef struct query_args {
	const char *search_str;		// user query
	int *sec_nums;		// Section in which to do the search
	int nrec;			// number of records to fetch
	int offset;		//From which position to start processing the records
	const char *machine;
	int (*callback) (void *, const char *, const char *, const char *,
		const char *, size_t);	// The callback function
	void *callback_data;	// data to pass to the callback function
	char **errmsg;		// buffer for storing the error msg
} query_args;

char *lower(char *);
void concat(char **, const char *);
void concat2(char **, const char *, size_t);
sqlite3 *init_db(int, const char *);
void close_db(sqlite3 *);
char *get_dbpath(const char *);
int run_query(sqlite3 *, const char *[3], query_args *);
int run_query_html(sqlite3 *, query_args *);
int run_query_pager(sqlite3 *, query_args *);
int run_query_term(sqlite3 *, query_args *);
#endif 
