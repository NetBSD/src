/*	$NetBSD: extern.h,v 1.1.4.1 1999/12/27 18:28:43 wrstuden Exp $	 */

/*
 * Copyright (c) 1997 Christos Zoulas. All rights reserved.
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
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* ch.c */
int ch_seek __P((off_t));
int ch_end_seek __P((void));
int ch_beg_seek __P((void));
off_t ch_length __P((void));
off_t ch_tell __P((void));
int ch_forw_get __P((void));
int ch_back_get __P((void));
void ch_init __P((int, int));
int ch_addbuf __P((int));

/* command.c */
void start_mca __P((int, char *));
int prompt __P((void));
void commands __P((void));
void editfile __P((void));
void showlist __P((void));

/* decode.c */
void noprefix __P((void));
int cmd_decode __P((int));
int cmd_search __P((char *, char *));

/* help.c */
void help __P((void));

/* input.c */
off_t forw_line __P((off_t));
off_t back_line __P((off_t));

/* line.c */
void prewind __P((void));
int pappend __P((int));
off_t forw_raw_line __P((off_t));
off_t back_raw_line __P((off_t));

/* linenum.c */
void clr_linenum __P((void));
void add_lnum __P((int, off_t));
int find_linenum __P((off_t));
int currline __P((int));

/* main.c */
int edit __P((char *));
void next_file __P((int));
void prev_file __P((int));
int main __P((int, char **));
char *save __P((char *));
void quit __P((void)) __attribute__((noreturn));

/* option.c */
int option __P((int, char **));

/* os.c */
void lsystem __P((char *));
int iread __P((int, char *, int));
void intread __P((void));
char *glob __P((char *));
char *bad_file __P((char *, char *, u_int));
void strtcpy __P((char *, char *, int));

/* output.c */
void put_line __P((void));
void flush __P((void));
void purge __P((void));
int  putchr __P((int));
void putstr __P((char *));
void error __P((char *));
void ierror __P((char *));

/* position.c */
off_t position __P((int));
void add_forw_pos __P((off_t));
void add_back_pos __P((off_t));
void copytable __P((void));
void pos_clear __P((void));
int onscreen __P((off_t));

/* prim.c */
void eof_check __P((void));
void squish_check __P((void));
void forw __P((int, off_t, int));
void back __P((int, off_t, int));
void forward __P((int, int));
void backward __P((int, int));
void prepaint __P((off_t));
void repaint __P((void));
void jump_forw __P((void));
void jump_back __P((int));
void jump_percent __P((int));
void jump_loc __P((off_t));
void init_mark __P((void));
void setmark __P((int));
void lastmark __P((void));
void gomark __P((int));
int get_back_scroll __P((void));
int search __P((int, char *, int, int));

/* screen.c */
void raw_mode __P((int));
void get_term __P((void));
void init __P((void));
void deinit __P((void));
void home __P((void));
void add_line __P((void));
void lower_left __P((void));
void bell __P((void));
void clear __P((void));
void clear_eol __P((void));
void so_enter __P((void));
void so_exit __P((void));
void ul_enter __P((void));
void ul_exit __P((void));
void bo_enter __P((void));
void bo_exit __P((void));
void backspace __P((void));
void putbs __P((void));

/* signal.c */
void winch __P((int));
void init_signals __P((int));
void psignals __P((void));

/* ttyin.c */
void open_getchr __P((void));
int getchr __P((void));

extern char **av;
extern char *current_file;
extern char *current_name;
extern char *firstsearch;
extern char *line;
extern char *next_name;
extern int ac;
extern int any_display;
extern int auto_wrap;
extern int back_scroll;
extern int be_width;
extern int bo_width;
extern int bs_mode;
extern int caseless;
extern int cbufs;
extern int cmdstack;
extern int curr_ac;
extern int erase_char;
extern int errmsgs;
extern int file;
extern int hit_eof;
extern int ignaw;
extern int ispipe;
extern int kill_char;
extern int linenums;
extern int lnloop;
extern int quit_at_eof;
extern int quitting;
extern int reading;
extern int retain_below;
extern int sc_height;
extern int sc_width;
extern int sc_window;
extern int screen_trashed;
extern int scroll;
extern int se_width;
extern int short_file;
extern int sigs;
extern int so_width;
extern int squeeze;
extern int tabstop;
extern int top_scroll;
extern int ue_width;
extern int ul_width;
extern int werase_char;
