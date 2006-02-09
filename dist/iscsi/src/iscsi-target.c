/* $NetBSD: iscsi-target.c,v 1.2 2006/02/09 15:11:40 he Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Alistair Crooks
 *	for the NetBSD project.
 * 4. The name of the author may not be used to endorse or promote
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

#define EXTERN

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
 
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
    
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <unistd.h>

#include "iscsi.h"
#include "util.h"
#include "target.h"
#include "device.h"

#include "conffile.h"
#include "storage.h"

/* let's use symbolic names for the fields in the config file */
enum {
	EXTENT_NAME_COL = 0,
	EXTENT_DEVICE_COL = 1,
	EXTENT_SACRED_COL = 2,
	EXTENT_LENGTH_COL = 3,

	DEVICE_NAME_COL = 0,
	DEVICE_RAIDLEVEL_COL = 1,
	DEVICE_LENGTH_COL = 2,

	TARGET_NAME_COL = 0,
	TARGET_DEVICE_COL = 1,
	TARGET_NETMASK_COL = 2
};


static int      g_main_pid;

static globals_t	g;

/* find an extent by name */
static disc_extent_t *
find_extent(extv_t *evp, char *s)
{
	int	i;

	for (i = 0 ; i < evp->c ; i++) {
		if (strcmp(evp->v[i].extent, s) == 0) {
			return &evp->v[i];
		}
	}
	return NULL;
}

/* allocate space for a new extent */
static int
do_extent(conffile_t *cf, extv_t *evp, ent_t *ep)
{
	char	*cp;

	if (find_extent(evp, ep->sv.v[EXTENT_NAME_COL]) != NULL) {
		(void) fprintf(stderr, "%s:%d: ", conffile_get_name(cf), conffile_get_lineno(cf));
		(void) fprintf(stderr, "Error: attempt to re-define extent `%s'\n", ep->sv.v[EXTENT_NAME_COL]);
		return 0;
	}
	ALLOC(disc_extent_t, evp->v, evp->size, evp->c, 14, 14, "do_extent", exit(EXIT_FAILURE));
	evp->v[evp->c].extent = strdup(ep->sv.v[EXTENT_NAME_COL]);
	evp->v[evp->c].dev = strdup(ep->sv.v[EXTENT_DEVICE_COL]);
	evp->v[evp->c].sacred = strtoll(ep->sv.v[EXTENT_SACRED_COL], NULL, 10);
	evp->v[evp->c].len = strtoll(ep->sv.v[EXTENT_LENGTH_COL], &cp, 10);
	if (cp != NULL) {
		switch(tolower((unsigned)*cp)) {
		case 't':
			evp->v[evp->c].len *= (uint64_t)(1024ULL * 1024ULL * 1024ULL * 1024ULL);
			break;
		case 'g':
			evp->v[evp->c].len *= (uint64_t)(1024ULL * 1024ULL * 1024ULL);
			break;
		case 'm':
			evp->v[evp->c].len *= (uint64_t)(1024ULL * 1024ULL);
			break;
		case 'k':
			evp->v[evp->c].len *= (uint64_t)1024ULL;
			break;
		}
	}
	evp->c += 1;
	return 1;
}

/* find a device by name */
static disc_device_t *
find_device(devv_t *devvp, char *s)
{
	int	i;

	for (i = 0 ; i < devvp->c ; i++) {
		if (strcmp(devvp->v[i].dev, s) == 0) {
			return &devvp->v[i];
		}
	}
	return NULL;
}

/* return the size of the sub-device/extent */
static uint64_t
getsize(conffile_t *cf, devv_t *devvp, extv_t *evp, char *s)
{
	disc_extent_t	*xp;
	disc_device_t	*dp;

	if ((xp = find_extent(evp, s)) != NULL) {
		return xp->len;
	}
	if ((dp = find_device(devvp, s)) != NULL) {
		switch (dp->xv[0].type) {
		case DE_EXTENT:
			return dp->xv[0].u.xp->len;
		case DE_DEVICE:
			return dp->xv[0].u.dp->len;
		}
	}
	(void) fprintf(stderr, "%s:%d: ", conffile_get_name(cf), conffile_get_lineno(cf));
	(void) fprintf(stderr, "Warning: sub-device/extent `%s' not found\n", s);
	return 0;
}

