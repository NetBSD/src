/*
 * Copyright (C) 1984-2023  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */


/*
 * Entry point, initialization, miscellaneous routines.
 */

#include "less.h"
#if MSDOS_COMPILER==WIN32C
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

public char *   every_first_cmd = NULL;
public int      new_file;
public int      is_tty;
public IFILE    curr_ifile = NULL_IFILE;
public IFILE    old_ifile = NULL_IFILE;
public struct scrpos initial_scrpos;
public POSITION start_attnpos = NULL_POSITION;
public POSITION end_attnpos = NULL_POSITION;
public int      wscroll;
public char *   progname;
public int      quitting;
public int      secure;
public int      dohelp;

#if LOGFILE
public int      logfile = -1;
public int      force_logfile = FALSE;
public char *   namelogfile = NULL;
#endif

#if EDITOR
public char *   editor;
public char *   editproto;
#endif

#if TAGS
extern char *   tags;
extern char *   tagoption;
extern int      jump_sline;
#endif

#ifdef WIN32
static char consoleTitle[256];
#endif

public int      one_screen;
extern int      less_is_more;
extern int      missing_cap;
extern int      know_dumb;
extern int      pr_type;
extern int      quit_if_one_screen;
extern int      no_init;
extern int      errmsgs;
extern int      redraw_on_quit;
extern int      term_init_done;
extern int      first_time;

/*
 * Entry point.
 */
int main(int argc, char *argv[])
{
	IFILE ifile;
	char *s;

#ifdef __EMX__
	_response(&argc, &argv);
	_wildcard(&argc, &argv);
#endif

	progname = *argv++;
	argc--;

#if SECURE
	secure = 1;
#else
	secure = 0;
	s = lgetenv("LESSSECURE");
	if (!isnullenv(s))
		secure = 1;
#endif

#ifdef WIN32
	if (getenv("HOME") == NULL)
	{
		/*
		 * If there is no HOME environment variable,
		 * try the concatenation of HOMEDRIVE + HOMEPATH.
		 */
		char *drive = getenv("HOMEDRIVE");
		char *path  = getenv("HOMEPATH");
		if (drive != NULL && path != NULL)
		{
			char *env = (char *) ecalloc(strlen(drive) + 
					strlen(path) + 6, sizeof(char));
			strcpy(env, "HOME=");
			strcat(env, drive);
			strcat(env, path);
			putenv(env);
		}
	}
	GetConsoleTitle(consoleTitle, sizeof(consoleTitle)/sizeof(char));
#endif /* WIN32 */

	/*
	 * Process command line arguments and LESS environment arguments.
	 * Command line arguments override environment arguments.
	 */
	is_tty = isatty(1);
	init_mark();
	init_cmds();
	init_poll();
	get_term();
	init_charset();
	init_line();
	init_cmdhist();
	init_option();
	init_search();

	/*
	 * If the name of the executable program is "more",
	 * act like LESS_IS_MORE is set.
	 */
	s = last_component(progname);
	if (strcmp(s, "more") == 0)
		less_is_more = 1;

	init_prompt();

	s = lgetenv(less_is_more ? "MORE" : "LESS");
	if (s != NULL)
		scan_option(s);

#define isoptstring(s)  (((s)[0] == '-' || (s)[0] == '+') && (s)[1] != '\0')
	while (argc > 0 && (isoptstring(*argv) || isoptpending()))
	{
		s = *argv++;
		argc--;
		if (strcmp(s, "--") == 0)
			break;
		scan_option(s);
	}
#undef isoptstring

	if (isoptpending())
	{
		/*
		 * Last command line option was a flag requiring a
		 * following string, but there was no following string.
		 */
		nopendopt();
		quit(QUIT_OK);
	}

	expand_cmd_tables();

#if EDITOR
	editor = lgetenv("VISUAL");
	if (editor == NULL || *editor == '\0')
	{
		editor = lgetenv("EDITOR");
		if (isnullenv(editor))
			editor = EDIT_PGM;
	}
	editproto = lgetenv("LESSEDIT");
	if (isnullenv(editproto))
		editproto = "%E ?lm+%lm. %g";
#endif

	/*
	 * Call get_ifile with all the command line filenames
	 * to "register" them with the ifile system.
	 */
	ifile = NULL_IFILE;
	if (dohelp)
		ifile = get_ifile(FAKE_HELPFILE, ifile);
	while (argc-- > 0)
	{
#if (MSDOS_COMPILER && MSDOS_COMPILER != DJGPPC)
		/*
		 * Because the "shell" doesn't expand filename patterns,
		 * treat each argument as a filename pattern rather than
		 * a single filename.  
		 * Expand the pattern and iterate over the expanded list.
		 */
		struct textlist tlist;
		char *filename;
		char *gfilename;
		char *qfilename;
		
		gfilename = lglob(*argv++);
		init_textlist(&tlist, gfilename);
		filename = NULL;
		while ((filename = forw_textlist(&tlist, filename)) != NULL)
		{
			qfilename = shell_unquote(filename);
			(void) get_ifile(qfilename, ifile);
			free(qfilename);
			ifile = prev_ifile(NULL_IFILE);
		}
		free(gfilename);
#else
		(void) get_ifile(*argv++, ifile);
		ifile = prev_ifile(NULL_IFILE);
#endif
	}
	/*
	 * Set up terminal, etc.
	 */
	if (!is_tty)
	{
		/*
		 * Output is not a tty.
		 * Just copy the input file(s) to output.
		 */
		set_output(1); /* write to stdout */
		SET_BINARY(1);
		if (edit_first() == 0)
		{
			do {
				cat_file();
			} while (edit_next(1) == 0);
		}
		quit(QUIT_OK);
	}

	if (missing_cap && !know_dumb)
		error("WARNING: terminal is not fully functional", NULL_PARG);
	open_getchr();
	raw_mode(1);
	init_signals(1);

	/*
	 * Select the first file to examine.
	 */
#if TAGS
	if (tagoption != NULL || strcmp(tags, "-") == 0)
	{
		/*
		 * A -t option was given.
		 * Verify that no filenames were also given.
		 * Edit the file selected by the "tags" search,
		 * and search for the proper line in the file.
		 */
		if (nifile() > 0)
		{
			error("No filenames allowed with -t option", NULL_PARG);
			quit(QUIT_ERROR);
		}
		findtag(tagoption);
		if (edit_tagfile())  /* Edit file which contains the tag */
			quit(QUIT_ERROR);
		/*
		 * Search for the line which contains the tag.
		 * Set up initial_scrpos so we display that line.
		 */
		initial_scrpos.pos = tagsearch();
		if (initial_scrpos.pos == NULL_POSITION)
			quit(QUIT_ERROR);
		initial_scrpos.ln = jump_sline;
	} else
#endif
	{
		if (edit_first())
			quit(QUIT_ERROR);
		/*
		 * See if file fits on one screen to decide whether 
		 * to send terminal init. But don't need this 
		 * if -X (no_init) overrides this (see init()).
		 */
		if (quit_if_one_screen)
		{
			if (nifile() > 1) /* If more than one file, -F cannot be used */
				quit_if_one_screen = FALSE;
			else if (!no_init)
				one_screen = get_one_screen();
		}
	}

	if (errmsgs > 0)
	{
		/*
		 * We displayed some messages on error output
		 * (file descriptor 2; see flush()).
		 * Before erasing the screen contents, wait for a keystroke.
		 */
		less_printf("Press RETURN to continue ", NULL_PARG);
		get_return();
		putchr('\n');
	}
	set_output(1);
	init();
	commands();
	quit(QUIT_OK);
	/*NOTREACHED*/
	return (0);
}

