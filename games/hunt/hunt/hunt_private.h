/*	$NetBSD: hunt_private.h,v 1.9.8.2 2014/08/20 00:00:23 tls Exp $	*/

/*
 * Copyright (c) 1983-2003, Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 * + Redistributions of source code must retain the above copyright 
 *   notice, this list of conditions and the following disclaimer.
 * + Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in the 
 *   documentation and/or other materials provided with the distribution.
 * + Neither the name of the University of California, San Francisco nor 
 *   the names of its contributors may be used to endorse or promote 
 *   products derived from this software without specific prior written 
 *   permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdbool.h>
#include <stdio.h> /* for BUFSIZ */

#ifdef INTERNET
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>
#else
#include <sys/un.h>
#endif

/*
 * external variables
 */

extern bool Last_player;

extern char Buf[BUFSIZ];
extern int huntsocket;

#ifdef INTERNET
extern char *Send_message;
#endif

#ifdef MONITOR
extern bool Am_monitor;
#endif

extern char map_key[256];
extern bool no_beep;

#ifdef INTERNET
/* XXX this had to be made public to split off server.c; fix it up */
extern SOCKET Daemon;
#endif

/*
 * function types
 */

/* in connect.c */
void do_connect(const char *, size_t, char, int32_t);

/* in hunt.c */
__dead void bad_con(void);
__dead void bad_ver(void);
__dead __printflike(2, 3) void leave(int, const char *, ...);
__dead __printflike(2, 3) void leavex(int, const char *, ...);
void intr(int);

/* in otto.c */
void otto(int, int, char);

/* in playit.c */
void playit(void);
int quit(int);
void clear_the_screen(void);
void do_message(void);

/* in server.c */
#ifdef INTERNET
void serverlist_setup(const char *, uint16_t);
void serverlist_query(unsigned short msg);
unsigned serverlist_num(void);
const struct sockaddr_storage *serverlist_gethost(unsigned, socklen_t *);
unsigned short serverlist_getresponse(unsigned);
#endif