/* allocate space for a device */
static int
do_device(conffile_t *cf, devv_t *devvp, extv_t *evp, ent_t *ep)
{
	if (find_device(devvp, ep->sv.v[DEVICE_NAME_COL]) != NULL) {
		(void) fprintf(stderr, "%s:%d: ", conffile_get_name(cf), conffile_get_lineno(cf));
		(void) fprintf(stderr, "Error: attempt to re-define device `%s'\n", ep->sv.v[DEVICE_NAME_COL]);
		return 0;
	}
	ALLOC(disc_device_t, devvp->v, devvp->size, devvp->c, 14, 14, "do_device", exit(EXIT_FAILURE));
	devvp->v[devvp->c].dev = strdup(ep->sv.v[DEVICE_NAME_COL]);
	devvp->v[devvp->c].raid = (strncasecmp(ep->sv.v[DEVICE_RAIDLEVEL_COL], "raid", 4) == 0) ? atoi(&ep->sv.v[DEVICE_RAIDLEVEL_COL][4]) : 0;
	devvp->v[devvp->c].size = ep->sv.c - 2;
	devvp->v[devvp->c].len = getsize(cf, devvp, evp, ep->sv.v[DEVICE_LENGTH_COL]);
	NEWARRAY(disc_de_t, devvp->v[devvp->c].xv, ep->sv.c - 2, "do_device", exit(EXIT_FAILURE));
	for (devvp->v[devvp->c].c = 0 ; devvp->v[devvp->c].c < devvp->v[devvp->c].size ; devvp->v[devvp->c].c++) {
		if ((devvp->v[devvp->c].xv[devvp->v[devvp->c].c].u.xp = find_extent(evp, ep->sv.v[devvp->v[devvp->c].c + 2])) != NULL) {
			if (devvp->v[devvp->c].xv[devvp->v[devvp->c].c].u.xp->used) {
				(void) fprintf(stderr, "%s:%d: ", conffile_get_name(cf), conffile_get_lineno(cf));
				(void) fprintf(stderr, "Error: extent `%s' has already been used\n", ep->sv.v[devvp->v[devvp->c].c + 2]);
				return 0;
			}
			if (devvp->v[devvp->c].xv[devvp->v[devvp->c].c].u.xp->len != devvp->v[devvp->c].len) {
				(void) fprintf(stderr, "%s:%d: ", conffile_get_name(cf), conffile_get_lineno(cf));
				(void) fprintf(stderr, "Error: extent `%s' has size %" PRIu64 ", not %" PRIu64"\n", ep->sv.v[devvp->v[devvp->c].c + 2], devvp->v[devvp->c].xv[devvp->v[devvp->c].c].u.xp->len, devvp->v[devvp->c].len);
				return 0;
			}
			devvp->v[devvp->c].xv[devvp->v[devvp->c].c].type = DE_EXTENT;
			devvp->v[devvp->c].xv[devvp->v[devvp->c].c].size = devvp->v[devvp->c].xv[devvp->v[devvp->c].c].u.xp->len;
			devvp->v[devvp->c].xv[devvp->v[devvp->c].c].u.xp->used = 1;
		} else if ((devvp->v[devvp->c].xv[devvp->v[devvp->c].c].u.dp = find_device(devvp, ep->sv.v[devvp->v[devvp->c].c + 2])) != NULL) {
			if (devvp->v[devvp->c].xv[devvp->v[devvp->c].c].u.dp->used) {
				(void) fprintf(stderr, "%s:%d: ", conffile_get_name(cf), conffile_get_lineno(cf));
				(void) fprintf(stderr, "Error: device `%s' has already been used\n", ep->sv.v[devvp->v[devvp->c].c + 2]);
				return 0;
			}
			devvp->v[devvp->c].xv[devvp->v[devvp->c].c].type = DE_DEVICE;
			devvp->v[devvp->c].xv[devvp->v[devvp->c].c].u.dp->used = 1;
			devvp->v[devvp->c].xv[devvp->v[devvp->c].c].size = devvp->v[devvp->c].xv[devvp->v[devvp->c].c].u.dp->len;
		} else {
			(void) fprintf(stderr, "%s:%d: ", conffile_get_name(cf), conffile_get_lineno(cf));
			(void) fprintf(stderr, "Error: no extent or device found for `%s'\n", ep->sv.v[devvp->v[devvp->c].c + 2]);
			return 0;
		}
	}
	if (devvp->v[devvp->c].raid == 1) {
		/* check we have more than 1 device/extent */
		if (devvp->v[devvp->c].c < 2) {
			(void) fprintf(stderr, "%s:%d: ", conffile_get_name(cf), conffile_get_lineno(cf));
			(void) fprintf(stderr, "Error: device `%s' is specified as RAID1, but has only %d sub-devices/extents\n", devvp->v[devvp->c].dev, devvp->v[devvp->c].c);
			return 0;
		}
	}
	devvp->c += 1;
	return 1;
}

