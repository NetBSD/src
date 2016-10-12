/* TUI support I/O functions.

   Copyright (C) 1998-2016 Free Software Foundation, Inc.

   Contributed by Hewlett-Packard Company.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "target.h"
#include "event-loop.h"
#include "event-top.h"
#include "command.h"
#include "top.h"
#include "tui/tui.h"
#include "tui/tui-data.h"
#include "tui/tui-io.h"
#include "tui/tui-command.h"
#include "tui/tui-win.h"
#include "tui/tui-wingeneral.h"
#include "tui/tui-file.h"
#include "ui-out.h"
#include "cli-out.h"
#include <fcntl.h>
#include <signal.h>
#include "filestuff.h"
#include "completer.h"
#include "gdb_curses.h"

/* This redefines CTRL if it is not already defined, so it must come
   after terminal state releated include files like <term.h> and
   "gdb_curses.h".  */
#include "readline/readline.h"

int
key_is_start_sequence (int ch)
{
  return (ch == 27);
}

int
key_is_end_sequence (int ch)
{
  return (ch == 126);
}

int
key_is_backspace (int ch)
{
  return (ch == 8);
}

/* Use definition from readline 4.3.  */
#undef CTRL_CHAR
#define CTRL_CHAR(c) \
     ((c) < control_character_threshold && (((c) & 0x80) == 0))

/* This file controls the IO interactions between gdb and curses.
   When the TUI is enabled, gdb has two modes a curses and a standard
   mode.

   In curses mode, the gdb outputs are made in a curses command
   window.  For this, the gdb_stdout and gdb_stderr are redirected to
   the specific ui_file implemented by TUI.  The output is handled by
   tui_puts().  The input is also controlled by curses with
   tui_getc().  The readline library uses this function to get its
   input.  Several readline hooks are installed to redirect readline
   output to the TUI (see also the note below).

   In normal mode, the gdb outputs are restored to their origin, that
   is as if TUI is not used.  Readline also uses its original getc()
   function with stdin.

   Note SCz/2001-07-21: the current readline is not clean in its
   management of the output.  Even if we install a redisplay handler,
   it sometimes writes on a stdout file.  It is important to redirect
   every output produced by readline, otherwise the curses window will
   be garbled.  This is implemented with a pipe that TUI reads and
   readline writes to.  A gdb input handler is created so that reading
   the pipe is handled automatically.  This will probably not work on
   non-Unix platforms.  The best fix is to make readline clean enougth
   so that is never write on stdout.

   Note SCz/2002-09-01: we now use more readline hooks and it seems
   that with them we don't need the pipe anymore (verified by creating
   the pipe and closing its end so that write causes a SIGPIPE).  The
   old pipe code is still there and can be conditionally removed by
   #undef TUI_USE_PIPE_FOR_READLINE.  */

/* For gdb 5.3, prefer to continue the pipe hack as a backup wheel.  */
#ifdef HAVE_PIPE
#define TUI_USE_PIPE_FOR_READLINE
#endif
/* #undef TUI_USE_PIPE_FOR_READLINE */

/* TUI output files.  */
static struct ui_file *tui_stdout;
static struct ui_file *tui_stderr;
struct ui_out *tui_out;

/* GDB output files in non-curses mode.  */
static struct ui_file *tui_old_stdout;
static struct ui_file *tui_old_stderr;
struct ui_out *tui_old_uiout;

/* Readline previous hooks.  */
static rl_getc_func_t *tui_old_rl_getc_function;
static rl_voidfunc_t *tui_old_rl_redisplay_function;
static rl_vintfunc_t *tui_old_rl_prep_terminal;
static rl_voidfunc_t *tui_old_rl_deprep_terminal;
static rl_compdisp_func_t *tui_old_rl_display_matches_hook;
static int tui_old_rl_echoing_p;

/* Readline output stream.
   Should be removed when readline is clean.  */
static FILE *tui_rl_outstream;
static FILE *tui_old_rl_outstream;
#ifdef TUI_USE_PIPE_FOR_READLINE
static int tui_readline_pipe[2];
#endif

/* The last gdb prompt that was registered in readline.
   This may be the main gdb prompt or a secondary prompt.  */
