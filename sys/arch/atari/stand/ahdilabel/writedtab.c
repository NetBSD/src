/*	$NetBSD: writedtab.c,v 1.1.1.1 2000/08/07 09:23:40 leo Exp $	*/

/*
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman, Waldi Ravens and Leo Weppelman.
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

#include "privahdi.h"
#include <disktab.h>
#include <fcntl.h>
#include <stdio.h>

/*
 * Write AHDI partitions to disk
 */
int
ahdi_writedisktab (ptable, disktype, disktab, dtype)
	struct ahdi_ptable	*ptable;
	char			*disktype, *disktab, *dtype;
{
	FILE			*fd;
	int			 pid, i, j, todo;

	if ((fd = fopen (strlen (disktab) ? disktab : _PATH_DISKTAB, "a"))
	    == NULL)
		return (-1);

	fprintf (fd, "%s disk|%s:\\\n",
	    strlen (dtype) ? dtype : "SCSI", disktype);
	fprintf (fd, "\t:ty#winchester:dt=%s:ns#%u:nt#%u:nc#%u:sc#%u:su#%u",
	    strlen (dtype) ? dtype : "SCSI", ptable->nsectors,
	    ptable->ntracks, ptable->ncylinders, ptable->secpercyl,
	    ptable->secperunit);

	todo = ptable->nparts;
	j = 0;
	while (todo) {
		for (i = 0; i < ptable->nparts; i++) {
			if (j == RAW_PART) {
				fprintf (fd,
				    "\\\n\t:p%c#%u:o%c#0:t%c=unknown:",
				    RAW_PART + 'a', ptable->secperunit,
				    RAW_PART + 'a', RAW_PART + 'a');
				break;
			}
			if (ptable->parts[i].letter == j) {
				fprintf (fd, "\\\n\t:p%c#%u:o%c#%u:t%c=",
				    ptable->parts[i].letter + 'a',
				    ptable->parts[i].size,
				    ptable->parts[i].letter + 'a',
				    ptable->parts[i].start,
				    ptable->parts[i].letter + 'a');
				pid = AHDI_MKPID (ptable->parts[i].id[0],
				    ptable->parts[i].id[1],
				    ptable->parts[i].id[2]);
				switch (pid) {
				case AHDI_PID_NBD:
					fprintf (fd, "4.2BSD:");
					break;
				case AHDI_PID_SWP:
					fprintf (fd, "swap:");
					break;
				case AHDI_PID_GEM:
				case AHDI_PID_BGM:
					fprintf (fd, "MSDOS:");
					break;
				default:
					fprintf (fd, "unknown:" );
				}
				todo--;
				break;
			}
		}
		j++;
	}
	if (j <= RAW_PART) {
		fprintf (fd, "\\\n\t:p%c#%u:o%c#0:t%c=unknown:",
		    RAW_PART + 'a', ptable->secperunit,
		    RAW_PART + 'a', RAW_PART + 'a');
	}
	fprintf (fd, "\n\n");

	fclose (fd);
	return (1);
}
