/*	$NetBSD: supcparse.c,v 1.16 2013/04/09 16:39:20 christos Exp $	*/

/*
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * sup collection parsing routines
 **********************************************************************
 * HISTORY
 *
 * 7-July-93  Nate Williams at Montana State University
 *	Modified SUP to use gzip based compression when sending files
 *	across the network to save BandWidth
 *
 * Revision 1.6  92/08/11  12:07:38  mrt
 * 	Added use-rel-suffix option corresponding to -u switch.
 * 	[92/07/26            mrt]
 *
 * Revision 1.5  92/02/08  18:24:19  mja
 * 	Added "keep" supfile option, corresponding to -k switch.
 * 	[92/01/17            vdelvecc]
 *
 * Revision 1.4  91/05/16  14:49:50  ern
 * 	Change default timeout from none to 3 hours so we don't accumalute
 * 	processes running sups to dead hosts especially for users.
 * 	[91/05/16  14:49:21  ern]
 *
 *
 * 10-Feb-88  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added timeout to backoff.
 *
 * 28-Jun-87  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code for "release" support.  Removed obsolete options.
 *
 * 25-May-87  Doug Philips (dwp) at Carnegie-Mellon University
 *	Split off from sup.c
 *
 **********************************************************************
 */

#include "supcdefs.h"
#include "supextern.h"


extern char _argbreak;		/* break character from nxtarg */

typedef enum {			/* supfile options */
	OHOST, OBASE, OHOSTBASE, OPREFIX, ORELEASE,
	ONOTIFY, OLOGIN, OPASSWORD, OCRYPT,
	OBACKUP, ODELETE, OEXECUTE, OOLD, OTIMEOUT, OKEEP, OURELSUF,
	OCOMPRESS, OCANONICALIZE, OIGNCHERR,
}    OPTION;

struct option {
	char *op_name;
	OPTION op_enum;
}      options[] = {
	{ "host", OHOST },
	{ "base", OBASE },
	{ "hostbase", OHOSTBASE },
	{ "prefix", OPREFIX },
	{ "release", ORELEASE },
	{ "notify", ONOTIFY },
	{ "login", OLOGIN },
	{ "password", OPASSWORD },
	{ "crypt", OCRYPT },
	{ "backup", OBACKUP },
	{ "delete", ODELETE },
	{ "execute", OEXECUTE },
	{ "old", OOLD },
	{ "timeout", OTIMEOUT },
	{ "keep", OKEEP },
	{ "use-rel-suffix", OURELSUF },
	{ "compress", OCOMPRESS },
	{ "canonicalize", OCANONICALIZE },
	{ "igncherr", OIGNCHERR },
};

static void passdelim(char **, char);

static void 
passdelim(char **ptr, char delim)
{				/* skip over delimiter */
	*ptr = skipover(*ptr, " \t");
	if (_argbreak != delim && **ptr == delim) {
		(*ptr)++;
		*ptr = skipover(*ptr, " \t");
	}
}

int 
parsecoll(COLLECTION * c, char *collname, char *args)
{
	char *arg, *p;
	OPTION option;
	int opno;

	c->Cnext = NULL;
	c->Cname = estrdup(collname);
	c->Chost = NULL;
	c->Chtree = NULL;
	c->Cbase = NULL;
	c->Chbase = NULL;
	c->Cprefix = NULL;
	c->Crelease = NULL;
	c->Cnotify = NULL;
	c->Clogin = NULL;
	c->Cpswd = NULL;
	c->Ccrypt = NULL;
	c->Ctimeout = 3 * 60 * 60;	/* default to 3 hours instead of no
					 * timeout */
	c->Cflags = 0;
	c->Cnogood = FALSE;
	c->Clockfd = -1;
	args = skipover(args, " \t");
	while (*(arg = nxtarg(&args, " \t="))) {
		for (opno = 0; opno < sizeofA(options); opno++)
			if (strcmp(arg, options[opno].op_name) == 0)
				break;
		if (opno == sizeofA(options)) {
			logerr("Invalid supfile option %s for collection %s",
			    arg, c->Cname);
			return (-1);
		}
		option = options[opno].op_enum;
		switch (option) {
		case OHOST:
			passdelim(&args, '=');
			do {
				arg = nxtarg(&args, ", \t");
				(void) Tinsert(&c->Chtree, arg, FALSE);
				arg = args;
				p = skipover(args, " \t");
				if (*p++ == ',')
					args = p;
			} while (arg != args);
			break;
		case OBASE:
			passdelim(&args, '=');
			arg = nxtarg(&args, " \t");
			c->Cbase = estrdup(arg);
			break;
		case OHOSTBASE:
			passdelim(&args, '=');
			arg = nxtarg(&args, " \t");
			c->Chbase = estrdup(arg);
			break;
		case OPREFIX:
			passdelim(&args, '=');
			arg = nxtarg(&args, " \t");
			c->Cprefix = estrdup(arg);
			break;
		case ORELEASE:
			passdelim(&args, '=');
			arg = nxtarg(&args, " \t");
			c->Crelease = estrdup(arg);
			break;
		case ONOTIFY:
			passdelim(&args, '=');
			arg = nxtarg(&args, " \t");
			c->Cnotify = estrdup(arg);
			break;
		case OLOGIN:
			passdelim(&args, '=');
			arg = nxtarg(&args, " \t");
			c->Clogin = estrdup(arg);
			break;
		case OPASSWORD:
			passdelim(&args, '=');
			arg = nxtarg(&args, " \t");
			c->Cpswd = estrdup(arg);
			break;
		case OCRYPT:
			passdelim(&args, '=');
			arg = nxtarg(&args, " \t");
			c->Ccrypt = estrdup(arg);
			break;
		case OBACKUP:
			c->Cflags |= CFBACKUP;
			break;
		case ODELETE:
			c->Cflags |= CFDELETE;
			break;
		case OEXECUTE:
			c->Cflags |= CFEXECUTE;
			break;
		case OOLD:
			c->Cflags |= CFOLD;
			break;
		case OKEEP:
			c->Cflags |= CFKEEP;
			break;
		case OURELSUF:
			c->Cflags |= CFURELSUF;
			break;
		case OCOMPRESS:
			c->Cflags |= CFCOMPRESS;
			break;
		case OCANONICALIZE:
			c->Cflags |= CFCANONICALIZE;
			break;
		case OTIMEOUT:
			passdelim(&args, '=');
			arg = nxtarg(&args, " \t");
			c->Ctimeout = atoi(arg);
			break;
		case OIGNCHERR:
			c->Cflags |= CFIGNCHERR;
			break;
		}
	}
	return (0);
}


time_t
getwhen(char *collection, char *relsuffix)
{
	char buf[STRINGLENGTH];
	char *ep;
	FILE *fp;
	time_t tstamp;

	(void) sprintf(buf, FILEWHEN, collection, relsuffix);

	if ((fp = fopen(buf, "r")) == NULL)
		return 0;

	if (fgets(buf, sizeof(buf), fp) == NULL) {
		(void) fclose(fp);
		return 0;
	}
	(void) fclose(fp);

	if ((tstamp = strtol(buf, &ep, 0)) == -1 || *ep != '\n')
		return 0;

	return tstamp;
}

int
putwhen(char *fname, time_t tstamp)
{
	FILE *fp;
	if ((fp = fopen(fname, "w")) == NULL)
		return 0;
	if (fprintf(fp, "%lu\n", (u_long) tstamp) < 0) {
		(void) fclose(fp);
		return 0;
	}
	if (fclose(fp) != 0)
		return 0;
	return 1;
}