static char *tui_rl_saved_prompt;

static void
tui_putc (char c)
{
  char buf[2];

  buf[0] = c;
  buf[1] = 0;
  tui_puts (buf);
}

/* Print the string in the curses command window.
   The output is buffered.  It is up to the caller to refresh the screen
   if necessary.  */

void
tui_puts (const char *string)
{
  static int tui_skip_line = -1;
  char c;
  WINDOW *w;

  w = TUI_CMD_WIN->generic.handle;
  while ((c = *string++) != 0)
    {
      /* Catch annotation and discard them.  We need two \032 and
         discard until a \n is seen.  */
      if (c == '\032')
        {
          tui_skip_line++;
        }
      else if (tui_skip_line != 1)
        {
          tui_skip_line = -1;
	  /* Expand TABs, since ncurses on MS-Windows doesn't.  */
	  if (c == '\t')
	    {
	      int col;

	      col = getcurx (w);
	      do
		{
		  waddch (w, ' ');
		  col++;
		} while ((col % 8) != 0);
	    }
	  else
	    waddch (w, c);
        }
      else if (c == '\n')
        tui_skip_line = -1;
    }
  TUI_CMD_WIN->detail.command_info.start_line = getcury (w);
}

/* Readline callback.
   Redisplay the command line with its prompt after readline has
   changed the edited text.  */
void
tui_redisplay_readline (void)
{
  int prev_col;
  int height;
  int col;
  int c_pos;
  int c_line;
  int in;
  WINDOW *w;
  const char *prompt;
  int start_line;

  /* Detect when we temporarily left SingleKey and now the readline
     edit buffer is empty, automatically restore the SingleKey
     mode.  The restore must only be done if the command has finished.
     The command could call prompt_for_continue and we must not
     restore SingleKey so that the prompt and normal keymap are used.  */
  if (tui_current_key_mode == TUI_ONE_COMMAND_MODE && rl_end == 0
      && !gdb_in_secondary_prompt_p (current_ui))
    tui_set_key_mode (TUI_SINGLE_KEY_MODE);

  if (tui_current_key_mode == TUI_SINGLE_KEY_MODE)
    prompt = "";
  else
    prompt = tui_rl_saved_prompt;
  
  c_pos = -1;
  c_line = -1;
  w = TUI_CMD_WIN->generic.handle;
  start_line = TUI_CMD_WIN->detail.command_info.start_line;
  wmove (w, start_line, 0);
  prev_col = 0;
  height = 1;
  for (in = 0; prompt && prompt[in]; in++)
    {
      waddch (w, prompt[in]);
      col = getcurx (w);
      if (col <= prev_col)
        height++;
      prev_col = col;
    }
  for (in = 0; in <= rl_end; in++)
    {
      unsigned char c;
      
      if (in == rl_point)
	{
          getyx (w, c_line, c_pos);
	}

      if (in == rl_end)
        break;

      c = (unsigned char) rl_line_buffer[in];
      if (CTRL_CHAR (c) || c == RUBOUT)
	{
          waddch (w, '^');
          waddch (w, CTRL_CHAR (c) ? UNCTRL (c) : '?');
	}
      else if (c == '\t')
	{
	  /* Expand TABs, since ncurses on MS-Windows doesn't.  */
	  col = getcurx (w);
	  do
	    {
	      waddch (w, ' ');
	      col++;
	    } while ((col % 8) != 0);
	}
      else
	{
          waddch (w, c);
	}
      if (c == '\n')
	TUI_CMD_WIN->detail.command_info.start_line = getcury (w);
      col = getcurx (w);
      if (col < prev_col)
        height++;
      prev_col = col;
    }
  wclrtobot (w);
  TUI_CMD_WIN->detail.command_info.start_line = getcury (w);
  if (c_line >= 0)
    wmove (w, c_line, c_pos);
  TUI_CMD_WIN->detail.command_info.start_line -= height - 1;

  wrefresh (w);
  fflush(stdout);
}

/* Readline callback to prepare the terminal.  It is called once each
   time we enter readline.  Terminal is already setup in curses
   mode.  */
