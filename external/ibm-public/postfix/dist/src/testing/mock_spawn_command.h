/*	$NetBSD: mock_spawn_command.h,v 1.1.1.1 2026/05/09 18:39:22 christos Exp $	*/

#ifndef _MOCK_SPAWN_COMMAND_H_INCLUDED_
#define _MOCK_SPAWN_COMMAND_H_INCLUDED_

/*++
/* NAME
/*	mock_spawn_command.h 3h
/* SUMMARY
/*	spawn_command() mocker
/* SYNOPSIS
/*	#include <mock_spawn_command.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <spawn_command.h>

 /*
  * External API.
  */
typedef struct MOCK_SPAWN_CMD_REQ {
    struct spawn_args want_args;
    int     out_status;
    const char *out_text;
} MOCK_SPAWN_CMD_REQ;

extern void setup_mock_spawn_command(const MOCK_SPAWN_CMD_REQ *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
