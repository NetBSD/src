/*	$NetBSD: scsipiconf.c,v 1.1.2.1 1997/07/01 16:52:42 bouyer Exp $	*/

/*
 * Copyright (c) 1994 Charles Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipiconf.h>

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
		if (bcmp(inqbuf->vendor, match->vendor, len))
			continue;
		priority += len;
		len = strlen(match->product);
		if (bcmp(inqbuf->product, match->product, len))
			continue;
		priority += len;
		len = strlen(match->revision);
		if (bcmp(inqbuf->revision, match->revision, len))
			continue;
		priority += len;

#ifdef SCSIDEBUG
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
	case T_CDROM:
		dtype = "cdrom";
		break;
	case T_WORM:
		dtype = "worm";
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
	case T_NODEVICE:
		panic("scsipi_dtype: impossible device type");
	default:
		dtype = "unknown";
		break;
	}
	return dtype;
}

void
scsipi_strvis(dst, src, len)
	u_char *dst, *src;
	int len;
{

	/* Trim leading and trailing blanks and NULs. */
	while (len > 0 && (src[0] == ' ' || src[0] == '\0'))
		++src, --len;
	while (len > 0 && (src[len-1] == ' ' || src[len-1] == '\0'))
		--len;

	while (len > 0) {
		if (*src < 0x20 || *src >= 0x80) {
			/* non-printable characters */
			*dst++ = '\\';
			*dst++ = ((*src & 0300) >> 6) + '0';
			*dst++ = ((*src & 0070) >> 3) + '0';
			*dst++ = ((*src & 0007) >> 0) + '0';
		} else if (*src == '\\') {
			/* quote characters */
			*dst++ = '\\';
			*dst++ = '\\';
		} else {
			/* normal characters */
			*dst++ = *src;
		}
		++src, --len;
	}

	*dst++ = 0;
}