static void
tui_prep_terminal (int notused1)
{
  /* Save the prompt registered in readline to correctly display it.
     (we can't use gdb_prompt() due to secondary prompts and can't use
     rl_prompt because it points to an alloca buffer).  */
  xfree (tui_rl_saved_prompt);
  tui_rl_saved_prompt = rl_prompt != NULL ? xstrdup (rl_prompt) : NULL;
}

/* Readline callback to restore the terminal.  It is called once each
   time we leave readline.  There is nothing to do in curses mode.  */
static void
tui_deprep_terminal (void)
{
}

#ifdef TUI_USE_PIPE_FOR_READLINE
/* Read readline output pipe and feed the command window with it.
   Should be removed when readline is clean.  */
static void
tui_readline_output (int error, gdb_client_data data)
{
  int size;
  char buf[256];

  size = read (tui_readline_pipe[0], buf, sizeof (buf) - 1);
  if (size > 0 && tui_active)
    {
      buf[size] = 0;
      tui_puts (buf);
    }
}
#endif

/* TUI version of displayer.crlf.  */

static void
tui_mld_crlf (const struct match_list_displayer *displayer)
{
  tui_putc ('\n');
}

/* TUI version of displayer.putch.  */

static void
tui_mld_putch (const struct match_list_displayer *displayer, int ch)
{
  tui_putc (ch);
}

/* TUI version of displayer.puts.  */

static void
tui_mld_puts (const struct match_list_displayer *displayer, const char *s)
{
  tui_puts (s);
}

/* TUI version of displayer.flush.  */

static void
tui_mld_flush (const struct match_list_displayer *displayer)
{
  wrefresh (TUI_CMD_WIN->generic.handle);
}

/* TUI version of displayer.erase_entire_line.  */

static void
tui_mld_erase_entire_line (const struct match_list_displayer *displayer)
{
  WINDOW *w = TUI_CMD_WIN->generic.handle;
  int cur_y = getcury (w);

  wmove (w, cur_y, 0);
  wclrtoeol (w);
  wmove (w, cur_y, 0);
}

/* TUI version of displayer.beep.  */

static void
tui_mld_beep (const struct match_list_displayer *displayer)
{
  beep ();
}

/* Helper function for tui_mld_read_key.
   This temporarily replaces tui_getc for use during tab-completion
   match list display.  */

static int
tui_mld_getc (FILE *fp)
{
  WINDOW *w = TUI_CMD_WIN->generic.handle;
  int c = wgetch (w);

  return c;
}

/* TUI version of displayer.read_key.  */

static int
tui_mld_read_key (const struct match_list_displayer *displayer)
{
  rl_getc_func_t *prev = rl_getc_function;
  int c;

  /* We can't use tui_getc as we need NEWLINE to not get emitted.  */
  rl_getc_function = tui_mld_getc;
  c = rl_read_key ();
  rl_getc_function = prev;
  return c;
}

/* TUI version of rl_completion_display_matches_hook.
   See gdb_display_match_list for a description of the arguments.  */

static void
tui_rl_display_match_list (char **matches, int len, int max)
{
  struct match_list_displayer displayer;

  rl_get_screen_size (&displayer.height, &displayer.width);
  displayer.crlf = tui_mld_crlf;
  displayer.putch = tui_mld_putch;
  displayer.puts = tui_mld_puts;
  displayer.flush = tui_mld_flush;
  displayer.erase_entire_line = tui_mld_erase_entire_line;
  displayer.beep = tui_mld_beep;
  displayer.read_key = tui_mld_read_key;

  gdb_display_match_list (matches, len, max, &displayer);
}

/* Setup the IO for curses or non-curses mode.
   - In non-curses mode, readline and gdb use the standard input and
   standard output/error directly.
   - In curses mode, the standard output/error is controlled by TUI
   with the tui_stdout and tui_stderr.  The output is redirected in
   the curses command window.  Several readline callbacks are installed
   so that readline asks for its input to the curses command window
   with wgetch().  */
