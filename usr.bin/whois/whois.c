/*	$NetBSD: whois.c,v 1.11.4.1 1999/12/27 18:37:18 wrstuden Exp $	*/

/*
 * RIPE version marten@ripe.net
 * many changes & networkupdate by david@ripe.net
 * cosmetics by steven@dante.org.uk --	gcc stopped complaining mostly,
 *					code is still messy, though.
 *
 * 1.15 94/09/07
 * 
 * 1.2  9705/02
 * "-v" option added; ambrose@ripe.net
 * "whois.ripe.net" replaced by "bsdbase.ripe.net";  ambrose@ripe.net
 * "bsdbase.ripe.net" replaced by "joshua.ripe.net"; marek@ripe.net 
 * "joshua.ripe.net" replaced by "whois.ripe.net"; roman@ripe.net 981105
 *
 * Copyright (c) 1980 Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 */

#include <sys/cdefs.h>
#if defined(sun) && defined(solaris)
#define SYSV
#endif

#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1980, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef RIPE
#ifndef lint
#if 0
static char sccsid[] = "@(#)whois.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: whois.c,v 1.11.4.1 1999/12/27 18:37:18 wrstuden Exp $");
#endif
#endif /* not lint */
#endif /* not RIPE */

#ifdef RIPE
#ifndef lint
char sccsid[] =
    "@(#)whois.c 5.11 (Berkeley) 3/2/91 - RIPE 1.15 94/09/07 marten@ripe.net";
#endif /* not lint */
#endif /* RIPE */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <err.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include <signal.h>

#if defined(SYSV)
#include	<crypt.h>
#endif /* SYSV */

#ifndef __NetBSD__
#ifdef __STDC__
extern int	getopt(int argc, char * const *argv, const char *optstring);
extern int	kill(pid_t pid, int sig);
extern FILE	*fdopen(int fildes, const char *type); 
extern int	gethostname(char *name, int namelen);
#else /* !__STDC__ */
extern int	gethostname();
#endif /* __STDC__ */
#endif /* __NetBSD__ */

#if defined(SYSV) || defined(__STDC__)

#define		index(s,c)		strchr((const char*)(s),(int)(c))
#define		rindex(s,c)		strrchr((const char*)(s),(int)(c))
#define		bzero(s,n)		memset((void*)s,0,(size_t)n)

#ifdef HASMEMMOVE
# define	bcopy(s,d,n)	memmove((void*)(d),(void*)(s),(size_t)(n))
#else
# define	bcopy(s,d,n)	memcpy((void*)(d),(void*)(s),(size_t)(n))
#endif /* HASMEMMOVE */

#endif /* SYSV || __STDC__ */

#ifdef GLIBC
typedef __u_short u_short;
typedef __caddr_t caddr_t;
#endif /* GLIBC */

/*

# the following defines can be used but are not fully functional anymore...
#
# CLEVER- Use a educated guess of the whereabouts of the nearest server
#	  This is done by taking the top-level domain of the current
#	  machine, and looking for a CNAME record for a server with name
#	  <top-level-domain>-whois.ripe.net
#	  If this machine does not exsist or the current machine's top-level
#	  domain could not be found,it will fall back to whois.ripe.net
#	  the default for this RIPE version of whois
#	  The CLEVER option implies the RIPE option.

# TOPDOMAIN=\"<top-level-domain>\"
#	- This option will fix the default host to be
#	  <top-level-domain>-whois.ripe.net, which may point to a secondary
#	  server inside your top-level domain. If there is no such secondary
#	  server, it will point to whois.ripe.net, the default. This option
#	  overrules the CLEVER option.
#	  The TOPDOMAIN option implies the RIPE option.

*/

#if defined(TOPDOMAIN) || defined(CLEVER)
#ifndef RIPE
#define RIPE
#endif /* !RIPE */
#endif /* TOPDOMAIN || CLEVER */

#if defined(RIPE) && !defined(__NetBSD__)
#include <sys/param.h>
#define NICHOST "whois.ripe.net"
#else
#define NICHOST "whois.networksolutions.com"
#endif

int main __P((int, char **));
static void usage __P((void));
static void closesocket __P((int, int));
static void termhandler __P((int));

