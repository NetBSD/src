/*  $NetBSD: debug.c,v 1.12.24.1 2019/02/10 13:40:41 martin Exp $ */

/*-
 *  Copyright (c) 2010 Emmanuel Dreyfus. All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <puffs.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <sys/types.h>

#include "perfuse_if.h"
#include "perfuse_priv.h"
#include "fuse.h"

struct perfuse_opcode {
	int  opcode;
	const char *opname;
};

const struct perfuse_opcode perfuse_opcode[] = {
	{ FUSE_LOOKUP, "LOOKUP" },
	{ FUSE_FORGET, "FORGET" },
	{ FUSE_GETATTR, "GETATTR" },
	{ FUSE_SETATTR, "SETATTR" },
	{ FUSE_READLINK, "READLINK" },
	{ FUSE_SYMLINK, "SYMLINK" },
	{ FUSE_MKNOD, "MKNOD" },
	{ FUSE_MKDIR, "MKDIR" },
	{ FUSE_UNLINK, "UNLINK" },
	{ FUSE_RMDIR, "RMDIR" },
	{ FUSE_RENAME, "RENAME" },
	{ FUSE_LINK, "LINK" },
	{ FUSE_OPEN, "OPEN" },
	{ FUSE_READ, "READ" },
	{ FUSE_WRITE, "WRITE" },
	{ FUSE_STATFS, "STATFS" },
	{ FUSE_RELEASE, "RELEASE" },
	{ FUSE_FSYNC, "FSYNC" },
	{ FUSE_SETXATTR, "SETXATTR" },
	{ FUSE_GETXATTR, "GETXATTR" },
	{ FUSE_LISTXATTR, "LISTXATTR" },
	{ FUSE_REMOVEXATTR, "REMOVEXATTR" },
	{ FUSE_FLUSH, "FLUSH" },
	{ FUSE_INIT, "INIT" },
	{ FUSE_OPENDIR, "OPENDIR" },
	{ FUSE_READDIR, "READDIR" },
	{ FUSE_RELEASEDIR, "RELEASEDIR" },
	{ FUSE_FSYNCDIR, "FSYNCDIR" },
	{ FUSE_GETLK, "GETLK" },
	{ FUSE_SETLK, "SETLK" },
	{ FUSE_SETLKW, "SETLKW" },
	{ FUSE_ACCESS, "ACCESS" },
	{ FUSE_CREATE, "CREATE" },
	{ FUSE_INTERRUPT, "INTERRUPT" },
	{ FUSE_BMAP, "BMAP" },
	{ FUSE_DESTROY, "DESTROY" },
	{ FUSE_IOCTL, "IOCTL" },
	{ FUSE_POLL, "POLL" },
	{ FUSE_CUSE_INIT, "CUSE_INIT" },
	{ 0, "UNKNOWN" },
};

const char * const perfuse_qtypestr[] = { 
	"READDIR",
	"READ",
	"WRITE",
	"AFTERWRITE",
	"OPEN",
	"AFTERXCHG",
	"REF"
};

const char *
perfuse_opname(int opcode)
{
	const struct perfuse_opcode *po;

	for (po = perfuse_opcode; po->opcode; po++) {
		if (po->opcode == opcode)
			return po->opname;
	}

	return po->opname; /* "UNKNOWN" */
}

char *
perfuse_opdump_in(struct perfuse_state *ps, perfuse_msg_t *pm)
{
	struct fuse_in_header *fih;
	static char buf[BUFSIZ] = "";

	fih = GET_INHDR(ps, pm);

	switch(fih->opcode) {
	case FUSE_LOOKUP: 
		(void)snprintf(buf, sizeof(buf), "path = \"%s\"", 
			       _GET_INPAYLOAD(ps, pm, const char *));
		break;
	default:
		buf[0] = '\0';
		break;
	}

	return buf;
}

struct perfuse_trace *
perfuse_trace_begin(struct perfuse_state *ps, puffs_cookie_t opc,
	perfuse_msg_t *pm)
{
	struct perfuse_trace *pt;

	if ((pt = malloc(sizeof(*pt))) == NULL)
		DERR(EX_OSERR, "malloc failed");

	pt->pt_opcode = ps->ps_get_inhdr(pm)->opcode;
	pt->pt_status = inxchg;

	if (clock_gettime(CLOCK_REALTIME, &pt->pt_start) != 0)
		DERR(EX_OSERR, "clock_gettime failed");

	if (opc == 0)
		(void)strcpy(pt->pt_path, "");
	else
		(void)strlcpy(pt->pt_path, 
			      perfuse_node_path(ps, opc),
			      sizeof(pt->pt_path));

	(void)strlcpy(pt->pt_extra,
		      perfuse_opdump_in(ps, pm),
		      sizeof(pt->pt_extra));

	TAILQ_INSERT_TAIL(&ps->ps_trace, pt, pt_list);
	ps->ps_tracecount++;

	return pt;
}

