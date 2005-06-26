/* $NetBSD: vext.h,v 1.3 2005/06/26 22:36:55 christos Exp $ */

/*-
 * Copyright (c) 1997, 1998
 *	Nan Yang Computer Services Limited.  All rights reserved.
 *
 *  This software is distributed under the so-called ``Berkeley
 *  License'':
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
 *	This product includes software developed by Nan Yang Computer
 *      Services Limited.
 * 4. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * This software is provided ``as is'', and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even if
 * advised of the possibility of such damage.
 */

/*
 * $Id: vext.h,v 1.3 2005/06/26 22:36:55 christos Exp $
 * $FreeBSD$
 */

#define MAXARGS 64					    /* maximum number of args on a line */
#define PLEXINITSIZE 65536				    /* init in this size chunks */
#define MAXPLEXINITSIZE 65536				    /* max chunk size to use for init */
#define MAXDATETEXT 128					    /* date text in history (far too much) */
#define VINUMDEBUG					    /* for including kernel headers */

enum {
    KILOBYTE = 1024,
    MEGABYTE = 1048576,
    GIGABYTE = 1073741824
};

#define VINUMMOD "vinum"

#define DEFAULT_HISTORYFILE "/var/log/vinum_history"	    /* default name for history stuff */

#include <sys/cdefs.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <netdb.h>
#include <paths.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <grp.h>
#include <readline/history.h>
#include <readline/readline.h>

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/dkstat.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/uio.h>
#include <sys/namei.h>
#include <sys/stat.h>
#include <sys/disklabel.h>
#include <sys/syslog.h>
#include <sys/fcntl.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <dev/vinum/vinumvar.h>
#include <dev/vinum/vinumio.h>
#include <dev/vinum/vinumkw.h>
#include <dev/vinum/vinumutil.h>
#include <machine/cpu.h>

/* Prototype declarations */
void parseline(int, char *[]);			    /* parse a line with c parameters at args */
void checkentry(int);
int haveargs(int);					    /* check arg, error message if not valid */
void setsigs(void);
void catchsig(int);
void vinum_create(int, char *[], char *[]);
void vinum_read(int, char *[], char *[]);
void vinum_modify(int, char *[], char *[]);
void vinum_volume(int, char *[], char *[]);
void vinum_plex(int, char *[], char *[]);
void vinum_sd(int, char *[], char *[]);
void vinum_drive(int, char *[], char *[]);
void vinum_list(int, char *[], char *[]);
void vinum_info(int, char *[], char *[]);
void vinum_set(int, char *[], char *[]);
void vinum_rm(int, char *[], char *[]);
void vinum_mv(int, char *[], char *[]);
void vinum_init(int, char *[], char *[]);
void listconfig(void);
void resetstats(struct vinum_ioctl_msg *);
void initvol(int);
void initplex(int, char *);
void initsd(int, int);
void vinum_resetconfig(int, char *[], char *[]);
void vinum_start(int, char *[], char *[]);
void continue_revive(int);
void vinum_stop(int, char *[], char *[]);
void vinum_makedev(int, char *[], char *[]);
void vinum_help(int, char *[], char *[]);
void vinum_quit(int, char *[], char *[]);
void vinum_setdaemon(int, char *[], char *[]);
void vinum_replace(int, char *[], char *[]);
void vinum_readpol(int, char *[], char *[]);
void reset_volume_stats(int, int);
void reset_plex_stats(int, int);
void reset_sd_stats(int, int);
void reset_drive_stats(int);
void vinum_resetstats(int, char *[], char *[]);
void vinum_attach(int, char *[], char *[]);
void vinum_detach(int, char *[], char *[]);
void vinum_rename(int, char *[], char *[]);
void vinum_rename_2(char *, char *);
void vinum_replace(int, char *[], char *[]);
void vinum_printconfig(int, char *[], char *[]);
void printconfig(FILE *, const char *);
void vinum_saveconfig(int, char *[], char *[]);
int checkupdates(void);
void genvolname(void);
struct _drive *create_drive(char *);
void vinum_concat(int, char *[], char *[]);
void vinum_stripe(int, char *[], char *[]);
void vinum_raid4(int, char *[], char *[]);
void vinum_raid5(int, char *[], char *[]);
void vinum_mirror(int, char *[], char *[]);
void vinum_label(int, char *[], char *[]);
void vinum_ld(int, char *[], char *[]);
void vinum_ls(int, char *[], char *[]);
void vinum_lp(int, char *[], char *[]);
void vinum_lv(int, char *[], char *[]);
void vinum_setstate(int, char *[], char *[]);
void vinum_checkparity(int, char *[], char *[]);
void vinum_rebuildparity(int, char *[], char *[]);
void parityops(int, char *[], enum parityop);
void start_daemon(void);
void vinum_debug(int, char *[], char *[]);
void make_devices(void);
void make_vol_dev(int, int);
void make_plex_dev(int, int);
void make_sd_dev(int);
void list_defective_objects(void);
void vinum_dumpconfig(int, char *[], char *[]);
void dumpconfig(char *);
int check_drive(char *);
void get_drive_info(struct _drive *, int);
void get_sd_info(struct _sd *, int);
void get_plex_sd_info(struct _sd *, int, int);
void get_plex_info(struct _plex *, int);
void get_volume_info(struct _volume *, int);
struct _drive *find_drive_by_devname(char *);
int find_object(const char *, enum objecttype *);
char *lltoa(int64_t, char *);
void vinum_ldi(int, int);
void vinum_lvi(int, int);
void vinum_lpi(int, int);
void vinum_lsi(int, int);
int vinum_li(int, enum objecttype);
char *roughlength(int64_t, int);
u_int64_t sizespec(char *);
char *sd_state(enum sdstate);
void timestamp(void);

extern int force;					    /* set to 1 to force some dangerous ops */
extern int interval;					    /* interval in ms between init/revive */
extern int vflag;					    /* set verbose operation or verify */
extern int Verbose;					    /* very verbose operation */
extern int recurse;					    /* set recursion */
extern int sflag;					    /* show statistics */
extern int SSize;					    /* sector size for revive */
extern int dowait;					    /* wait for children to exit */
extern char *objectname;				    /* name for some functions */

extern FILE *History;					    /* history file */

/* Structures to read kernel data into */
extern struct __vinum_conf vinum_conf;			    /* configuration information */

extern struct _volume vol;
extern struct _plex plex;
extern struct _sd sd;
extern struct _drive drive;

extern jmp_buf command_fail;				    /* return on a failed command */
extern int superdev;					    /* vinum super device */
extern int no_devfs;					    /* set if we don't have DEVFS */

extern int line;					    /* stdin line number for error messages */
extern int file_line;					    /* and line in input file (yes, this is tacky) */

extern char buffer[];					    /* buffer to read in to */

#define min(a, b) a < b? a: b
