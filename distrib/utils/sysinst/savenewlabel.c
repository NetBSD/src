/*	$NetBSD: savenewlabel.c,v 1.5.42.1 2009/05/18 19:35:14 bouyer Exp $	*/

/*
 * Copyright 1997 Jonathan Stone
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Jonathan Stone.
 * 4. The name of Jonathan Stone may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JONATHAN STONE ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: savenewlabel.c,v 1.5.42.1 2009/05/18 19:35:14 bouyer Exp $");
#endif

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <inttypes.h>
#include <util.h>
#include <unistd.h>
#include <sys/dkio.h>
#include <sys/ioctl.h>

#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"

int
/*ARGSUSED*/
savenewlabel(partinfo *lp, int nparts)
{
	FILE *f;
	int i;

	/*
	  N.B. disklabels only support up to 2TB (32-bit field for sectors).
	  This function explicitly narrows from daddr_t (64-bit unsigned) to
	  uint32_t when writing the disklabel.
	 */

	/* Create the disktab.preinstall */
	f = fopen("/tmp/disktab", "w");
	if (logging)
		(void)fprintf(logfp, "Creating disklabel %s\n", bsddiskname);
	scripting_fprintf(NULL, "cat <<EOF >>/etc/disktab\n");
	if (f == NULL) {
		endwin();
		(void)fprintf(stderr, "Could not open /etc/disktab");
		if (logging)
			(void)fprintf(logfp,
			    "Failed to open /etc/diskabel for appending.\n");
		exit (1);
	}
	scripting_fprintf(f, "%s|NetBSD installation generated:\\\n", bsddiskname);
	scripting_fprintf(f, "\t:dt=%s:ty=winchester:\\\n", disktype);
	scripting_fprintf(f, "\t:nc#%d:nt#%d:ns#%d:\\\n", dlcyl, dlhead, dlsec);
	scripting_fprintf(f, "\t:sc#%d:su#%" PRIu32 ":\\\n", dlhead*dlsec,
	    (uint32_t)dlsize);
	scripting_fprintf(f, "\t:se#%d:%s\\\n", sectorsize, doessf);
	for (i = 0; i < nparts; i++) {
		scripting_fprintf(f, "\t:p%c#%" PRIu32 ":o%c#%" PRIu32 ":t%c=%s:",
		    'a'+i, (uint32_t)bsdlabel[i].pi_size,
		    'a'+i, (uint32_t)bsdlabel[i].pi_offset,
		    'a'+i, fstypenames[bsdlabel[i].pi_fstype]);
		if (PI_ISBSDFS(&bsdlabel[i]))
			scripting_fprintf (f, "b%c#%" PRIu32 ":f%c#%" PRIu32 ":ta=4.2BSD:",
			   'a'+i, (uint32_t)(bsdlabel[i].pi_fsize * bsdlabel[i].pi_frag),
			   'a'+i, (uint32_t)bsdlabel[i].pi_fsize);
	
		if (i < nparts - 1)
			scripting_fprintf(f, "\\\n");
		else
			scripting_fprintf(f, "\n");
	}
	fclose (f);
	scripting_fprintf(NULL, "EOF\n");
	fflush(NULL);
	return(0);
}