/* find a target by name */
static disc_target_t *
find_target(targv_t *tvp, char *s)
{
	int	i;

	for (i = 0 ; i < tvp->c ; i++) {
		if (strcmp(tvp->v[i].target, s) == 0) {
			return &tvp->v[i];
		}
	}
	return NULL;
}

/* allocate space for a new target */
static int
do_target(conffile_t *cf, targv_t *tvp, devv_t *devvp, extv_t *evp, ent_t *ep)
{
	disc_extent_t	*xp;
	disc_device_t	*dp;

	if (find_target(tvp, ep->sv.v[TARGET_NAME_COL]) != NULL) {
		(void) fprintf(stderr, "%s:%d: ", conffile_get_name(cf), conffile_get_lineno(cf));
		(void) fprintf(stderr, "Error: attempt to re-define target `%s'\n", ep->sv.v[TARGET_NAME_COL]);
		return 0;
	}
	ALLOC(disc_target_t, tvp->v, tvp->size, tvp->c, 14, 14, "do_target", exit(EXIT_FAILURE));
	if ((dp = find_device(devvp, ep->sv.v[TARGET_DEVICE_COL])) != NULL) {
		tvp->v[tvp->c].de.type = DE_DEVICE;
		tvp->v[tvp->c].de.u.dp = dp;
		tvp->v[tvp->c].target = strdup(ep->sv.v[TARGET_NAME_COL]);
		tvp->v[tvp->c].mask = strdup(ep->sv.v[TARGET_NETMASK_COL]);
		tvp->c += 1;
		return 1;
	}
	if ((xp = find_extent(evp, ep->sv.v[TARGET_DEVICE_COL])) != NULL) {
		tvp->v[tvp->c].de.type = DE_EXTENT;
		tvp->v[tvp->c].de.u.xp = xp;
		tvp->v[tvp->c].target = strdup(ep->sv.v[TARGET_NAME_COL]);
		tvp->v[tvp->c].mask = strdup(ep->sv.v[TARGET_NETMASK_COL]);
		tvp->c += 1;
		return 1;
	}
	(void) fprintf(stderr, "%s:%d: ", conffile_get_name(cf), conffile_get_lineno(cf));
	(void) fprintf(stderr, "Error: no device or extent found for `%s'\n", ep->sv.v[TARGET_DEVICE_COL]);
	return 0;
}

/* print an extent */
static void
pextent(disc_extent_t *ep, int indent)
{
	int	i;

	for (i = 0 ; i < indent ; i++) {
		(void) fputc('\t', stdout);
	}
	printf("%s:%s:%" PRIu64 ":%" PRIu64 "\n", ep->extent, ep->dev, ep->sacred, ep->len);
}

static void pdevice(disc_device_t *, int);

/* print information about an extent or a device */
static void
pu(disc_de_t *dep, int indent)
{
	switch(dep->type) {
	case DE_EXTENT:
		pextent(dep->u.xp, indent);
		break;
	case DE_DEVICE:
		pdevice(dep->u.dp, indent);
		break;
	}
}

/* print information about a device */
static void
pdevice(disc_device_t *dp, int indent)
{
	int	i;
	int	j;

	for (i = 0 ; i < indent ; i++) {
		(void) fputc('\t', stdout);
	}
	printf("%s:RAID%d\n", dp->dev, dp->raid);
	for (j = 0 ; j < dp->c ; j++) {
		pu(&dp->xv[j], indent + 1);
	}
}

/* print informnation about a target */
static void
ptarget(disc_target_t *tp, int indent)
{
	int	i;

	for (i = 0 ; i < indent ; i++) {
		(void) fputc('\t', stdout);
	}
	printf("%s:%s\n", tp->target, tp->mask);
	pu(&tp->de, indent + 1);
}

/* print all information */
static void
ptargets(targv_t *tvp)
{
	int	i;

	for (i = 0 ; i < tvp->c ; i++) {
		ptarget(&tvp->v[i], 0);
	}
}

