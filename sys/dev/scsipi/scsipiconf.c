/*	$NetBSD: scsipiconf.c,v 1.12.2.3 2001/08/24 00:10:51 nathanw Exp $	*/

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum; by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Originally written by Julian Elischer (julian@tfs.com)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 * Ported to run under 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/lwp.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipiconf.h>

#define	STRVIS_ISWHITE(x) ((x) == ' ' || (x) == '\0' || (x) == (u_char)'\377')

int
scsipi_command(periph, cmd, cmdlen, data_addr, datalen, retries, timeout, bp,
     flags)
	struct scsipi_periph *periph;
	struct scsipi_generic *cmd;
	int cmdlen;
	u_char *data_addr;
	int datalen;
	int retries;
	int timeout;
	struct buf *bp;
	int flags;
{
	int error;
 
	if ((flags & XS_CTL_DATA_ONSTACK) != 0) {
		/*
		 * If the I/O buffer is allocated on stack, the
		 * process must NOT be swapped out, as the device will
		 * be accessing the stack.
		 */
		PHOLD(curproc);
	}
	error = (*periph->periph_channel->chan_bustype->bustype_cmd)(periph,
	    cmd, cmdlen, data_addr, datalen, retries, timeout, bp, flags);
	if ((flags & XS_CTL_DATA_ONSTACK) != 0)
		PRELE(curproc);
	return (error);
}

/*
 * allocate and init a scsipi_periph structure for a new device.
 */
struct scsipi_periph *
scsipi_alloc_periph(malloc_flag)
	int malloc_flag;
{
	struct scsipi_periph *periph;
	int i;

	periph = malloc(sizeof(*periph), M_DEVBUF, malloc_flag);
	if (periph == NULL)
		return NULL;
	memset(periph, 0, sizeof(*periph));

	periph->periph_dev = NULL;

	/*
	 * Start with one command opening.  The periph driver
	 * will grow this if it knows it can take advantage of it.
	 */
	periph->periph_openings = 1; 
	periph->periph_active = 0;   

	for (i = 0; i < PERIPH_NTAGWORDS; i++)
		periph->periph_freetags[i] = 0xffffffff;

	TAILQ_INIT(&periph->periph_xferq);
	callout_init(&periph->periph_callout);

	return periph;
}

/*
 * Return a priority based on how much of the inquiry data matches
 * the patterns for the particular driver.
 */
caddr_t
scsipi_inqmatch(inqbuf, base, nmatches, matchsize, bestpriority)
	struct scsipi_inquiry_pattern *inqbuf;
	caddr_t base;
	int nmatches, matchsize;
	int *bestpriority;
{
	u_int8_t type;
	caddr_t bestmatch;

	/* Include the qualifier to catch vendor-unique types. */
	type = inqbuf->type;

	for (*bestpriority = 0, bestmatch = 0; nmatches--; base += matchsize) {
		struct scsipi_inquiry_pattern *match = (void *)base;
		int priority, len;

		if (type != match->type)
			continue;
		if (inqbuf->removable != match->removable)
			continue;
		priority = 2;
		len = strlen(match->vendor);
		if (memcmp(inqbuf->vendor, match->vendor, len))
			continue;
		priority += len;
		len = strlen(match->product);
		if (memcmp(inqbuf->product, match->product, len))
			continue;
		priority += len;
		len = strlen(match->revision);
		if (memcmp(inqbuf->revision, match->revision, len))
			continue;
		priority += len;

#ifdef SCSIPI_DEBUG
		printf("scsipi_inqmatch: %d/%d/%d <%s, %s, %s>\n",
		    priority, match->type, match->removable,
		    match->vendor, match->product, match->revision);
#endif
		if (priority > *bestpriority) {
			*bestpriority = priority;
			bestmatch = base;
		}
	}

	return (bestmatch);
}

char *
scsipi_dtype(type)
	int type;
{
	char *dtype;

	switch (type) {
	case T_DIRECT:
		dtype = "direct";
		break;
	case T_SEQUENTIAL:
		dtype = "sequential";
		break;
	case T_PRINTER:
		dtype = "printer";
		break;
	case T_PROCESSOR:
		dtype = "processor";
		break;
	case T_WORM:
		dtype = "worm";
		break;
	case T_CDROM:
		dtype = "cdrom";
		break;
	case T_SCANNER:
		dtype = "scanner";
		break;
	case T_OPTICAL:
		dtype = "optical";
		break;
	case T_CHANGER:
		dtype = "changer";
		break;
	case T_COMM:
		dtype = "communication";
		break;
	case T_IT8_1:
	case T_IT8_2:
		dtype = "graphic arts pre-press";
		break;
	case T_STORARRAY:
		dtype = "storage array";
		break;
	case T_ENCLOSURE:
		dtype = "enclosure services";
		break;
	case T_SIMPLE_DIRECT:
		dtype = "simplified direct";
		break;
	case T_OPTIC_CARD_RW:
		dtype = "optical card r/w";
		break;
	case T_OBJECT_STORED:
		dtype = "object-based storage";
		break;
	case T_NODEVICE:
		panic("scsipi_dtype: impossible device type");
	default:
		dtype = "unknown";
		break;
	}
	return (dtype);
}

void
scsipi_strvis(dst, dlen, src, slen)
	u_char *dst, *src;
	int dlen, slen;
{

	/* Trim leading and trailing blanks and NULs. */
	while (slen > 0 && STRVIS_ISWHITE(src[0]))
		++src, --slen;
	while (slen > 0 && STRVIS_ISWHITE(src[slen - 1]))
		--slen;

	while (slen > 0) {
		if (*src < 0x20 || *src >= 0x80) {
			/* non-printable characters */
			dlen -= 4;
			if (dlen < 1)
				break;
			*dst++ = '\\';
			*dst++ = ((*src & 0300) >> 6) + '0';
			*dst++ = ((*src & 0070) >> 3) + '0';
			*dst++ = ((*src & 0007) >> 0) + '0';
		} else if (*src == '\\') {
			/* quote characters */
			dlen -= 2;
			if (dlen < 1)
				break;
			*dst++ = '\\';
			*dst++ = '\\';
		} else {
			/* normal characters */
			if (--dlen < 1)
				break;
			*dst++ = *src;
		}
		++src, --slen;
	}

	*dst++ = 0;
}