/*
 * Copy a string to a "safe" place
 * (that is, to a buffer allocated by calloc).
 */
public char * save(constant char *s)
{
	char *p;

	p = (char *) ecalloc(strlen(s)+1, sizeof(char));
	strcpy(p, s);
	return (p);
}

public void out_of_memory(void)
{
	error("Cannot allocate memory", NULL_PARG);
	quit(QUIT_ERROR);
}

/*
 * Allocate memory.
 * Like calloc(), but never returns an error (NULL).
 */
public void * ecalloc(int count, unsigned int size)
{
	void * p;

	p = (void *) calloc(count, size);
	if (p == NULL)
		out_of_memory();
	return p;
}

/*
 * Skip leading spaces in a string.
 */
public char * skipsp(char *s)
{
	while (*s == ' ' || *s == '\t')
		s++;
	return (s);
}

/*
 * See how many characters of two strings are identical.
 * If uppercase is true, the first string must begin with an uppercase
 * character; the remainder of the first string may be either case.
 */
public int sprefix(char *ps, char *s, int uppercase)
{
	int c;
	int sc;
	int len = 0;

	for ( ;  *s != '\0';  s++, ps++)
	{
		c = *ps;
		if (uppercase)
		{
			if (len == 0 && ASCII_IS_LOWER(c))
				return (-1);
			if (ASCII_IS_UPPER(c))
				c = ASCII_TO_LOWER(c);
		}
		sc = *s;
		if (len > 0 && ASCII_IS_UPPER(sc))
			sc = ASCII_TO_LOWER(sc);
		if (c != sc)
			break;
		len++;
	}
	return (len);
}

/*
 * Exit the program.
 */
public void quit(int status)
{
	static int save_status;

	/*
	 * Put cursor at bottom left corner, clear the line,
	 * reset the terminal modes, and exit.
	 */
	if (status < 0)
		status = save_status;
	else
		save_status = status;
	quitting = 1;
	check_altpipe_error();
	if (interactive())
		clear_bot();
	deinit();
	flush();
	if (redraw_on_quit && term_init_done)
	{
		/*
		 * The last file text displayed might have been on an 
		 * alternate screen, which now (since deinit) cannot be seen.
		 * redraw_on_quit tells us to redraw it on the main screen.
		 */
		first_time = 1; /* Don't print "skipping" or tildes */
		repaint();
		flush();
	}
	edit((char*)NULL);
	save_cmdhist();
	raw_mode(0);
#if MSDOS_COMPILER && MSDOS_COMPILER != DJGPPC
	/* 
	 * If we don't close 2, we get some garbage from
	 * 2's buffer when it flushes automatically.
	 * I cannot track this one down  RB
	 * The same bug shows up if we use ^C^C to abort.
	 */
	close(2);
#endif
#ifdef WIN32
	SetConsoleTitle(consoleTitle);
#endif
	close_getchr();
	exit(status);
}
