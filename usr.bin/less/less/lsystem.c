/*	$NetBSD: lsystem.c,v 1.1.1.2 1997/04/22 13:45:48 mrg Exp $	*/

/*
 * Copyright (c) 1984,1985,1989,1994,1995,1996  Mark Nudelman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice in the documentation and/or other materials provided with 
 *    the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN 
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * Routines to execute other programs.
 * Necessarily very OS dependent.
 */

#include <signal.h>
#include "less.h"
#include "position.h"

#if MSDOS_COMPILER
#include <dos.h>
#endif

extern int screen_trashed;
extern IFILE curr_ifile;


#if HAVE_SYSTEM

/*
 * Pass the specified command to a shell to be executed.
 * Like plain "system()", but handles resetting terminal modes, etc.
 */
	public void
lsystem(cmd, donemsg)
	char *cmd;
	char *donemsg;
{
	register int inp;
	register char *shell;
	register char *p;
	IFILE save_ifile;

	/*
	 * Print the command which is to be executed,
	 * unless the command starts with a "-".
	 */
	if (cmd[0] == '-')
		cmd++;
	else
	{
		clear_bot();
		putstr("!");
		putstr(cmd);
		putstr("\n");
	}

	/*
	 * Close the current input file.
	 */
	save_ifile = curr_ifile;
	(void) edit_ifile(NULL_IFILE);

	/*
	 * De-initialize the terminal and take out of raw mode.
	 */
	deinit();
	flush();	/* Make sure the deinit chars get out */
	raw_mode(0);

	/*
	 * Restore signals to their defaults.
	 */
	init_signals(0);

#if HAVE_DUP
	/*
	 * Force standard input to be the user's terminal
	 * (the normal standard input), even if less's standard input 
	 * is coming from a pipe.
	 */
	inp = dup(0);
	close(0);
	if (OPEN_TTYIN() < 0)
		dup(inp);
#endif

	/*
	 * Pass the command to the system to be executed.
	 * If we have a SHELL environment variable, use
	 * <$SHELL -c "command"> instead of just <command>.
	 * If the command is empty, just invoke a shell.
	 */
#if HAVE_SHELL
	p = NULL;
	if ((shell = lgetenv("SHELL")) != NULL && *shell != '\0')
	{
		if (*cmd == '\0')
			p = save(shell);
		else
		{
			p = (char *) ecalloc(strlen(shell) + strlen(cmd) + 7, 
					sizeof(char));
			sprintf(p, "%s -c \"%s\"", shell, cmd);
		}
	}
	if (p == NULL)
	{
		if (*cmd == '\0')
			p = save("sh");
		else
			p = save(cmd);
	}

	system(p);
	free(p);
#else
	system(cmd);
#endif

#if HAVE_DUP
	/*
	 * Restore standard input, reset signals, raw mode, etc.
	 */
	close(0);
	dup(inp);
	close(inp);
#endif

	init_signals(1);
	raw_mode(1);
	if (donemsg != NULL)
	{
		putstr(donemsg);
		putstr("  (press RETURN)");
		get_return();
	}
	init();
	screen_trashed = 1;

	/*
	 * Reopen the current input file.
	 */
	reedit_ifile(save_ifile);

#if defined(SIGWINCH) || defined(SIGWIND)
	/*
	 * Since we were ignoring window change signals while we executed
	 * the system command, we must assume the window changed.
	 * Warning: this leaves a signal pending (in "sigs"),
	 * so psignals() should be called soon after lsystem().
	 */
	winch(0);
#endif
}

#endif

#if PIPEC

/*
 * Pipe a section of the input file into the given shell command.
 * The section to be piped is the section "between" the current
 * position and the position marked by the given letter.
 *
 * If the mark is after the current screen, the section between
 * the top line displayed and the mark is piped.
 * If the mark is before the current screen, the section between
 * the mark and the bottom line displayed is piped.
 * If the mark is on the current screen, or if the mark is ".",
 * the whole current screen is piped.
 */
	public int
pipe_mark(c, cmd)
	int c;
	char *cmd;
{
	POSITION mpos, tpos, bpos;

	/*
	 * mpos = the marked position.
	 * tpos = top of screen.
	 * bpos = bottom of screen.
	 */
	mpos = markpos(c);
	if (mpos == NULL_POSITION)
		return (-1);
	tpos = position(TOP);
	if (tpos == NULL_POSITION)
		tpos = ch_zero();
	bpos = position(BOTTOM);

 	if (c == '.') 
 		return (pipe_data(cmd, tpos, bpos));
 	else if (mpos <= tpos)
 		return (pipe_data(cmd, mpos, bpos));
 	else if (bpos == NULL_POSITION)
 		return (pipe_data(cmd, tpos, bpos));
 	else
 		return (pipe_data(cmd, tpos, mpos));
}