void usage()
{
#ifdef RIPE
#ifdef NETWORKUPDATE
  (void)fprintf(stderr, "\nUsage: networkupdate [-46] [-h hostname] [-p port]");
#else
  (void)fprintf(stderr, "\nUsage: whois [-46aFLmMrSvR] [-h hostname] [-s sources] [-T types] [-i attr] keys\n");
  (void)fprintf(stderr, "       whois -t type");
  (void)fprintf(stderr, "       whois -v type");
#endif
#else
  (void)fprintf(stderr, "\nUsage: whois [-46] [-h hostname] [-p port] name ...");
#endif
  (void)fprintf(stderr, "\n\nWhere:\n\n");
  (void)fprintf(stderr, "-4                         Use IPv4 Only\n");
  (void)fprintf(stderr, "-6                         Use IPv6 Only\n");
#ifdef RIPE
#ifndef NETWORKUPDATE
  (void)fprintf(stderr, "-a                         search all databases\n");
  (void)fprintf(stderr, "-F                         fast raw output\n");
#endif
#endif
  (void)fprintf(stderr, "-h hostname                search alternate server\n");
#ifdef RIPE
#ifndef NETWORKUPDATE
  (void)fprintf(stderr, "-i [attr][[,attr] ... ]    do an inverse lookup for specified attributes\n");
  (void)fprintf(stderr, "-L                         find all Less specific matches\n");
  (void)fprintf(stderr, "-m                         find first level more specific matches\n");
  (void)fprintf(stderr, "-M                         find all More specific matches\n");
#endif
#endif
  (void)fprintf(stderr, "-p port                    port to connect to\n");
#ifdef RIPE
#ifndef NETWORKUPDATE
  (void)fprintf(stderr, "-r                         turn off recursive lookups\n");
  (void)fprintf(stderr, "-s source[[,source] ... ]  search databases with source 'source'\n");
  (void)fprintf(stderr, "-S                         tell server to leave out 'syntactic sugar'\n");
  (void)fprintf(stderr, "-t type                    requests template for object of type 'type'\n");
  (void)fprintf(stderr, "-v type                    requests verbose template for object of type 'type'\n");
  (void)fprintf(stderr, "-R                         force to show local copy of the domain object even if it contains referral\n"); 
  (void)fprintf(stderr, "-T type[[,type] ... ]      only look for objects of type 'type'\n\n");
  (void)fprintf(stderr, "Please note that most of these flags are NOT understood by\n");
  (void)fprintf(stderr, "non RIPE whois servers\n");
#endif
#endif
  (void)fprintf(stderr, "\n");

  exit(1);
}

int s;

void closesocket(s, child) 
int s, child;
{
  /* printf("close connection child=%i\n", child);  */

  close(s);

#ifdef NETWORKUPDATE
  if (child==0) {
     kill(getppid(), SIGTERM);
  }
#endif

  exit(0);

}

void termhandler(sig)
int sig;
{
  closesocket(s,1);
}   


#ifdef RIPE
#if defined(__STDC__) || defined(SYSV)
#define occurs(str,pat)		((int) strstr((str),(pat)))
#else /* !__STDC__ && !SYSV */
int occurs(str, pat)
     char *str, *pat;
{
  register char *point = str;
  
  while ((point=index(point, *pat)))
    {
      if (strncmp(point, pat, strlen(pat)) == 0)
	return(1);
      point++;
    }
  return(0);
}
#endif
#endif

