/* $NetBSD: uuid.c,v 1.1 2009/06/25 13:47:11 agc Exp $ */

/*
 * Copyright © 2006 Alistair Crooks.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "config.h"
   
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
 
#include <sys/types.h>
 
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
  
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage.h"
#include "compat.h"
#include "defs.h"

/* just fill the struct with random values for now */
void
nbuuid_create(nbuuid_t *uuid, uint32_t *status)
{
	uint64_t	ether;
	time_t		t;

	(void) time(&t);
	ether = ((uint64_t)random() << 32) | random();
	uuid->time_low = t;
	uuid->time_mid = (uint16_t)(random() & 0xffff);
	uuid->time_hi_and_version = (uint16_t)(random() & 0xffff);
	uuid->clock_seq_low = random() & 0xff;
	uuid->clock_seq_hi_and_reserved = random() & 0xff;
	(void) memcpy(&uuid->node, &ether, sizeof(uuid->node));
	*status = 0;
}

/* convert the struct to a printable string */
void
nbuuid_to_string(nbuuid_t *uuid, char **str, uint32_t *status)
{
	char	s[64];

	(void) snprintf(s, sizeof(s), "%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
		uuid->time_low,
		uuid->time_mid,
		uuid->time_hi_and_version,
		uuid->clock_seq_hi_and_reserved,
		uuid->clock_seq_low,
		uuid->node[0],
		uuid->node[1],
		uuid->node[2],
		uuid->node[3],
		uuid->node[4],
		uuid->node[5]);
	*str = strdup(s);
	*status = 0;
}
