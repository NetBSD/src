/*	$NetBSD: parse.c,v 1.14 2003/05/17 21:17:43 itojun Exp $	*/

/*
** parse.c                         This file contains the protocol parser
**
** This program is in the public domain and may be used freely by anyone
** who wants to.
**
** Last update: 23 Feb 1994
**
** Please send bug fixes/bug reports to: Peter Eriksson <pen@lysator.liu.se>
*/

#ifdef NeXT31
#  include <libc.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <pwd.h>
#ifdef ALLOW_FORMAT
#  include <grp.h>
#endif

#include <sys/types.h>
#include <netinet/in.h>

#ifndef HPUX7
#  include <arpa/inet.h>
#endif

#include <kvm.h>

#include <sys/types.h>
#include <sys/stat.h>

#if defined(MIPS) || defined(BSD43)
extern int errno;
#endif

#if defined(SOLARIS) || defined(__linux__)
#  include <string.h>
#  include <stdlib.h>
#endif

#include "identd.h"
#include "error.h"

static int eat_whitespace __P((void));
static int check_noident __P((const char *));
static int valid_fhost(struct in_addr *, char *);

/*
** This function will eat whitespace characters until
** either a non-whitespace character is read, or EOF
** occurs. This function is only used if the "-m" option
** is enabled.
*/
static int eat_whitespace()
{
  int c;


  while ((c = getchar()) != EOF &&
	 !(c == '\r' || c == '\n'))
    ;

  if (c != EOF)
    while ((c = getchar()) != EOF &&
	   (c == ' ' || c == '\t' || c == '\n' || c == '\r'))
      ;

  if (c != EOF)
    ungetc(c, stdin);

  return (c != EOF);
}


#ifdef INCLUDE_EXTENSIONS
/*
** Validate an indirect request
*/
static int valid_fhost(faddr, password)
  struct in_addr *faddr;
  char *password;
{
  if (indirect_host == NULL)
    return 0;

  if (strcmp(indirect_host, "*") != 0)
  {
    if (isdigit(indirect_host[0]))
    {
      if (strcmp(inet_ntoa(*faddr), indirect_host))
      {
	syslog(LOG_NOTICE, "valid_fhost: Access Denied for: %s",
	       gethost(faddr));
	return 0;
      }
    }
    else
    {
      if (strcmp(gethost(faddr), indirect_host))
      {
	syslog(LOG_NOTICE, "valid_fhost: Access Denied for: %s",
	       gethost(faddr));
	return 0;
      }
    }
  }

  if (indirect_password == NULL)
    return 1;

  if (strcmp(password, indirect_password))
  {
    syslog(LOG_NOTICE, "valid_fhost: Invalid password from: %s",
	   gethost(faddr));
    return 0;
  }

  return 1;
}
#endif

/*
** A small routine to check for the existance of the ".noident"
** file in a users home directory.
*/
static int check_noident(homedir)
  const char *homedir;
{
  char *tmp_path;
  struct stat sbuf;
  int rcode;

  if (!homedir)
    return 0;

  if (asprintf(&tmp_path, "%s/.noident", homedir) < 0)
    return 0;

  rcode = stat(tmp_path, &sbuf);
  free(tmp_path);

  return (rcode == 0);
}

#ifdef INCLUDE_CRYPT
/*
** Checks address of incoming call against network/mask pairs of trusted
** networks to determine whether to crypt response or not.
*/
int check_crypt(faddr)
  struct in_addr *faddr;
{
  int i;
  extern int netcnt;
  extern u_long localnet[], localmask[];
 
  for (i = 0; i < netcnt; i++) {
    if ((faddr->s_addr & localmask[i]) == localnet[i])
      return 0;
  }
  return 1;
}
#endif

int parse(fp, laddr, faddr)
  FILE *fp;
  struct in_addr *laddr, *faddr;
{
  int uid, try, rcode;
  struct passwd *pwp;
#ifdef ALLOW_FORMAT
  int pid;
  char *cmd, *cmd_and_args;
  struct group *grp;
  char grname[128];
#endif
  char lhostaddr[16];
  char fhostaddr[16];
  char password[33];
#if defined(INCLUDE_EXTENSIONS) || defined(STRONG_LOG)
  char arg[33];
#endif
#ifdef INCLUDE_EXTENSIONS
  int c;
#endif
  struct in_addr laddr2;
  struct in_addr faddr2;
  int k_opened;

  k_opened = 0;


  if (debug_flag && syslog_flag)
    syslog(LOG_DEBUG, "In function parse()");