void
tui_setup_io (int mode)
{
  extern int _rl_echoing_p;

  if (mode)
    {
      /* Redirect readline to TUI.  */
      tui_old_rl_redisplay_function = rl_redisplay_function;
      tui_old_rl_deprep_terminal = rl_deprep_term_function;
      tui_old_rl_prep_terminal = rl_prep_term_function;
      tui_old_rl_getc_function = rl_getc_function;
      tui_old_rl_display_matches_hook = rl_completion_display_matches_hook;
      tui_old_rl_outstream = rl_outstream;
      tui_old_rl_echoing_p = _rl_echoing_p;
      rl_redisplay_function = tui_redisplay_readline;
      rl_deprep_term_function = tui_deprep_terminal;
      rl_prep_term_function = tui_prep_terminal;
      rl_getc_function = tui_getc;
      _rl_echoing_p = 0;
      rl_outstream = tui_rl_outstream;
      rl_prompt = 0;
      rl_completion_display_matches_hook = tui_rl_display_match_list;
      rl_already_prompted = 0;

      /* Keep track of previous gdb output.  */
      tui_old_stdout = gdb_stdout;
      tui_old_stderr = gdb_stderr;
      tui_old_uiout = current_uiout;

      /* Reconfigure gdb output.  */
      gdb_stdout = tui_stdout;
      gdb_stderr = tui_stderr;
      gdb_stdlog = gdb_stdout;	/* for moment */
      gdb_stdtarg = gdb_stderr;	/* for moment */
      gdb_stdtargerr = gdb_stderr;	/* for moment */
      current_uiout = tui_out;

      /* Save tty for SIGCONT.  */
      savetty ();
    }
  else
    {
      /* Restore gdb output.  */
      gdb_stdout = tui_old_stdout;
      gdb_stderr = tui_old_stderr;
      gdb_stdlog = gdb_stdout;	/* for moment */
      gdb_stdtarg = gdb_stderr;	/* for moment */
      gdb_stdtargerr = gdb_stderr;	/* for moment */
      current_uiout = tui_old_uiout;

      /* Restore readline.  */
      rl_redisplay_function = tui_old_rl_redisplay_function;
      rl_deprep_term_function = tui_old_rl_deprep_terminal;
      rl_prep_term_function = tui_old_rl_prep_terminal;
      rl_getc_function = tui_old_rl_getc_function;
      rl_completion_display_matches_hook = tui_old_rl_display_matches_hook;
      rl_outstream = tui_old_rl_outstream;
      _rl_echoing_p = tui_old_rl_echoing_p;
      rl_already_prompted = 0;

      /* Save tty for SIGCONT.  */
      savetty ();
    }
}

#ifdef SIGCONT
/* Catch SIGCONT to restore the terminal and refresh the screen.  */
static void
tui_cont_sig (int sig)
{
  if (tui_active)
    {
      /* Restore the terminal setting because another process (shell)
         might have changed it.  */
      resetty ();

      /* Force a refresh of the screen.  */
      tui_refresh_all_win ();

      wrefresh (TUI_CMD_WIN->generic.handle);
    }
  signal (sig, tui_cont_sig);
}
#endif

/* Initialize the IO for gdb in curses mode.  */
void
tui_initialize_io (void)
{
#ifdef SIGCONT
  signal (SIGCONT, tui_cont_sig);
#endif

  /* Create tui output streams.  */
  tui_stdout = tui_fileopen (stdout);
  tui_stderr = tui_fileopen (stderr);
  tui_out = tui_out_new (tui_stdout);

  /* Create the default UI.  */
  tui_old_uiout = cli_out_new (gdb_stdout);

#ifdef TUI_USE_PIPE_FOR_READLINE
  /* Temporary solution for readline writing to stdout: redirect
     readline output in a pipe, read that pipe and output the content
     in the curses command window.  */
  if (gdb_pipe_cloexec (tui_readline_pipe) != 0)
    error (_("Cannot create pipe for readline"));

  tui_rl_outstream = fdopen (tui_readline_pipe[1], "w");
  if (tui_rl_outstream == 0)
    error (_("Cannot redirect readline output"));

  setvbuf (tui_rl_outstream, (char*) NULL, _IOLBF, 0);

#ifdef O_NONBLOCK
  (void) fcntl (tui_readline_pipe[0], F_SETFL, O_NONBLOCK);
#else
#ifdef O_NDELAY
  (void) fcntl (tui_readline_pipe[0], F_SETFL, O_NDELAY);
#endif
#endif
  add_file_handler (tui_readline_pipe[0], tui_readline_output, 0);
#else
  tui_rl_outstream = stdout;
#endif
}

