/*
 * Copyright (c) 1997 Erez Zadok
 * Copyright (c) 1990 Jan-Simon Pendry
 * Copyright (c) 1990 Imperial College of Science, Technology & Medicine
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry at Imperial College, London.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      %W% (Berkeley) %G%
 *
 * $Id: get_args.c,v 1.1.1.2 1997/07/24 21:20:45 christos Exp $
 *
 */

/*
 * Argument decode
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amd.h>

/* include auto-generated version file */
#include <build_version.h>

char *conf_file = NULL;		/* default amd configuration file */
char *conf_tag = NULL;		/* default conf file tags to use */
int usage = 0;
int use_conf_file = 0;		/* use amd configuration file */
#ifdef MOUNT_TABLE_ON_FILE
char *mtab;
#endif /* MOUNT_TABLE_ON_FILE */
#ifdef DEBUG
int debug_flags = D_AMQ		/* Register AMQ */
		| D_DAEMON;	/* Enter daemon mode */
#endif /* DEBUG */


/*
 * Return the version string (static buffer!)
 */
char *
get_version_string(void)
{
  static char vers[1024];
  char tmpbuf[256];

  sprintf(vers, "%s\n%s\n%s\n%s\n",
	  "Copyright (c) 1997 Erez Zadok",
	  "Copyright (c) 1990 Jan-Simon Pendry",
	  "Copyright (c) 1990 Imperial College of Science, Technology & Medicine",
	  "Copyright (c) 1990 The Regents of the University of California.");
  sprintf(tmpbuf, "%s version %s (build %d).\n",
	  PACKAGE, VERSION, AMU_BUILD_VERSION);
  strcat(vers, tmpbuf);
  sprintf(tmpbuf, "Built by %s@%s on date %s.\n",
	  USER_NAME, HOST_NAME, CONFIG_DATE);
  strcat(vers, tmpbuf);
  sprintf(tmpbuf, "cpu=%s (%s-endian), arch=%s, karch=%s.\n",
	  cpu, endian, gopt.arch, gopt.karch);
  strcat(vers, tmpbuf);
  sprintf(tmpbuf, "full_os=%s, os=%s, osver=%s, vendor=%s.\n",
	  HOST_OS, gopt.op_sys, gopt.op_sys_ver, HOST_VENDOR);
  strcat(vers, tmpbuf);

  strcat(vers, "Map support for: ");
  mapc_showtypes(tmpbuf);
  strcat(vers, tmpbuf);
  strcat(vers, ".\nAMFS: ");
  ops_showamfstypes(tmpbuf);
  strcat(vers, tmpbuf);
  strcat(vers, ".\nFS: ");
  ops_showfstypes(tmpbuf);
  strcat(vers, tmpbuf);
  sprintf(tmpbuf, "Primary network: primnetname=\"%s\" (primnetnum=%s).\n",
	  PrimNetName, PrimNetNum);
  strcat(vers, tmpbuf);
  if (SubsNetName && !STREQ(SubsNetName, NO_SUBNET)) {
    sprintf(tmpbuf, "Subsidiary network: subsnetname=\"%s\" (subsnetnum=%s).\n",
	    SubsNetName, SubsNetNum);
  } else {
    sprintf(tmpbuf, "No Subsidiary network.\n");
  }
  strcat(vers, tmpbuf);

  /* finally return formed string */
  return vers;
}


