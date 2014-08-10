/*	$NetBSD: savenewlabel.c,v 1.2.2.2 2014/08/10 07:00:24 tls Exp $	*/

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
__RCSID("$NetBSD: savenewlabel.c,v 1.2.2.2 2014/08/10 07:00:24 tls Exp $");
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
	char *f_name = malloc(STRSIZE * sizeof(char));
	int i;
	pm_devs_t *pm_i;

	/* Check names collision */
	if (partman_go)
		SLIST_FOREACH(pm_i, &pm_head, l)
			for (i = 'a'; pm_i != pm && i < 'z' &&
					! strcmp(pm_i->bsddiskname, pm->bsddiskname); i++) {
				if (strlen(pm_i->bsddiskname) > 0)
					pm_i->bsddiskname[strlen(pm_i->bsddiskname)-1] = i;
				else
					snprintf(pm_i->bsddiskname, DISKNAME_SIZE, "disk %c", i);
			}

	snprintf(f_name, STRSIZE, "/tmp/disktab.%s", pm->bsddiskname);

	/*
	  N.B. disklabels only support up to 2TB (32-bit field for sectors).
	  This function explicitly narrows from daddr_t (64-bit unsigned) to
	  uint32_t when writing the disklabel.
	 */

	/* Create /etc/disktab */

	f = fopen(f_name, "w");
	if (logfp)
		(void)fprintf(logfp, "Creating disklabel %s in %s\n", pm->bsddiskname,
						f_name);
	scripting_fprintf(NULL, "cat <<EOF >>/etc/disktab\n");
	if (f == NULL) {
		endwin();
		(void)fprintf(stderr, "Could not open %s for writing\n", f_name);
		if (logfp)
			(void)fprintf(logfp, "Could not open %s for writing\n", f_name);
		exit (1);
	}
	scripting_fprintf(f, "%s|NetBSD installation generated:\\\n", pm->bsddiskname);
	scripting_fprintf(f, "\t:dt=%s:ty=winchester:\\\n", pm->disktype);
	scripting_fprintf(f, "\t:nc#%d:nt#%d:ns#%d:\\\n", pm->dlcyl, pm->dlhead, pm->dlsec);
	scripting_fprintf(f, "\t:sc#%d:su#%" PRIu32 ":\\\n", pm->dlhead*pm->dlsec,
	    (uint32_t)pm->dlsize);
	scripting_fprintf(f, "\t:se#%d:%s\\\n", pm->sectorsize, pm->doessf);
	if ((size_t)nparts > __arraycount(pm->bsdlabel)) {
		nparts = __arraycount(pm->bsdlabel);
		if (logfp)
			(void)fprintf(logfp, "nparts limited to %d.\n", nparts);
	}
	for (i = 0; i < nparts; i++) {
		scripting_fprintf(f, "\t:p%c#%" PRIu32 ":o%c#%" PRIu32
		    ":t%c=%s:", 'a'+i, (uint32_t)lp[i].pi_size,
		    'a'+i, (uint32_t)lp[i].pi_offset, 'a'+i,
		    getfslabelname(lp[i].pi_fstype));
		if (PI_ISBSDFS(&lp[i]))
			scripting_fprintf (f, "b%c#%" PRIu32 ":f%c#%" PRIu32
			    ":", 'a'+i,
			    (uint32_t)(lp[i].pi_fsize *
			    lp[i].pi_frag),
			    'a'+i, (uint32_t)lp[i].pi_fsize);
	
		if (i < nparts - 1)
			scripting_fprintf(f, "\\\n");
		else
			scripting_fprintf(f, "\n");
	}
	fclose (f);
	scripting_fprintf(NULL, "EOF\n");
	fflush(NULL);
	run_program(0, "sh -c 'cat /tmp/disktab.* >/tmp/disktab'");
	return(0);
}