/* Get a character from the command window.  This is called from the
   readline package.  */
int
tui_getc (FILE *fp)
{
  int ch;
  WINDOW *w;

  w = TUI_CMD_WIN->generic.handle;

#ifdef TUI_USE_PIPE_FOR_READLINE
  /* Flush readline output.  */
  tui_readline_output (0, 0);
#endif

  ch = wgetch (w);

  /* The \n must be echoed because it will not be printed by
     readline.  */
  if (ch == '\n')
    {
      /* When hitting return with an empty input, gdb executes the last
         command.  If we emit a newline, this fills up the command window
         with empty lines with gdb prompt at beginning.  Instead of that,
         stay on the same line but provide a visual effect to show the
         user we recognized the command.  */
      if (rl_end == 0 && !gdb_in_secondary_prompt_p (current_ui))
        {
	  wmove (w, getcury (w), 0);

          /* Clear the line.  This will blink the gdb prompt since
             it will be redrawn at the same line.  */
          wclrtoeol (w);
          wrefresh (w);
          napms (20);
        }
      else
        {
	  /* Move cursor to the end of the command line before emitting the
	     newline.  We need to do so because when ncurses outputs a newline
	     it truncates any text that appears past the end of the cursor.  */
	  int px, py;
	  getyx (w, py, px);
	  px += rl_end - rl_point;
	  py += px / TUI_CMD_WIN->generic.width;
	  px %= TUI_CMD_WIN->generic.width;
	  wmove (w, py, px);
	  tui_putc ('\n');
        }
    }
  
  /* Handle prev/next/up/down here.  */
  ch = tui_dispatch_ctrl_char (ch);
  
  if (ch == KEY_BACKSPACE)
    return '\b';

  if (current_ui->command_editing && key_is_start_sequence (ch))
    {
      int ch_pending;

      nodelay (w, TRUE);
      ch_pending = wgetch (w);
      nodelay (w, FALSE);

      /* If we have pending input following a start sequence, call the stdin
	 event handler again because ncurses may have already read and stored
	 the input into its internal buffer, meaning that we won't get an stdin
	 event for it.  If we don't compensate for this missed stdin event, key
	 sequences as Alt_F (^[f) will not behave promptly.

	 (We only compensates for the missed 2nd byte of a key sequence because
	 2-byte sequences are by far the most commonly used. ncurses may have
	 buffered a larger, 3+-byte key sequence though it remains to be seen
	 whether it is useful to compensate for all the bytes of such
	 sequences.)  */
      if (ch_pending != ERR)
	{
	  ungetch (ch_pending);
	  call_stdin_event_handler_again_p = 1;
	}
    }

  return ch;
}

/* Utility function to expand TABs in a STRING into spaces.  STRING
   will be displayed starting at column COL, and is assumed to include
   no newlines.  The returned expanded string is malloc'ed.  */

char *
tui_expand_tabs (const char *string, int col)
{
  int n_adjust, ncol;
  const char *s;
  char *ret, *q;

  /* 1. How many additional characters do we need?  */
  for (ncol = col, n_adjust = 0, s = string; s; )
    {
      s = strpbrk (s, "\t");
      if (s)
	{
	  ncol += (s - string) + n_adjust;
	  /* Adjustment for the next tab stop, minus one for the TAB
	     we replace with spaces.  */
	  n_adjust += 8 - (ncol % 8) - 1;
	  s++;
	}
    }

  /* Allocate the copy.  */
  ret = q = (char *) xmalloc (strlen (string) + n_adjust + 1);

  /* 2. Copy the original string while replacing TABs with spaces.  */
  for (ncol = col, s = string; s; )
    {
      const char *s1 = strpbrk (s, "\t");
      if (s1)
	{
	  if (s1 > s)
	    {
	      strncpy (q, s, s1 - s);
	      q += s1 - s;
	      ncol += s1 - s;
	    }
	  do {
	    *q++ = ' ';
	    ncol++;
	  } while ((ncol % 8) != 0);
	  s1++;
	}
      else
	strcpy (q, s);
      s = s1;
    }

  return ret;
}