static int
read_conf_file(const char *cf, targv_t *tvp, devv_t *dvp, extv_t *evp)
{
	conffile_t	conf;
	ent_t		e;

	(void) memset(&conf, 0x0, sizeof(conf));
	if (!conffile_open(&conf, cf, "r", " \t", "#")) {
		(void) fprintf(stderr, "Error: can't open `%s'\n", cf);
		return 0;
	}
	PRINT("Reading configuration from `%s'\n", cf);
	(void) memset(&e, 0x0, sizeof(e));
	while (conffile_getent(&conf, &e)) {
		if (strncmp(e.sv.v[0], "extent", 6) == 0) {
			do_extent(&conf, evp, &e);
		} else if (strncmp(e.sv.v[0], "device", 6) == 0) {
			do_device(&conf, dvp, evp, &e);
		} else if (strncmp(e.sv.v[0], "target", 6) == 0) {
			do_target(&conf, tvp, dvp, evp, &e);
		}
		e.sv.c = 0;
	}
	ptargets(tvp);
	(void) conffile_close(&conf);
	return 1;
}

/* write the pid to the pid file */
static void
write_pid_file(void)
{
	FILE	*fp;

	if ((fp = fopen(_PATH_ISCSI_PID_FILE, "w")) == NULL) {
		(void) fprintf(stderr, "Couldn't create pid file \"%s\": %s", _PATH_ISCSI_PID_FILE, strerror(errno)); 
	} else {
		fprintf(fp, "%ld\n", (long) getpid());
		fclose(fp);
	}
}

int
main(int argc, char **argv)
{
	const char	*cf;
	targv_t		tv;
	devv_t		dv;
	extv_t		ev;
	char            TargetName[1024];
	int		detach_me_harder;
	int             i;

	(void) memset(&g, 0x0, sizeof(g));
	(void) memset(&tv, 0x0, sizeof(tv));
	(void) memset(&dv, 0x0, sizeof(dv));
	(void) memset(&ev, 0x0, sizeof(ev));

	/* set defaults */
	(void) strlcpy(TargetName, DEFAULT_TARGET_NAME, sizeof(TargetName));
	detach_me_harder = 1;
	g.port = ISCSI_PORT;

	cf = _PATH_ISCSI_TARGETS;

	while ((i = getopt(argc, argv, "Db:f:p:t:v:")) != -1) {
		switch (i) {
		case 'D':
			detach_me_harder = 0;
			break;
		case 'b':
			device_set_var("blocklen", optarg);
			break;
		case 'f':
			cf = optarg;
			break;
		case 'p':
			g.port = (uint16_t) atoi(optarg);
			break;
		case 't':
			(void) strlcpy(TargetName, optarg, sizeof(TargetName));
			break;
		case 'v':
			if (strcmp(optarg, "net") == 0) {
				set_debug("net");
			} else if (strcmp(optarg, "iscsi") == 0) {
				set_debug("iscsi");
			} else if (strcmp(optarg, "scsi") == 0) {
				set_debug("scsi");
			} else if (strcmp(optarg, "all") == 0) {
				set_debug("all");
			}
			break;
		}
	}

	if (!read_conf_file(cf, &tv, &dv, &ev)) {
		(void) fprintf(stderr, "Error: can't open `%s'\n", cf);
		return EXIT_FAILURE;
	}

	(void) signal(SIGPIPE, SIG_IGN);

	g_main_pid = ISCSI_GETPID;

	if (tv.c == 0) {
		(void) fprintf(stderr, "No targets to initialise\n");
		return EXIT_FAILURE;
	}
	/* Initialize target */
	for (i = 0 ; i < tv.c ; i++) {
		if (target_init(&g, &tv, TargetName, i) != 0) {
			TRACE_ERROR("target_init() failed\n");
			exit(EXIT_FAILURE);
		}
	}

#ifdef HAVE_DAEMON
	/* if we are supposed to be a daemon, detach from controlling tty */
	if (detach_me_harder && daemon(0, 0) < 0) {
		TRACE_ERROR("daemon() failed\n");
		exit(EXIT_FAILURE);
	}
#endif

	/* write pid to a file */
	write_pid_file();
	
	/* Wait for connections */
	if (target_listen(&g) != 0) {
		TRACE_ERROR("target_listen() failed\n");
	}

	return EXIT_SUCCESS;
}
