/*	$NetBSD: nosleep.c,v 1.1.1.1 2026/05/09 18:39:22 christos Exp $	*/

/*++
/* NAME
/*	nosleep 3
/* SUMMARY
/*	override the system sleep(3) implementation
/* SYNOPSIS
/*	LD_PRELOAD=/path/to/nosleep.so command....
/*
/*	int	sleep(unsigned int seconds)
/* DESCRIPTION
/*	This sleep(3) implementation returns zero immediately. The purpose
/*	is to speed up error handler tests by skipping the delay between
/*	reporting a fatal error and before terminating the process.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#include <unistd.h>

unsigned int sleep(unsigned int seconds)
{
    /* Don't sleep. */
    return (0);
}