void
get_args(int c, char *v[])
{
  int opt_ch;
  FILE *fp = stdin;

  while ((opt_ch = getopt(c, v, "nprvSa:c:d:h:k:l:o:t:w:x:y:C:D:F:T:H")) != EOF)

    switch (opt_ch) {

    case 'a':
      if (*optarg != '/') {
	fprintf(stderr, "%s: -a option must begin with a '/'\n",
		progname);
	exit(1);
      }
      gopt.auto_dir = optarg;
      break;

    case 'c':
      gopt.am_timeo = atoi(optarg);
      if (gopt.am_timeo <= 0)
	gopt.am_timeo = AM_TTL;
      break;

    case 'd':
      gopt.sub_domain = optarg;
      break;

    case 'h':
      plog(XLOG_USER, "-h: option ignored.  HOST_EXEC is not enabled.");
      break;

    case 'k':
      gopt.karch = optarg;
      break;

    case 'l':
      gopt.logfile = optarg;
      break;

    case 'n':
      gopt.flags |= CFM_NORMALIZE_HOSTNAMES;
      break;

    case 'o':
      gopt.op_sys_ver = optarg;
      break;

    case 'p':
     gopt.flags |= CFM_PRINT_PID;
      break;

    case 'r':
      gopt.flags |= CFM_RESTART_EXISTING_MOUNTS;
      break;

    case 't':
      /* timeo.retrans */
      {
	char *dot = strchr(optarg, '.');
	if (dot)
	  *dot = '\0';
	if (*optarg) {
	  gopt.afs_timeo = atoi(optarg);
	}
	if (dot) {
	  gopt.afs_retrans = atoi(dot + 1);
	  *dot = '.';
	}
      }
      break;

    case 'v':
      fputs(get_version_string(), stderr);
      exit(0);
      break;

    case 'w':
      gopt.am_timeo_w = atoi(optarg);
      if (gopt.am_timeo_w <= 0)
	gopt.am_timeo_w = AM_TTL_W;
      break;

    case 'x':
      usage += switch_option(optarg);
      break;

    case 'y':
#ifdef HAVE_MAP_NIS
      gopt.nis_domain = optarg;
#else /* not HAVE_MAP_NIS */
      plog(XLOG_USER, "-y: option ignored.  No NIS support available.");
#endif /* not HAVE_MAP_NIS */
      break;

    case 'C':
      gopt.cluster = optarg;
      break;

    case 'D':
#ifdef DEBUG
      usage += debug_option(optarg);
#else /* not DEBUG */
      fprintf(stderr, "%s: not compiled with DEBUG option -- sorry.\n", progname);
#endif /* not DEBUG */
      break;

    case 'S':
      gopt.flags |= CFM_NOSWAP;
      break;

    case 'F':
      conf_file = optarg;
      break;

    case 'T':
      conf_tag = optarg;
      break;

    case 'H':
      goto show_usage;
      break;

    default:
      usage = 1;
      break;
    }

  /* check if amd conf file used */
  if (conf_file) {
    fp = fopen(conf_file, "r");
    if (!fp) {
      perror(conf_file);
      exit(1);
    }
    yyin = fp;
    yyparse();
    fclose(fp);
    if (process_last_regular_map() != 0)
      exit(1);
    use_conf_file = 1;
  }

  /* make sure there are some default options defined */
  if (xlog_level_init == ~0) {
    switch_option("");
  }
#ifdef DEBUG
  usage += switch_option("debug");
#endif /* DEBUG */

#ifdef HAVE_MAP_LDAP
  /* ensure that if ldap_base is specified, that also ldap_hostports is */
  if (gopt.ldap_hostports && !gopt.ldap_base) {
    fprintf(stderr, "must specify both ldap_hostports and ldap_base\n");
    exit(1);
  }
#endif /* HAVE_MAP_LDAP */

  if (usage)
    goto show_usage;

  while (optind <= c - 2) {
    char *dir = v[optind++];
    char *map = v[optind++];
    char *opts = "";
    if (v[optind] && *v[optind] == '-')
      opts = &v[optind++][1];

    root_newmap(dir, opts, map, NULL);
  }

  if (optind == c) {
    /*
     * Append domain name to hostname.
     * sub_domain overrides hostdomain
     * if given.
     */
    if (gopt.sub_domain)
      hostdomain = gopt.sub_domain;
    if (*hostdomain == '.')
      hostdomain++;
    strcat(hostd, ".");
    strcat(hostd, hostdomain);

#ifdef MOUNT_TABLE_ON_FILE
# ifdef DEBUG
    if (debug_flags & D_MTAB)
      mtab = DEBUG_MNTTAB;
    else
# endif /* DEBUG */
      mtab = MNTTAB_FILE_NAME;
#else /* not MOUNT_TABLE_ON_FILE */
# ifdef DEBUG
    if (debug_flags & D_MTAB)
      dlog("-D mtab option ignored");
# endif /* DEBUG */
#endif /* not MOUNT_TABLE_ON_FILE */

    if (switch_to_logfile(gopt.logfile) != 0)
      plog(XLOG_USER, "Cannot switch logfile");

    /*
     * If the kernel architecture was not specified
     * then use the machine architecture.
     */
    if (gopt.karch == 0)
      gopt.karch = gopt.arch;

    if (gopt.cluster == 0)
      gopt.cluster = hostdomain;

    if (gopt.afs_timeo <= 0)
      gopt.afs_timeo = AFS_TIMEO;
    if (gopt.afs_retrans <= 0)
      gopt.afs_retrans = AFS_RETRANS;
    if (gopt.afs_retrans <= 0)
      gopt.afs_retrans = 3;	/* XXX */
    return;
  }

show_usage:
  fprintf(stderr,
	  "Usage: %s [-mnprvHS] [-a mnt_point] [-c cache_time] [-d domain]\n\
\t[-k kernel_arch] [-l logfile|\"syslog\"] [-t afs_timeout]\n\
\t[-w wait_timeout] [-C cluster_name]\n\
\t[-F conf_file] [-T conf_tag]", progname);

#ifdef HAVE_MAP_NIS
  fputs(" [-y nis-domain]\n", stderr);
#else /* not HAVE_MAP_NIS */
  fputc('\n', stderr);
#endif /* HAVE_MAP_NIS */

  show_opts('x', xlog_opt);
#ifdef DEBUG
  show_opts('D', dbg_opt);
#endif /* DEBUG */
  fprintf(stderr, "\t{directory mapname [-map_options]} ...\n");
  exit(1);
}