/*
 * Create a pipe to the given shell command.
 * Feed it the file contents between the positions spos and epos.
 */
	public int
pipe_data(cmd, spos, epos)
	char *cmd;
	POSITION spos;
	POSITION epos;
{
	register FILE *f;
	register int c;
	extern FILE *popen();

	/*
	 * This is structured much like lsystem().
	 * Since we're running a shell program, we must be careful
	 * to perform the necessary deinitialization before running
	 * the command, and reinitialization after it.
	 */
	if (ch_seek(spos) != 0)
	{
		error("Cannot seek to start position", NULL_PARG);
		return (-1);
	}

	if ((f = popen(cmd, "w")) == NULL)
	{
		error("Cannot create pipe", NULL_PARG);
		return (-1);
	}
	clear_bot();
	putstr("!");
	putstr(cmd);
	putstr("\n");

	deinit();
	flush();
	raw_mode(0);
	init_signals(0);
#ifdef SIGPIPE
	LSIGNAL(SIGPIPE, SIG_IGN);
#endif

	c = EOI;
	while (epos == NULL_POSITION || spos++ <= epos)
	{
		/*
		 * Read a character from the file and give it to the pipe.
		 */
		c = ch_forw_get();
		if (c == EOI)
			break;
		if (putc(c, f) == EOF)
			break;
	}

	/*
	 * Finish up the last line.
	 */
 	while (c != '\n' && c != EOI ) 
 	{
 		c = ch_forw_get();
 		if (c == EOI)
 			break;
 		if (putc(c, f) == EOF)
 			break;
 	}

	pclose(f);

#ifdef SIGPIPE
	LSIGNAL(SIGPIPE, SIG_DFL);
#endif
	init_signals(1);
	raw_mode(1);
	init();
	screen_trashed = 1;
#if defined(SIGWINCH) || defined(SIGWIND)
	/* {{ Probably don't need this here. }} */
	winch(0);
#endif
	return (0);
}

#endif

#ifdef _OSK
/*
 * Popen, and Pclose, for OS-9.
 */

#define ERR      (-1)
#define PIPEMAX  _NFILE
#define READ     1 /* For OS-9 */
#define WRITE    2 /* For OS-9 */
#define STDIN    0 /* For OS-9 */
#define STDOUT   1 /* For OS-9 */

#define RESTORE  free(parameter); close(path); dup(save); close(save);

static int   _pid[PIPEMAX];

FILE *popen(command, type)
	char *command, *type;
{
	register char *p = command;
	char *parameter;
	FILE *_pfp;
	int l, path, pipe, pcnt, save;

	path = (*type == 'w') ? STDIN : STDOUT;

	if ((pipe = open("/pipe", READ+WRITE)) == ERR)
		return (NULL);
	pcnt = pipe;

	if ((save = dup(path)) == ERR) 
	{
		close(pipe);
		return (NULL);
	}
	close(path);

	if (dup(pipe) == ERR) 
	{
		dup(save);
		close(save);
		close(pipe);
		return (NULL);
	}

	while (*p != ' ' && *p)
		p++;
	if (*p == ' ')
		p++;
	l = strlen(p);
	parameter = (char *) malloc(l+2);
	strcpy(parameter,p);
	strcat(parameter,"\n");

	if ((_pid[pcnt] = os9fork(command,l+1,parameter,1,1,0)) == ERR) 
	{
		{ RESTORE }
		close(pipe);
		_pid[pcnt] = 0;
		return (NULL);
	}

	{ RESTORE }

	if ((_pfp = fdopen(pipe,type)) == NULL)
	{
		close(pipe);
		while (((l=wait(0)) != _pid[pcnt]) && l != ERR)
			;
		_pid[pcnt] = 0;
		return (NULL);
	}

	return (_pfp);
}

int pclose(stream)
	FILE *stream;
{
	register int i;
	int f, status;

	f = fileno(stream);
	fclose(stream);
	while ((i = wait(&status)) != _pid[f] && i != ERR)
		;
	_pid[f] = 0;
	return ((i == ERR) ? ERR : status);
}
#endif /* _OSK */