int main(argc, argv)
     int argc;
     char **argv;
{
  extern char *optarg;
  extern int optind;
  FILE *sfi;
  FILE *sfo;
  int ch;
  struct addrinfo *dst, hints;
  int af=PF_UNSPEC;
  int error;
  char *host, *whoishost;
  int optp=0;
  char *optport="whois";
#ifdef DEBUG
  int verb=1;
#else /*DEBUG */
  int verb=0;
#endif
#ifdef RIPE
  int opthost=0;
#ifndef NETWORKUPDATE
  /* normal whois client */
  char *string;
  int alldatabases=0;
  int optsource=0, optrecur=0, optfast=0, opttempl=0, optverbose=0;
  int optobjtype=0, optsugar=0, optinverselookup=0, optgetupdates=0;
  int optL=0, optm=0, optM=0, optchanged=0, optnonreferral=0;
  char	*source=NULL, *templ=NULL, *verbose=NULL, *objtype=NULL,
	*inverselookup=NULL, *getupdates=NULL;
#else /* NETWORKUPDATE */
  /* networkupdate client */
  int prev;
  char domainname[64]; /* that's what sys/param.h says */
  struct passwd *passwdentry;
  int child;
#endif
#ifdef CLEVER
  int  myerror;
  char *mytoplevel;
  char *myhost;
#endif
#endif
  
#ifdef TOPDOMAIN
  host = strcat(TOPDOMAIN, "-whois.ripe.net");
#else
  host = NICHOST;
#endif
  
#ifdef RIPE
#ifdef NETWORKUPDATE
    while ((ch = getopt(argc, argv, "46h:p:")) != EOF)
#else
  while ((ch = getopt(argc, argv, "46acFg:h:i:LmMp:rs:SRt:T:v:")) != EOF)
#endif
#else
    while ((ch = getopt(argc, argv, "46h:p:")) != EOF)
#endif
      switch((char)ch) {
      case '4':
	af = PF_INET;
	break;
      case '6':
	af = PF_INET6;
	break;
      case 'h':
	host = optarg;
	opthost = 1;
	break;
      case 'p':
	optport=optarg;
        optp =1;
        break;	
#ifdef RIPE
#ifndef NETWORKUPDATE
      case 'a':
	alldatabases=1;
	break;
      case 'c':
        optchanged=1;
        break;
      case 'F':
	optfast = 1;
	break;
      case 'g':
        getupdates=optarg;
	optgetupdates=1;
	break;
      case 'i':
        inverselookup=optarg;
	optinverselookup = 1;
	break;
      case 'L':
	if (optM || optm) {
          fprintf(stderr, "Only one of -L, -m or -M allowed\n\n");
          usage();
        }
	optL=1;
	break;
      case 'm':
	if (optM || optL) {
	  fprintf(stderr, "Only one of -L, -m or -M allowed\n\n");
          usage();
	}
	optm=1;
	break;
      case 'M':
	if (optL || optm) {
	  fprintf(stderr, "Only one of -L, -m or -M allowed\n\n");
	  usage();
	}
	optM=1;
	break;
      
      case 's':
	source = optarg;
	optsource=1;
	break;
      case 'S':
	optsugar=1;
	break;
      case 'R':
	optnonreferral=1;
	break;
      case 'r':
	optrecur=1;
	break;
      case 't':
	 templ=optarg;
	 opttempl=1;
	 break;
      case 'v':
	 verbose=optarg;
	 optverbose=1;
	 break;
      case 'T':
	objtype=optarg;
	optobjtype=1;
	break;
      
#endif	
#endif
      case '?':
      default:
	usage();
      }
  argc -= optind;
  argv += optind;

#ifdef RIPE
#ifdef NETWORKUPDATE
  if (argc>0)
    usage();
#else
  if ((argc<=0) && !opttempl && !optverbose && !optgetupdates && (!(opttempl && optgetupdates)))
    usage();
#endif
#else
  if (argc<=0)
    usage();
#endif

  if (!opthost) {
    
#ifdef CLEVER
    whoishost=(char *)calloc(MAXHOSTNAMELEN, sizeof(char));
    myhost =(char *)calloc(MAXHOSTNAMELEN, sizeof(char));
    myerror = gethostname(myhost, MAXHOSTNAMELEN);
    if (myerror >= 0) {
      if (occurs(myhost, ".")) {
	mytoplevel = rindex(myhost,'.');
	mytoplevel++;
	(void) sprintf(whoishost, "%s-whois.ripe.net", mytoplevel);
	if (verb) fprintf(stderr, "Clever guess: %s\n", whoishost);
      }
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = af;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    error = getaddrinfo(host, optport, &hints, &dst);
    if ((error) && (verb))
      fprintf(stderr,"No such host: %s\n", whoishost);
    if (error) {
#endif
    
      whoishost=NICHOST;
    
      if (verb)
	fprintf(stderr, "Default host: %s\n\n", whoishost);
      memset(&hints, 0, sizeof(hints));
      hints.ai_flags = AI_CANONNAME;
      hints.ai_family = af;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_protocol = 0;
      error = getaddrinfo(host, optport , &hints, &dst);
      if (error) {
	fprintf(stderr,"No such host: %s\n", whoishost);
	if (verb) fprintf(stderr, "Now I give up ...\n");
	perror("Unknown host");
	exit(1);	
      }

#ifdef CLEVER
    }
#endif
  }
  else {
    if (verb)
      fprintf(stderr, "Trying: %s\n\n", host);
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = af;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    error = getaddrinfo(host, optport, &hints, &dst);
    if (error) {
      (void)fprintf(stderr, "whois: %s: ", host);
      perror("Unknown host");
      exit(1);
    }
  }
  
  for (/*nothing*/; dst; dst = dst->ai_next) {
    s = socket(dst->ai_family, dst->ai_socktype, dst->ai_protocol);
    if (s < 0)
      continue;
    if (connect(s, dst->ai_addr, dst->ai_addrlen) < 0) {
      close(s);
      if (verb) (void)fprintf(stderr, "whois: connect miss\n");
      continue;
    }
    /*okay*/
    break;
  }
  if (dst == NULL) {
    perror("whois: connect");
    exit(1);
  }
  if (verb) (void)fprintf(stderr, "whois: connect success\n");

#ifndef NETWORKUPDATE
  sfi = fdopen(s, "r");
  sfo = fdopen(s, "w");
  if (sfi == NULL || sfo == NULL) {
    perror("whois: fdopen");
    (void)close(s);
    exit(1);
  }
#endif

  signal(SIGTERM, termhandler);  

#ifdef RIPE
#ifdef NETWORKUPDATE

  if ((child=fork())==0) {
     
     sfo = fdopen(s, "w");
     if (sfo == NULL) {
       perror("whois: fdopen");
       (void)close(s);
       exit(1);
     }

     if (gethostname(domainname, sizeof(domainname))) {
        fprintf(stderr, "error when doing gethostname()");
        exit(-1);
     }

     passwdentry=getpwuid(getuid());

     fprintf(sfo, "-Vnc2.0 -U %s %s\n", passwdentry->pw_name, domainname);
     fflush(sfo);  
     
     prev='\0';

     while ((ch=getchar()) != EOF) {
        
        fputc(ch, sfo);
        
        if (ch=='\n') fflush(sfo);
        if (feof(sfo)) closesocket(s, child);
        if ((ch=='.') && (prev=='\n')) closesocket(s, child);
        if (!isspace(ch) || ((!isspace(prev)) && (ch=='\n'))) prev=ch;
     }

     closesocket(s, child);

  }
  
  sfi = fdopen(s, "r");
  if (sfi == NULL) {
       perror("whois: fdopen");
       (void)close(s);
       exit(1);
  } 

#else
  
  if (alldatabases)
    (void)fprintf(sfo, "-a ");
  if (optchanged)
    (void)fprintf(sfo, "-c ");
  if (optfast)
    (void)fprintf(sfo, "-F ");
  if (optgetupdates)
    (void)fprintf(sfo, "-g %s ", getupdates);  
  if (optinverselookup)
    (void)fprintf(sfo, "-i %s ", inverselookup);
  if (optL)
    (void)fprintf(sfo, "-L ");
  if (optm)
    (void)fprintf(sfo, "-m ");
  if (optM)
    (void)fprintf(sfo, "-M ");
  if (optrecur)
    (void)fprintf(sfo, "-r ");
  if (optsource)
    (void)fprintf(sfo, "-s %s ", source);
  if (optsugar)
    (void)fprintf(sfo, "-S ");
  if (optnonreferral) 
    (void)fprintf(sfo, "-R ");
  if (opttempl)
    (void)fprintf(sfo, "-t %s ", templ);
  if (optverbose)
    (void)fprintf(sfo, "-v %s ", verbose);
  if (optobjtype)
    (void)fprintf(sfo, "-T %s ", objtype);

  /* we can only send the -V when we are sure that we are dealing with 
     a RIPE whois server :-( */
  
  whoishost=(char *)calloc(strlen(host)+1, sizeof(char));
  strcpy(whoishost, host);
  for (string=whoishost;(*string=(char)tolower(*string));string++);
  
  if (strstr(whoishost, "ripe.net") ||
      strstr(whoishost, "ra.net") ||
      strstr(whoishost, "apnic.net") ||
      strstr(whoishost, "mci.net") ||      
      strstr(whoishost, "isi.edu") ||
      strstr(whoishost, "garr.it") ||
      strstr(whoishost, "ans.net") ||
      alldatabases || optfast || optgetupdates || optinverselookup ||
      optL || optm || optM || optrecur || optsugar || optsource ||
      opttempl || optverbose || optobjtype)
    (void)fprintf(sfo, "-VwC2.0 ");
#endif
#endif

#ifndef NETWORKUPDATE
  while (argc-- > 1)
    (void)fprintf(sfo, "%s ", *argv++);
   if (*argv) (void)fputs(*argv, sfo);
   (void)fputs("\r\n", sfo);
  (void)fflush(sfo);
#endif
  
  while ((ch = getc(sfi)) != EOF)
    putchar(ch);

  closesocket(s, 1);

  exit(0);

}
