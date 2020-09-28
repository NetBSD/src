/*	$NetBSD: job.h,v 1.54 2020/09/28 00:13:03 rillig Exp $	*/

/*
 * Copyright (c) 1988, 1989, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam de Boor.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	from: @(#)job.h	8.1 (Berkeley) 6/6/93
 */

/*
 * Copyright (c) 1988, 1989 by Adam de Boor
 * Copyright (c) 1989 by Berkeley Softworks
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam de Boor.
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
 *
 *	from: @(#)job.h	8.1 (Berkeley) 6/6/93
 */

/*
 * Running of jobs in parallel mode.
 */

#ifndef MAKE_JOB_H
#define MAKE_JOB_H

#define TMPPAT	"makeXXXXXX"		/* relative to tmpdir */

#ifdef USE_SELECT
/*
 * Emulate poll() in terms of select().  This is not a complete
 * emulation but it is sufficient for make's purposes.
 */

#define poll emul_poll
#define pollfd emul_pollfd

struct emul_pollfd {
    int fd;
    short events;
    short revents;
};

#define	POLLIN		0x0001
#define	POLLOUT		0x0004

int
emul_poll(struct pollfd *fd, int nfd, int timeout);
#endif

/*
 * The POLL_MSEC constant determines the maximum number of milliseconds spent
 * in poll before coming out to see if a child has finished.
 */
#define POLL_MSEC	5000

struct pollfd;


#ifdef USE_META
# include "meta.h"
#endif

/* A Job manages the shell commands that are run to create a single target.
 * Each job is run in a separate subprocess by a shell.  Several jobs can run
 * in parallel.
 *
 * The shell commands for the target are written to a temporary file,
 * then the shell is run with the temporary file as stdin, and the output
 * of that shell is captured via a pipe.
 *
 * When a job is finished, Make_Update updates all parents of the node
 * that was just remade, marking them as ready to be made next if all
 * other dependencies are finished as well. */
typedef struct Job {
    /* The process ID of the shell running the commands */
    int pid;

    /* The target the child is making */
    GNode *node;

    /* If one of the shell commands is "...", all following commands are
     * delayed until the .END node is made.  This list node points to the
     * first of these commands, if any. */
    StringListNode *tailCmds;

    /* When creating the shell script, this is where the commands go.
     * This is only used before the job is actually started. */
    FILE *cmdFILE;

    int exit_status;		/* from wait4() in signal handler */

    char job_state;		/* status of the job entry */
#define JOB_ST_FREE	0	/* Job is available */
#define JOB_ST_SETUP	1	/* Job is allocated but otherwise invalid */
#define JOB_ST_RUNNING	3	/* Job is running, pid valid */
#define JOB_ST_FINISHED	4	/* Job is done (ie after SIGCHILD) */

    char job_suspended;

    short flags;		/* Flags to control treatment of job */
#define	JOB_IGNERR	0x001	/* Ignore non-zero exits */
#define	JOB_SILENT	0x002	/* no output */
#define JOB_SPECIAL	0x004	/* Target is a special one. i.e. run it locally
				 * if we can't export it and maxLocal is 0 */
#define JOB_IGNDOTS	0x008	/* Ignore "..." lines when processing
				 * commands */
#define JOB_TRACED	0x400	/* we've sent 'set -x' */

    int inPipe;			/* Pipe for reading output from job */
    int outPipe;		/* Pipe for writing control commands */
    struct pollfd *inPollfd;	/* pollfd associated with inPipe */

#define JOB_BUFSIZE	1024
    /* Buffer for storing the output of the job, line by line. */
    char outBuf[JOB_BUFSIZE + 1];
    int curPos;			/* Current position in outBuf. */

#ifdef USE_META
    struct BuildMon bm;
#endif
} Job;

/*-
 * Shell Specifications:
 * Each shell type has associated with it the following information:
 *	1) The string which must match the last character of the shell name
 *	   for the shell to be considered of this type. The longest match
 *	   wins.
 *	2) A command to issue to turn off echoing of command lines
 *	3) A command to issue to turn echoing back on again
 *	4) What the shell prints, and its length, when given the echo-off
 *	   command. This line will not be printed when received from the shell
 *	5) A boolean to tell if the shell has the ability to control
 *	   error checking for individual commands.
 *	6) The string to turn this checking on.
 *	7) The string to turn it off.
 *	8) The command-flag to give to cause the shell to start echoing
 *	   commands right away.
 *	9) The command-flag to cause the shell to Lib_Exit when an error is
 *	   detected in one of the commands.
 *
 * Some special stuff goes on if a shell doesn't have error control. In such
 * a case, errCheck becomes a printf template for echoing the command,
 * should echoing be on and ignErr becomes another printf template for
 * executing the command while ignoring the return status. Finally errOut
 * is a printf template for running the command and causing the shell to
 * exit on error. If any of these strings are empty when hasErrCtl is FALSE,
 * the command will be executed anyway as is and if it causes an error, so be
 * it. Any templates setup to echo the command will escape any '$ ` \ "'i
 * characters in the command string to avoid common problems with
 * echo "%s\n" as a template.
 */
typedef struct Shell {
    const char *name;		/* the name of the shell. For Bourne and C
				 * shells, this is used only to find the
				 * shell description when used as the single
				 * source of a .SHELL target. For user-defined
				 * shells, this is the full path of the shell.
				 */
    Boolean hasEchoCtl;		/* True if both echoOff and echoOn defined */
    const char *echoOff;	/* command to turn off echo */
    const char *echoOn;		/* command to turn it back on again */
    const char *noPrint;	/* command to skip when printing output from
				 * shell. This is usually the command which
				 * was executed to turn off echoing */
    size_t noPLen;		/* length of noPrint command */
    Boolean hasErrCtl;		/* set if can control error checking for
				 * individual commands */
    const char *errCheck;	/* string to turn error checking on */
    const char *ignErr;		/* string to turn off error checking */
    const char *errOut;		/* string to use for testing exit code */
    const char *newline;	/* string literal that results in a newline
				 * character when it appears outside of any
				 * 'quote' or "quote" characters */
    char commentChar;		/* character used by shell for comment lines */

    /*
     * command-line flags
     */
    const char *echo;		/* echo commands */
    const char *exit;		/* exit on error */
} Shell;

extern const char *shellPath;
extern const char *shellName;
extern char *shellErrFlag;

extern int jobTokensRunning;	/* tokens currently "out" */
extern int maxJobs;		/* Max jobs we can run */

void Shell_Init(void);
const char *Shell_GetNewline(void);
void Job_Touch(GNode *, Boolean);
Boolean Job_CheckCommands(GNode *, void (*abortProc)(const char *, ...));
void Job_CatchChildren(void);
void Job_CatchOutput(void);
void Job_Make(GNode *);
void Job_Init(void);
Boolean Job_ParseShell(char *);
int Job_Finish(void);
void Job_End(void);
void Job_Wait(void);
void Job_AbortAll(void);
void Job_TokenReturn(void);
Boolean Job_TokenWithdraw(void);
void Job_ServerStart(int, int, int);
void Job_SetPrefix(void);
Boolean Job_RunTarget(const char *, const char *);

#endif /* MAKE_JOB_H */