void
perfuse_trace_end(struct perfuse_state *ps, struct perfuse_trace *pt, int error)
{
	if (clock_gettime(CLOCK_REALTIME, &pt->pt_end) != 0)
		DERR(EX_OSERR, "clock_gettime failed");

	pt->pt_status = done;
	pt->pt_error = error;

	while (ps->ps_tracecount > PERFUSE_TRACECOUNT_MAX) {
		struct perfuse_trace *fpt = TAILQ_FIRST(&ps->ps_trace);

		if (fpt == NULL || fpt->pt_status != done)
			break;

		TAILQ_REMOVE(&ps->ps_trace, fpt, pt_list);
		free(fpt);
		ps->ps_tracecount--;
	}
}

void
perfuse_trace_dump(struct puffs_usermount *pu, FILE *fp)
{
	struct perfuse_state *ps;
	struct perfuse_trace *pt;
	struct timespec ts_min[FUSE_OPCODE_MAX];
	struct timespec ts_max[FUSE_OPCODE_MAX];
	struct timespec ts_total[FUSE_OPCODE_MAX];
	int count[FUSE_OPCODE_MAX];
	uint64_t avg;
	int i;

	if (!(perfuse_diagflags & PDF_TRACE))
		return;

	ps = puffs_getspecific(pu);

	(void)ftruncate(fileno(fp), 0);
	(void)fseek(fp, 0UL, SEEK_SET);

	(void)memset(&ts_min, 0, sizeof(ts_min));
	(void)memset(&ts_max, 0, sizeof(ts_max));
	(void)memset(&ts_total, 0, sizeof(ts_total));
	(void)memset(&count, 0, sizeof(count));

	fprintf(fp, "Last %"PRId64" operations\n", ps->ps_tracecount);

	TAILQ_FOREACH(pt, &ps->ps_trace, pt_list) {
		const char *quote = pt->pt_path[0] != '\0' ? "\"" : "";

		fprintf(fp, "%lld.%09ld %s %s%s%s %s ",  
			(long long)pt->pt_start.tv_sec, pt->pt_start.tv_nsec,
			perfuse_opname(pt->pt_opcode),
			quote, pt->pt_path, quote,
			pt->pt_extra);

		if (pt->pt_status == done) {
			struct timespec ts;

			ts.tv_sec = 0;	/* delint */
			ts.tv_nsec = 0;	/* delint */
			timespecsub(&pt->pt_end, &pt->pt_start, &ts);

			fprintf(fp, "error = %d elapsed = %lld.%09lu ",
				pt->pt_error, (long long)ts.tv_sec,
				ts.tv_nsec);

			count[pt->pt_opcode]++;
			timespecadd(&ts_total[pt->pt_opcode],
				    &ts,
				    &ts_total[pt->pt_opcode]);

			if (timespeccmp(&ts, &ts_min[pt->pt_opcode], <) ||
			    (count[pt->pt_opcode] == 1))
				ts_min[pt->pt_opcode] = ts;

			if (timespeccmp(&ts, &ts_max[pt->pt_opcode], >))
				ts_max[pt->pt_opcode] = ts;
		} else {
			fprintf(fp, "ongoing ");
		}

		fprintf(fp, "\n");
	}

	fprintf(fp, "\nStatistics by operation\n");
	fprintf(fp, "operation\tcount\tmin\tavg\tmax\n");
	for (i = 0; i < FUSE_OPCODE_MAX; i++) {
		time_t min;

		if (count[i] != 0) {
			avg = timespec2ns(&ts_total[i]) / count[i];
			min = ts_min[i].tv_sec;
		} else {
			avg = 0;
			min = 0;
		}
			
		fprintf(fp, "%s\t%d\t%lld.%09ld\t%lld.%09ld\t%lld.%09ld\t\n",
			perfuse_opname(i), count[i],
			(long long)min, ts_min[i].tv_nsec,
			(long long)(time_t)(avg / 1000000000L),
			(long)(avg % 1000000000L),
			(long long)ts_max[i].tv_sec, ts_max[i].tv_nsec);
	}	

	fprintf(fp, "\n\nGlobal statistics\n");
	fprintf(fp, "Nodes: %d\n", ps->ps_nodecount);
	fprintf(fp, "Exchanges: %d\n", ps->ps_xchgcount);
	
	(void)fflush(fp);
	return;
}
