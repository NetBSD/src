/*
 * Copyright (c) 1997, 2000 Hellmuth Michaelis. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *---------------------------------------------------------------------------
 *
 *	exec.h - supplemental program/script execution
 *	----------------------------------------------
 *
 *	$Id: exec.c,v 1.10.6.1 2012/04/17 00:09:47 yamt Exp $ 
 *
 * $FreeBSD$
 *
 *      last edit-date: [Wed Sep 27 09:39:22 2000]
 *
 *---------------------------------------------------------------------------*/

#include "isdnd.h"

#include <sys/wait.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_PIDS 32

static struct pid_tab {
	pid_t	pid;
	struct cfg_entry *cep;
} pid_tab[MAX_PIDS];

/*---------------------------------------------------------------------------*
 *	SIGCHLD signal handler
 *---------------------------------------------------------------------------*/
void
sigchild_handler(int sig)
{
	int retstat;
	register int i;
	pid_t pid;
	
	if ((pid = waitpid(-1, &retstat, WNOHANG)) <= 0)
	{
		logit(LL_ERR, "ERROR, sigchild_handler, waitpid: %s", strerror(errno));
		error_exit(1, "ERROR, sigchild_handler, waitpid: %s", strerror(errno));
	}
	else
	{
		if (WIFEXITED(retstat))
		{
			DBGL(DL_PROC, (logit(LL_DBG, "normal child (pid=%d) termination, exitstat = %d",
				pid, WEXITSTATUS(retstat))));
		}
		else if (WIFSIGNALED(retstat))
		{
			if (WCOREDUMP(retstat))
				logit(LL_WRN, "child (pid=%d) termination due to signal %d (coredump)",
					pid, WTERMSIG(retstat));
			else
				logit(LL_WRN, "child (pid=%d) termination due to signal %d",
					pid, WTERMSIG(retstat));
		}
	}

	/* check if hangup required */
	
	for (i=0; i < MAX_PIDS; i++)
	{
		if (pid_tab[i].pid == pid)
		{
			if (pid_tab[i].cep->cdid != CDID_UNUSED)
			{
				DBGL(DL_PROC, (logit(LL_DBG, "sigchild_handler: scheduling hangup for cdid %d, pid %d",
					pid_tab[i].cep->cdid, pid_tab[i].pid)));
				pid_tab[i].cep->hangup = 1;
			}
			pid_tab[i].pid = 0;
			break;
		}
	}
}

/*---------------------------------------------------------------------------*
 *	execute prog as a subprocess and pass an argumentlist
 *---------------------------------------------------------------------------*/
pid_t
exec_prog(const char *prog, const char **arglist)
{
	char tmp[MAXPATHLEN];
	char path[MAXPATHLEN+1];
	pid_t pid;
	int a;

	snprintf(path, sizeof(path), "%s/%s", ETCPATH, prog);

	arglist[0] = path;

	tmp[0] = '\0';

	for (a=1; arglist[a] != NULL; ++a )
	{
		strlcat(tmp, " ", sizeof(tmp));
		strlcat(tmp, arglist[a], sizeof(tmp));
	}

	DBGL(DL_PROC, (logit(LL_DBG, "exec_prog: %s, args:%s", path, tmp)));
	
	switch (pid = fork())
	{
	case -1:		/* error */
		logit(LL_ERR, "ERROR, exec_prog/fork: %s", strerror(errno));
		error_exit(1, "ERROR, exec_prog/fork: %s", strerror(errno));
	case 0:			/* child */
		break;
	default:		/* parent */
		return(pid);
	}

	/* this is the child now */

	/*
	 * close files used only by isdnd, e.g.
	 * 1. /dev/isdn
	 * 2. /var/log/isdnd.acct (or similar, when used)
	 * 3. /var/log/isdnd.log (or similar, when used)
	 */
	close(isdnfd);
	if (useacctfile)
		fclose(acctfp);
	if (uselogfile)
		fclose(logfp);
	

	if (execvp(path, __UNCONST(arglist)) < 0 )
		_exit(127);

	return(-1);
}

/*---------------------------------------------------------------------------*
 *	run interface up/down script
 *---------------------------------------------------------------------------*/
int
exec_connect_prog(struct cfg_entry *cep, const char *prog, int link_down)
{
	const char *argv[32], **av = argv;
	char devicename[MAXPATHLEN], addr[100];
	int s;
	struct ifreq ifr;

	/* the obvious things */
	snprintf(devicename, sizeof(devicename), "%s%d", cep->usrdevicename, cep->usrdeviceunit);
	*av++ = prog;
	*av++ = "-d";
	*av++ = devicename;
	*av++ = "-f";
	*av++ = link_down ? "down" : "up";

	/* try to figure AF_INET address of interface */
	addr[0] = '\0';
	memset(&ifr, 0, sizeof ifr);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, devicename, sizeof(ifr.ifr_name));
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s >= 0) {
		if (ioctl(s, SIOCGIFADDR, (caddr_t)&ifr) >= 0) {
			struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
			strlcpy(addr, inet_ntoa(sin->sin_addr), sizeof(addr));
			*av++ = "-a";
			*av++ = addr;
		}
		close(s);
	}

	/* terminate argv */
	*av++ = NULL;

	return exec_prog(prog, argv);
}

/*---------------------------------------------------------------------------*
 *	run answeringmachine application
 *---------------------------------------------------------------------------*/
