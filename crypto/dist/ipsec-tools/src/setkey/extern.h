/*	$NetBSD: extern.h,v 1.9 2020/05/12 16:17:58 christos Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* parse.y */
void parse_init(void);

/* token.l */
int parse(const char *, FILE *);
int parse_string(char *);

/* setkey.c */
int sendkeymsg(char *, size_t);
uint32_t *sendkeymsg_spigrep(unsigned int, struct addrinfo *,
    struct addrinfo *, int *);

int yylex(void);
int yyparse(void);
void yyfatal(const char *);
void yyerror(const char *);

#ifdef HAVE_POLICY_FWD
extern int f_rfcmode;
#endif
extern int f_mode;
extern const char *filename;
extern int lineno;
extern int exit_now;
#ifdef HAVE_PFKEY_POLICY_PRIORITY
extern int last_msg_type;
extern uint32_t last_priority;
#endif

#define MODE_SCRIPT	1
#define MODE_CMDDUMP	2
#define MODE_CMDFLUSH	3
#define MODE_PROMISC	4
#define MODE_STDIN	5