  /*
  ** Get the local/foreign port pair from the luser
  */
  do
  {
    if (debug_flag && syslog_flag)
      syslog(LOG_DEBUG, "  Before fscanf()");

    faddr2 = *faddr;
    laddr2 = *laddr;
    lport = fport = 0;
    lhostaddr[0] = fhostaddr[0] = password[0] = '\0';


    /* Read query from client */
    rcode = fscanf(fp, " %d , %d", &lport, &fport);

    if (liar_flag)
    {
      if (syslog_flag)
        syslog(LOG_NOTICE, "User %s requested a user for host %s: %d, %d, and I lied",
               pwp->pw_name,
               gethost(faddr),
               lport, fport);

      printf("%d , %d : USERID : OTHER :%s\r\n",
           lport, fport, lie_string);
      continue;
    }

#ifdef INCLUDE_EXTENSIONS
    /*
    ** Do additional parsing in case of extended request
    */
    if (rcode == 0)
    {
      rcode = fscanf(fp, "%32[^ \t\n\r:]", arg);

      /* Skip leading space up to EOF, EOL or non-space char */
      while ((c = getc(fp)) == ' ' || c == '\t')
	;

      if (rcode <= 0)
      {
#ifdef STRONG_LOG
	if (syslog_flag)
	      syslog(LOG_NOTICE, "from: %s (%s) INVALID REQUEST",
		     inet_ntoa(*faddr), gethost(faddr));
#endif
	printf("%d , %d : ERROR : %s\r\n",
	       lport, fport,
	       unknown_flag ? "UNKNOWN-ERROR" : "X-INVALID-REQUEST");
	continue;
      }

      /*
      ** Non-standard extended request, returns with Pidentd
      ** version information
      */
      if (strcmp(arg, "VERSION") == 0)
      {
#ifdef STRONG_LOG
	  if (syslog_flag)
	      syslog(LOG_NOTICE, "from: %s (%s) VERSION REQUEST",
		     inet_ntoa(*faddr), gethost(faddr));
#endif
#if defined(__TIME__) && defined(__DATE__)
	printf("%d , %d : X-VERSION : %s (Compiled: %s %s)\r\n", lport, fport,
	       version, __TIME__, __DATE__);
#else
	printf("%d , %d : X-VERSION : %s\r\n", lport, fport,
	       version);
#endif
	continue;
      }

      /*
      ** Non-standard extended proxy request
      */
      else if (strcmp(arg, "PROXY") == 0 && c == ':')
      {
	/* We have a colon char, check for port numbers */
	rcode = fscanf(fp, " %d , %d : %15[0-9.] , %15[0-9.]",
		       &lport, &fport, fhostaddr, lhostaddr);

	if (!(rcode == 3 || rcode == 4))
	{
#ifdef STRONG_LOG
	    if (syslog_flag)
		syslog(LOG_NOTICE, "from: %s (%s) INVALID PROXY REQUEST",
		       inet_ntoa(*faddr), gethost(faddr));
#endif

	  printf("%d , %d : ERROR : %s\r\n",
		 lport, fport,
		 unknown_flag ? "UNKNOWN-ERROR" : "X-INVALID-REQUEST");
	  continue;
	}

	if (rcode == 4)
	  laddr2.s_addr = inet_addr(lhostaddr);

	faddr2.s_addr = inet_addr(fhostaddr);

#ifdef STRONG_LOG
	if (syslog_flag)
	{
	    char a1[64], a2[64], a3[64];

	    strlcpy(a1, inet_ntoa(*faddr), sizeof(a1));
	    strlcpy(a2, inet_ntoa(faddr2), sizeof(a2));
	    strlcpy(a3, inet_ntoa(laddr2), sizeof(a3));

	    syslog(LOG_NOTICE,
		   "from: %s (%s) PROXY REQUEST for %d, %d between %s and %s",
		   a1, gethost(faddr), lport, fport, a2, a3);
	}
#endif

	proxy(&laddr2, &faddr2, lport, fport, NULL);
	continue;
      }

      /*
      ** Non-standard extended remote indirect request
      */
      else if (strcmp(arg, "REMOTE") == 0 && c == ':')
      {
	/* We have a colon char, check for port numbers */
	rcode = fscanf(fp, " %d , %d", &lport, &fport);

	/* Skip leading space up to EOF, EOL or non-space char */
	while ((c = getc(fp)) == ' ' || c == '\t')
	  ;

	if (rcode != 2 || c != ':')
	{
#ifdef STRONG_LOG
	    if (syslog_flag)
		syslog(LOG_NOTICE, "from: %s (%s) INVALID REMOTE REQUEST",
		       inet_ntoa(*faddr), gethost(faddr));
#endif

	  printf("%d , %d : ERROR : %s\r\n",
		 lport, fport,
		 unknown_flag ? "UNKNOWN-ERROR" : "X-INVALID-REQUEST");
	  continue;
	}

	/* We have a colon char, check for addr and password */
	rcode = fscanf(fp, " %15[0-9.] , %32[^ \t\r\n]",
		       fhostaddr, password);
	if (rcode > 0)
	  rcode += 2;
	else
	{
#ifdef STRONG_LOG
	    if (syslog_flag)
		syslog(LOG_NOTICE,
		       "from: %s (%s) INVALID REMOTE REQUEST for %d, %d",
		       inet_ntoa(*faddr), gethost(faddr), lport, fport);
#endif
	    printf("%d , %d : ERROR : %s\r\n",
		   lport, fport,
		   unknown_flag ? "UNKNOWN-ERROR" : "X-INVALID-REQUEST");
	    continue;
	}

	/*
	** Verify that the host originating the indirect request
	** is allowed to do that
	*/
	if (!valid_fhost(faddr, password))
	{
#ifdef STRONG_LOG
	    if (syslog_flag)
		syslog(LOG_NOTICE,
       "from: %s (%s) REJECTED REMOTE REQUEST for %d, %d with password %s",
		       inet_ntoa(*faddr), gethost(faddr), lport, fport,
		       password);
#endif
	    printf("%d , %d : ERROR : %s\r\n",
		   lport, fport,
		   unknown_flag ? "UNKNOWN-ERROR" : "X-ACCESS-DENIED");
	    continue;
	}

	faddr2.s_addr = inet_addr(fhostaddr);
#ifdef STRONG_LOG
	if (syslog_flag)
	{
	    char a1[64];

	    strlcpy(a1, inet_ntoa(*faddr), sizeof(a1));

	    syslog(LOG_INFO,
	   "from: %s (%s) REMOTE REQUEST for %d, %d from %s with password %s",
		   a1, gethost(faddr), lport, fport,
		   inet_ntoa(faddr2), password);
	}
#endif
    }

      else
      {
#ifdef STRONG_LOG
	  if (syslog_flag)
	      syslog(LOG_NOTICE, "from: %s (%s) UNKNOWN REQUEST: %s",
		     inet_ntoa(*faddr), gethost(faddr), arg);
#endif

	  printf("%d , %d : ERROR : %s\r\n",
		 lport, fport,
		 unknown_flag ? "UNKNOWN-ERROR" : "X-INVALID-REQUEST");
	  continue;
      }
    }
#endif /* EXTENSIONS */

    if (rcode < 2 || lport < 1 || lport > 65535 || fport < 1 || fport > 65535)
    {
#ifdef STRONG_LOG
	if (syslog_flag)
	{
	    if (rcode > 0)
		/* we have scanned at least one correct port */
		syslog(LOG_NOTICE,
		       "from: %s (%s) for invalid-port(s): %d , %d",
		       inet_ntoa(*faddr), gethost(faddr), lport, fport);
	    else
	    {
		/* we have scanned nothing at all so try to get the rest */
		if (fscanf(fp, "%32[^\n\r]", arg) <= 0)
		    syslog(LOG_NOTICE, "from: %s (%s) EMPTY REQUEST",
			   inet_ntoa(*faddr), gethost(faddr));
		else
		    syslog(LOG_NOTICE, "from: %s (%s) INVALID REQUEST: %s",
			   inet_ntoa(*faddr), gethost(faddr), arg);
	    }
	}
#else
	if (syslog_flag && rcode > 0)
	    syslog(LOG_NOTICE, "scanf: invalid-port(s): %d , %d from %s",
		   lport, fport, gethost(faddr));
#endif

      printf("%d , %d : ERROR : %s\r\n",
	     lport, fport,
	     unknown_flag ? "UNKNOWN-ERROR" : "INVALID-PORT");
      continue;
    }

#ifdef STRONG_LOG
      if (syslog_flag)
      {
	  syslog(LOG_INFO, "from: %s ( %s ) for: %d, %d",
		 inet_ntoa(*faddr), gethost(faddr), lport, fport);
      }
#endif

    if (debug_flag && syslog_flag)
      syslog(LOG_DEBUG, "  After fscanf(), before k_open()");


    if (! k_opened)
    {
      /*
      ** Open the kernel memory device and read the nlist table
      ** 
      ** Of course k_open should not call ERROR (which then exits)
      ** but maybe use syslog(LOG_ERR) and return non-zero. But I am
      ** too lazy to change them all ...
      */
      if (k_open() != 0)
      {
	if (syslog_flag) syslog(LOG_ERR, "k_open call failed");
	printf("%d , %d : ERROR : %s\r\n",
	     lport, fport,
	     unknown_flag ? "UNKNOWN-ERROR" : "X-CANNOT-OPEN-KMEM");
	continue;
      }
      k_opened = 1;
    }


    if (debug_flag && syslog_flag)
      syslog(LOG_DEBUG, "  After k_open(), before k_getuid()");


    /*
    ** Get the specific TCP connection and return the uid - user number.
    */

#ifdef ALLOW_FORMAT
    /* Initialize values, for architectures that do not set it */
    pid = 0;
    cmd = "";
    cmd_and_args = "";
#endif

#define MAX_RETRY 20
    /*
    ** Try to fetch the information MAX_RETRY times in case the
    ** kernel changed beneath us and we missed or took a fault.
    **
    ** Why would we ever fail? Is not there a reliable way for the
    ** kernel to identify its sockets? Cannot we use that interface?
    **
    ** Used to be 5 times, but often this is not enough on Alpha OSF.
    */
/* #define SLEEP_BETWEEN_RETRIES 1 */
    /*
    ** If we failed in k_getuid, that is presumably because the OS was
    ** busy creating or destroying processes. We may want to sleep for
    ** a random time between retries, hoping for peace and quiet.
    */

/* k_getuid returns 0 on success, any non-zero on failure. */

    for (try = 0;
	 (try < MAX_RETRY &&
	   k_getuid(&faddr2, htons(fport), laddr, htons(lport), &uid
#ifdef ALLOW_FORMAT
		    , &pid, &cmd, &cmd_and_args
#endif
		    ) != 0);
	 try++)
#ifdef SLEEP_BETWEEN_RETRIES
      {
	/* Seed the generator: lport should be unique (among other concurrent identd's) */
	if (try < 1) srandom(lport);
	/* This gives a max sleep of 0xffff = 65535 microsecs, about 32millisec average */
	usleep(random()&0x00ffff);
      }
#else
      ;
#endif

    if (try >= MAX_RETRY)
    {
      if (syslog_flag)
	syslog(LOG_INFO, "Returned: %d , %d : NO-USER", lport, fport);

      printf("%d , %d : ERROR : %s\r\n",
	     lport, fport,
	     unknown_flag ? "UNKNOWN-ERROR" : "NO-USER");
      continue;
    }

    if (try > 0 && syslog_flag)
      syslog(LOG_NOTICE, "k_getuid retries: %d", try);

    if (debug_flag && syslog_flag)
      syslog(LOG_DEBUG, "  After k_getuid(), before getpwuid()");

    /*
    ** Then we should try to get the username. If that fails we
    ** return it as an OTHER identifier
    */
    pwp = getpwuid(uid);

    if (!pwp || uid != pwp->pw_uid)
    {
      if (syslog_flag)
	syslog(LOG_WARNING, "getpwuid() could not map uid (%d) to name",
	       uid);

      printf("%d , %d : USERID : OTHER%s%s : %d\r\n",
	     lport, fport,
	     charset_name ? " , " : "",
	     charset_name ? charset_name : "",
	     uid);
      continue;
    }

#ifdef ALLOW_FORMAT
    grp = getgrgid(pwp->pw_gid);
    if (grp && pwp->pw_gid != grp->gr_gid)
    {
	if (syslog_flag)
	    syslog(LOG_WARNING,
		   "getgrgid() could not map gid (%d) to name (for uid %d, name %s)",
		   pwp->pw_gid, uid, pwp->pw_name);

      printf("%d , %d : USERID : OTHER%s%s : %d\r\n",
	     lport, fport,
	     charset_name ? " , " : "",
	     charset_name ? charset_name : "",
	     uid);
      continue;
    }
    if (grp)
	snprintf(grname, sizeof(grname), "%.99s", grp->gr_name);
    else
	snprintf(grname, sizeof(grname), "%d", pwp->pw_gid);
#endif

    /*
    ** Hey! We finally made it!!!
    */
#ifdef ALLOW_FORMAT
    if (syslog_flag)
      syslog(LOG_DEBUG, "Successful lookup: %d , %d : %s.%s\n",
	     lport, fport, pwp->pw_name, grname);
#else
    if (syslog_flag)
      syslog(LOG_DEBUG, "Successful lookup: %d , %d : %s\n",
	     lport, fport, pwp->pw_name);
#endif

    if (noident_flag && check_noident(pwp->pw_dir))
    {
      if (syslog_flag)
	syslog(LOG_NOTICE, "User %s requested HIDDEN-USER for host %s: %d, %d",
	       pwp->pw_name,
	       gethost(faddr),
	       lport, fport);

      printf("%d , %d : ERROR : HIDDEN-USER\r\n",
	   lport, fport);
      continue;
    }

#ifdef INCLUDE_CRYPT
    if (crypto_flag && check_crypt(faddr))
      printf("%d , %d : USERID : OTHER%s%s : [%s]\r\n",
	     lport, fport,
	     charset_name ? " , " : "",
	     charset_name ? charset_name : "",
	     make_packet (pwp->pw_uid, laddr, lport, faddr, fport));
    else
#endif
#ifdef ALLOW_FORMAT
    if (format_flag)
    {
      char *cp;
      __aconst char *__aconst *gmp;
      int bp;
      char buff[512];
      for (cp = format, bp = 0; *cp != 0; cp++)
      {
	if (*cp == '%')
	{
	  cp++;
	  if (*cp == 0) break;
	  else if (*cp == 'u') snprintf(&buff[bp], sizeof(buff) - bp, "%.*s", 490-bp, pwp->pw_name);
	  else if (*cp == 'U') snprintf(&buff[bp], sizeof(buff) - bp, "%d",           pwp->pw_uid);
	  else if (*cp == 'g') snprintf(&buff[bp], sizeof(buff) - bp, "%.*s", 490-bp, grname);
	  else if (*cp == 'G') snprintf(&buff[bp], sizeof(buff) - bp, "%d",           pwp->pw_gid);
	  else if (*cp == 'c') snprintf(&buff[bp], sizeof(buff) - bp, "%.*s", 490-bp, cmd);
	  else if (*cp == 'C') snprintf(&buff[bp], sizeof(buff) - bp, "%.*s", 490-bp, cmd_and_args);
	  else if (*cp == 'l') {
	    snprintf(&buff[bp], sizeof(buff) - bp, "%.*s", 490-bp, grname);
	    bp += strlen(&buff[bp]); if (bp >= 490) break;
	    setgrent();
	    while ((grp = getgrent()) != NULL) {
	      if (grp->gr_gid == pwp->pw_gid) continue;
	      for (gmp = grp->gr_mem; *gmp && **gmp; gmp++) {
		if (! strcmp(*gmp, pwp->pw_name)) {
		  snprintf(&buff[bp], sizeof(buff) - bp, ",%.*s", 490-bp, grp->gr_name);
		  bp += strlen(&buff[bp]);
		  break;
		}
	      }
	      if (bp >= 490) break;
	    }
	    endgrent();
	  }
	  else if (*cp == 'L') {
	    snprintf(&buff[bp], sizeof(buff) - bp, "%d", pwp->pw_gid);
	    bp += strlen(&buff[bp]); if (bp >= 490) break;
	    setgrent();
	    while ((grp = getgrent()) != NULL) {
	      if (grp->gr_gid == pwp->pw_gid) continue;
	      for (gmp = grp->gr_mem; *gmp && **gmp; gmp++) {
		if (! strcmp(*gmp, pwp->pw_name)) {
		  snprintf(&buff[bp], sizeof(buff) - bp, ",%d", grp->gr_gid);
		  bp += strlen(&buff[bp]);
		  break;
		}
	      }
	      if (bp >= 490) break;
	    }
	    endgrent();
	  }
	  else if (*cp == 'p') snprintf(&buff[bp], sizeof(buff) - bp, "%d", pid);
	  else { buff[bp] = *cp; buff[bp+1] = 0; }
	  bp += strlen(&buff[bp]); if (bp >= 490) break;
	}
	else { buff[bp++] = *cp; if (bp >= 490) break; }
      }
      if (bp >= 490) {
        snprintf(&buff[490], sizeof(buff) - 490, "...");
        bp = 493;
      }
      buff[bp] = 0;
      printf("%d , %d : USERID : %s%s%s :%s\r\n",
	     lport, fport,
	     other_flag ? "OTHER" : "UNIX",
	     charset_name ? " , " : "",
	     charset_name ? charset_name : "",
	     buff);
    }
    else
#endif
      printf("%d , %d : USERID : %s%s%s :%s\r\n",
	     lport, fport,
	     other_flag ? "OTHER" : "UNIX",
	     charset_name ? " , " : "",
	     charset_name ? charset_name : "",
	     pwp->pw_name);

  } while(fflush(stdout), fflush(stderr), multi_flag && eat_whitespace());

  return 0;
}