int
exec_answer(struct cfg_entry *cep)
{
	const char *argv[32];
	char devicename[MAXPATHLEN];	
	int pid;

	snprintf(devicename, sizeof(devicename), "/dev/%s%d", cep->usrdevicename, cep->usrdeviceunit);

	argv[0] = cep->answerprog;
	argv[1] = "-D";
	argv[2] = devicename;
	argv[3] = "-d";
	argv[4] = "unknown";
	argv[5] = "-s";
	argv[6] = "unknown";
	argv[7] = NULL;

	/* if destination telephone number avail, add it as argument */
	
	if (*cep->local_phone_incoming)
		argv[4] = cep->local_phone_incoming;

	/* if source telephone number avail, add it as argument */
	
	if (*cep->real_phone_incoming)
		argv[6] = cep->real_phone_incoming;

	if (*cep->display)
	{
		argv[7] = "-t";
		argv[8] = cep->display;
		argv[9] = NULL;
	}

	/* exec program */
	
	DBGL(DL_PROC, (logit(LL_DBG, "exec_answer: prog=[%s]", cep->answerprog)));
	
	pid = exec_prog(cep->answerprog, argv);
		
	/* enter pid and conf ptr entry addr into table */
	
	if (pid != -1)
	{
		int i;
		
		for (i=0; i < MAX_PIDS; i++)
		{
			if (pid_tab[i].pid == 0)
			{
				pid_tab[i].pid = pid;
				pid_tab[i].cep = cep;
				break;
			}
		}
		return(GOOD);
	}
	return(ERROR);
}

/*---------------------------------------------------------------------------*
 *	check if a connection has an outstanding process, if yes, kill it
 *---------------------------------------------------------------------------*/
void
check_and_kill(struct cfg_entry *cep)
{
	int i;
	
	for (i=0; i < MAX_PIDS; i++)
	{
		if (pid_tab[i].cep == cep)
		{
			pid_t kp;

			DBGL(DL_PROC, (logit(LL_DBG, "check_and_kill: killing pid %d", pid_tab[i].pid)));

			kp = pid_tab[i].pid;
			pid_tab[i].pid = 0;			
			kill(kp, SIGHUP);
			break;
		}
	}
}

/*---------------------------------------------------------------------------*
 *	update budget callout/callback statistics counter file
 *---------------------------------------------------------------------------*/
void
upd_callstat_file(char *filename, int rotateflag)
{
	FILE *fp;
	time_t s, l, now;
	long s_in, l_in;
	int n;
	int ret;

	now = time(NULL);
	
	fp = fopen(filename, "r+");

	if (fp == NULL)
	{
		/* file not there, create it and exit */
		
		logit(LL_WRN, "upd_callstat_file: creating %s", filename);

		fp = fopen(filename, "w");
		if (fp == NULL)
		{
			logit(LL_ERR, "ERROR, upd_callstat_file: cannot create %s, %s", filename, strerror(errno));
			return;
		}

		ret = fprintf(fp, "%jd %jd 1", (intmax_t)now, (intmax_t)now);
		if (ret <= 0)
			logit(LL_ERR, "ERROR, upd_callstat_file: fprintf failed: %s", strerror(errno));
		
		fclose(fp);
		return;
	}

	/* get contents */
	
	ret = fscanf(fp, "%ld %ld %d", &s_in, &l_in, &n);
	s = s_in; l = l_in;

	/* reset fp */
	
	rewind(fp);
		
	if (ret != 3)
	{
		/* file corrupt ? anyway, initialize */
		
		logit(LL_WRN, "upd_callstat_file: initializing %s", filename);

		s = l = now;
		n = 0;
	}

	if (rotateflag)
	{
		struct tm *stmp;
		int dom;

		/* get day of month for last timestamp */
		stmp = localtime(&l);
		dom = stmp->tm_mday;	

		/* get day of month for just now */
		stmp = localtime(&now);
		
		if (dom != stmp->tm_mday)
		{
			FILE *nfp;
			char buf[MAXPATHLEN];

			/* new day, write last days stats */

			snprintf(buf, sizeof(buf), "%s-%02d", filename,
			    stmp->tm_mday);

			nfp = fopen(buf, "w");
			if (nfp == NULL)
			{
				logit(LL_ERR, "ERROR, upd_callstat_file: cannot open for write %s, %s", buf, strerror(errno));
				return;
			}

			ret = fprintf(nfp, "%jd %jd %d", (intmax_t)s, (intmax_t)l, n);
			if (ret <= 0)
				logit(LL_ERR, "ERROR, upd_callstat_file: fprintf failed: %s", strerror(errno));
			
			fclose(nfp);

			/* init new days stats */
			n = 0;
			s = now;

			logit(LL_WRN, "upd_callstat_file: rotate %s, new s=%jd l=%jd n=%d", filename, (intmax_t)s, (intmax_t)l, n);
		}				
	}

	n++;	/* increment call count */

	/*
	 * the "%-3d" is necessary to overwrite any
	 * leftovers from previous contents!
	 */

	ret = fprintf(fp, "%jd %jd %-3d", (time_t)s, (time_t)now, n);	

	if (ret <= 0)
		logit(LL_ERR, "ERROR, upd_callstat_file: fprintf failed: %s", strerror(errno));
	
	fclose(fp);
}
	
/* EOF */
