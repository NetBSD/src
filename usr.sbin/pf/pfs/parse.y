/* $NetBSD: parse.y,v 1.1.12.1 2014/08/20 00:05:11 tls Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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

%{
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: parse.y,v 1.1.12.1 2014/08/20 00:05:11 tls Exp $");
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <errno.h>

#include <net/if.h>
#include <netinet/in.h>
#include <net/pfvar.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp_fsm.h>

#include "parser.h"

// XXX it is really correct ?
extern const char * const tcpstates[];


struct pfsync_state global_state;
struct pfsync_state_peer *src_peer, *dst_peer;
struct pfsync_state_peer current_peer;

static void parse_init(void);
static void add_state(void);
static bool get_pfsync_host(const char*, struct pfsync_state_host*, sa_family_t*);
static uint8_t retrieve_peer_state(const char*, int);
static bool retrieve_seq(const char*, struct pfsync_state_peer*);
static bool strtou32(const char*, uint32_t*);

%}

%union {
	uintmax_t num;
	char* str;
}

%token STATE 
%token IN OUT
%token ON PROTO
%token FROM TO USING
%token ID CID EXPIRE TIMEOUT
%token SRC DST
%token SEQ  MAX_WIN WSCALE MSS
%token NOSCRUB SCRUB FLAGS TTL MODE
%token NUMBER STRING

%type <str> STRING
%type <num> NUMBER
%%

states
	: /* NOTHING */
	| state states  { parse_init(); }
	;

state
	: STATE direction iface proto addrs id cid expire timeout src_peer dst_peer { 
			add_state();
		}
	;

direction
	: IN {
		   global_state.direction = PF_IN;
		   src_peer = &global_state.dst;
		   dst_peer = &global_state.src;
		}
	| OUT {
			 global_state.direction = PF_OUT; 
			 src_peer = &global_state.src;
			 dst_peer = &global_state.dst;
		}
	;

iface
	: ON STRING { 
			strlcpy(global_state.ifname, $2, sizeof(global_state.ifname));
			free($2);
		}
	;

proto
	: PROTO STRING { 
			struct protoent *p;
			p = getprotobyname($2);
			if (p == NULL) 
				yyfatal("Invalid protocol name");
			global_state.proto = p->p_proto;
			free($2);
			}
	| PROTO NUMBER { 
			// check that the number may be valid proto ?
			global_state.proto = $2;
			}
	;

addrs
	: FROM STRING TO STRING { 
		get_pfsync_host($2, &global_state.lan, &global_state.af);
		get_pfsync_host($4, &global_state.ext, &global_state.af);
		memcpy(&global_state.gwy, &global_state.lan, sizeof(struct pfsync_state_host));
		free($2);
		free($4);
		}
	| FROM STRING TO STRING USING STRING {
		get_pfsync_host($2, &global_state.lan, &global_state.af);
		get_pfsync_host($4, &global_state.ext, &global_state.af);
		get_pfsync_host($6, &global_state.gwy, &global_state.af);
		free($2);
		free($4);
		free($6);
		}
	;

id	
	: ID NUMBER { 
			if ( $2 > UINT64_MAX) 
				yyfatal("id is too big");
			uint64_t value = (uint64_t)$2;
			memcpy(global_state.id, &value, sizeof(global_state.id));
		}
	;

cid 
	: CID NUMBER { 
			if ( $2 > UINT32_MAX)
				yyfatal("creator id is too big");
			global_state.creatorid = (uint32_t)$2;
		}
	;

expire 
	: EXPIRE NUMBER { 
			if ( $2 > UINT32_MAX)
				yyfatal("expire time is too big");
			global_state.expire = (uint32_t) $2;
		}
	;

timeout
	: TIMEOUT NUMBER {
			if ($2 > UINT8_MAX)
				yyfatal("timeout time is too big");
			global_state.timeout = (uint8_t) $2;
		}
	;

src_peer
	: SRC peer {
			memcpy(src_peer, &current_peer, sizeof(current_peer));
		}
	;

dst_peer
	: DST peer { 
			memcpy(dst_peer, &current_peer, sizeof(current_peer));
		}
	;

peer
	: peer_state scrub 
	| peer_state tcp_options scrub 
	;

peer_state
	: STATE STRING {
			current_peer.state = retrieve_peer_state($2, global_state.proto);
			free($2);
		}
	| STATE	NUMBER { 
		if ( $2 > UINT8_MAX)
			yyfatal("peer state is too big");
		current_peer.state = $2;
		}
	;

tcp_options
	: SEQ seqs MAX_WIN NUMBER WSCALE NUMBER {
			if ($4 > UINT16_MAX)
				yyfatal("max_win is too big");
			current_peer.max_win = $4;
			
			if ($6 > UINT8_MAX)
				yyfatal("wscale is too big");
			current_peer.wscale = $6;
		}
	| SEQ seqs MAX_WIN NUMBER WSCALE NUMBER MSS NUMBER {
			if ($4 > UINT16_MAX)
				yyfatal("max_win is too big");
			current_peer.max_win = $4;
			
			if ($6 > UINT8_MAX)
				yyfatal("wscale is too big");
			current_peer.wscale = $6;

			if ($8 > UINT16_MAX)
				yyfatal("mss is too big");
			current_peer.mss = $8;
		}
	;

seqs 
	: STRING {
		if (!retrieve_seq($1, &current_peer))
			yyfatal("invalid seq number");

		free($1);
		}
	;

scrub
	: NOSCRUB { current_peer.scrub.scrub_flag= 0;}
	| SCRUB FLAGS NUMBER MODE NUMBER TTL NUMBER {
			current_peer.scrub.scrub_flag= PFSYNC_SCRUB_FLAG_VALID;
			if ($3 > UINT16_MAX)
				yyfatal("scrub flags is too big");
			current_peer.scrub.pfss_flags = $3;

			if ($5 > UINT32_MAX)
				yyfatal("scrub mode is too big");
			current_peer.scrub.pfss_ts_mod = $5;

			if ($7 > UINT8_MAX)
				yyfatal("scrub ttl is too big");
			current_peer.scrub.pfss_ttl = $7;
		}
	;


%%

static void
parse_init(void)
{
	memset(&global_state, 0, sizeof(global_state));
	memset(&current_peer, 0, sizeof(current_peer));
	src_peer = NULL;
	dst_peer = NULL;
}

static bool
get_pfsync_host(const char* str, struct pfsync_state_host* host, sa_family_t* af)
{
	size_t count_colon, addr_len, port_len;
	const char* p, *last_colon, *first_bracket, *last_bracket;
	char buf[48];
	char buf_port[6];

	if (str == NULL || *str == '\0')
		return false;

	p = str;
	last_colon = NULL;
	count_colon = 0;

	while (*p != '\0') {
		if (*p == ':') {
			count_colon++;
			last_colon = p;
		}
		p++;
	}

	/*
	 * If no colon, it is not an expected addr
	 * If there are more than one colon, we guess that af = AF_INET6
	 */

	if (count_colon == 0)
		return false;

	if (count_colon == 1)
		*af = AF_INET;
	else
		*af = AF_INET6;

	/*
	 * First bracket must be next character after last colon
	 * Last bracket must be the last character
	 * distance between both must be <= 7
	 */ 
	
	if (*(last_colon+1) == '[')
		first_bracket = last_colon + 1;
	else
		return false;

	last_bracket = str + (strlen(str) - 1);
	if (*last_bracket != ']')
		return false;
	
	port_len = last_bracket - first_bracket;
	if (last_bracket - first_bracket > 7)
		return false;

	memcpy(buf_port, first_bracket +1, port_len - 1);
	buf_port[port_len-1]= '\0';
	
	addr_len = last_colon - str;
	if (addr_len >= sizeof(buf))
		return false;
	memcpy(buf, str, addr_len);
	buf[addr_len] = '\0';

	if (inet_pton(*af, buf, &host->addr) != 1)
		return false;

	host->port = htons(atoi(buf_port));

	return true;
}

static uint8_t
retrieve_peer_state(const char* str, int proto)
{
	uint8_t i;

	if (proto == IPPROTO_TCP) {
		i = 0;
		while (i < TCP_NSTATES) {
			if (strcmp(str, tcpstates[i]) == 0)
				return i;
			i++;
		}
		yyfatal("Invalid peer state");

	} else {
		if (proto == IPPROTO_UDP) {
			const char* mystates[] = PFUDPS_NAMES;
			i = 0;

			while (i < PFUDPS_NSTATES) {
				if (strcmp(str, mystates[i]) == 0)
					return i;
				i++;
			}

			yyfatal("Invalid peer state");
		} else {
			const char *mystates[] = PFOTHERS_NAMES;
			i = 0;

			while (i < PFOTHERS_NSTATES) {
				if (strcmp(str, mystates[i]) == 0)
					return i;
				i++;
			}

			yyfatal("Invalid peer state");
		}
	}
     /*NOTREACHED*/
	return 0;
}

static bool
strtou32(const char* str, uint32_t* res)
{
	uintmax_t u;
	errno = 0;
	u = strtoumax(str, NULL, 10);
	if (errno == ERANGE && u == UINTMAX_MAX)
		return false;
	if (u > UINT32_MAX)
		return false;
	*res = (uint32_t) u;
	return true;
}

static bool
retrieve_seq(const char* str, struct pfsync_state_peer* peer)
{
	const char* p, *p_colon, *p_comma;
	char buf[100];
	size_t size;

	if (str == NULL || *str == '\0')
		return false;

	if (*str != '[' || *(str+(strlen(str) -1)) != ']')
		return false;

	p = str;
	p_colon = NULL;
	p_comma = NULL;
	while (*p != '\0') {
		if (*p == ':') {
			if (p_colon !=NULL) 
				return false;
			else 
				p_colon = p;
		}

		if (*p == ',') {
			if (p_comma != NULL) 
				return false;
			else
				p_comma = p;
		}
		p++;
	}

	size = p_colon - str;
	if (size > sizeof(buf))
		return false;
	memcpy(buf, str+1, size-1);
	buf[size-1] = '\0';

	if (!strtou32(buf, &peer->seqlo))
		return false;


	if (p_comma == NULL) 
		size = str + strlen(str) - 1 - p_colon;
	else 
		size = p_comma - p_colon;
		
	if (size > sizeof(buf))
		return false;
	memcpy(buf, p_colon+1, size -1);
	buf[size-1] = '\0';

	if (!strtou32(buf, &peer->seqhi))
		return false;

	if (p_comma == NULL) {
		peer->seqdiff = 0;
	} else {
		size = str + strlen(str) - 1 - p_comma;
		if (size > sizeof(buf))
			return false;
		memcpy(buf, p_comma +1, size -1);
		buf[size-1] = '\0';

		if (!strtou32(buf, &peer->seqdiff))
			return false;
	}
		
	return true;
}

static void
add_state(void)
{

	if (allocated == 0) {
		allocated = 5;
		states->ps_buf = malloc(allocated * sizeof(struct pfsync_state));
		if (states->ps_buf == NULL)
			yyfatal("Not enougth memory");
	}

	if (allocated == (states->ps_len / sizeof(struct pfsync_state))) {
		void *buf;
		allocated = allocated * 2 + 1; 
		buf = realloc(states->ps_buf, allocated * sizeof(struct pfsync_state));
		if (buf == NULL) {
			free(states->ps_buf);
			yyfatal("Not enougth memory");
		}
		states->ps_buf = buf;
	}

}
