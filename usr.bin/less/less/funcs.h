/*	$NetBSD: funcs.h,v 1.3.2.1 1999/12/27 18:37:02 wrstuden Exp $	*/


/* brac.c */
public void match_brac __P((int, int, int, int));

/* ch.c */
int fch_get __P((void));
public void ch_ungetchar __P((int));
public void end_logfile __P((void));
public void sync_logfile __P((void));
public int ch_seek __P((POSITION));
public int ch_end_seek __P((void));
public int ch_beg_seek __P((void));
public POSITION ch_length __P((void));
public POSITION ch_tell __P((void));
public int ch_forw_get __P((void));
public int ch_back_get __P((void));
public int ch_nbuf __P((int));
public void ch_flush __P((void));
public int seekable __P((int));
public void ch_init __P((int, int));
public void ch_close __P((void));
public int ch_getflags __P((void));
struct filestate;
public void ch_dump __P((struct filestate *));

/* charset.c */
public void setbinfmt __P((char *));
public void init_charset __P((void));
public int binary_char __P((unsigned int));
public int control_char __P((int));
public char *prchar __P((int));

/* cmdbuf.c */
public void cmd_reset __P((void));
public void clear_cmd __P((void));
public void cmd_putstr __P((char *));
public int len_cmdbuf __P((void));
public void set_mlist __P((constant void *));
struct mlist;
public void cmd_addhist __P((struct mlist *, char *));
public void cmd_accept __P((void));
public int cmd_char __P((int));
public int cmd_int __P((void));
public char *get_cmdbuf __P((void));

/* command.c */
public int in_mca __P((void));
public void dispversion __P((void));
public int getcc __P((void));
public void ungetcc __P((int));
public void ungetsc __P((char *));
public void commands __P((void));

/* decode.c */
public void init_cmds __P((void));
public void add_fcmd_table __P((char *, int));
public void add_ecmd_table __P((char *, int));
public void add_var_table __P((char *, int));
public int cmd_search __P((char *, char *, char *, char **));
public int fcmd_decode __P((char *, char **));
public int ecmd_decode __P((char *, char **));
public char *lgetenv __P((char *));
public int lesskey __P((char *));
public void add_hometable __P((void));
public int editchar __P((int, int));

/* edit.c */
public void init_textlist __P((struct textlist *, char *));
public char *forw_textlist __P((struct textlist *, char *));
public char *back_textlist __P((struct textlist *, char *));
public int edit __P((char *));
public int edit_ifile __P((IFILE));
public int edit_list __P((char *));
public int edit_first __P((void));
public int edit_last __P((void));
public int edit_next __P((int));
public int edit_prev __P((int));
public int edit_index __P((int));
public IFILE save_curr_ifile __P((void));
public void unsave_ifile __P((IFILE));
public void reedit_ifile __P((IFILE));
public int edit_stdin __P((void));
public void cat_file __P((void));
public void use_logfile __P((char *));

/* filename.c */
public char *unquote_file __P((char *));
public char *homefile __P((char *));
public char *fexpand __P((char *));
public char *fcomplete __P((char *));
public int bin_file __P((int));
public char *lglob __P((char *));
public char *open_altfile __P((char *, int *, void **));
public void close_altfile __P((char *, char *, void *));
public int is_dir __P((char *));
public char *bad_file __P((char *));
public POSITION filesize __P((int));

/* forwback.c */
public void forw __P((int, POSITION, int, int, int));
public void back __P((int, POSITION, int, int));
public void forward __P((int, int, int));
public void backward __P((int, int, int));
public int get_back_scroll __P((void));

/* help.c */

/* ifile.c */
public void del_ifile __P((IFILE));
public IFILE next_ifile __P((IFILE));
public IFILE prev_ifile __P((IFILE));
public IFILE getoff_ifile __P((IFILE));
public int nifile __P((void));
public IFILE get_ifile __P((char *, IFILE));
public char *get_filename __P((IFILE));
public int get_index __P((IFILE));
public void store_pos __P((IFILE, struct scrpos *));
public void get_pos __P((IFILE, struct scrpos *));
public void set_open __P((IFILE));
public int opened __P((IFILE));
public void hold_ifile __P((IFILE, int));
public int held_ifile __P((IFILE));
public void *get_filestate __P((IFILE));
public void set_filestate __P((IFILE, void *));
public void if_dump __P((void));

/* input.c */
public void set_attnpos __P((POSITION));
public POSITION forw_line __P((POSITION));
public POSITION back_line __P((POSITION));

/* jump.c */
public void jump_forw __P((void));
public void jump_back __P((int));
public void repaint __P((void));
public void jump_percent __P((int));
public void jump_line_loc __P((POSITION, int));
public void jump_loc __P((POSITION, int));

/* line.c */
public void prewind __P((void));
public void plinenum __P((POSITION));
public int pappend __P((int, POSITION));
public void pdone __P((int));
public int gline __P((int, int *));
public void null_line __P((void));
public POSITION forw_raw_line __P((POSITION, char **));
public POSITION back_raw_line __P((POSITION, char **));

/* linenum.c */
public void clr_linenum __P((void));
public void add_lnum __P((int, POSITION));
public int find_linenum __P((POSITION));
public POSITION find_pos __P((int));
public int currline __P((int));

/* lsystem.c */
public void lsystem __P((char *, char *));
public int pipe_mark __P((int, char *));
public int pipe_data __P((char *, POSITION, POSITION));

/* main.c */
int main __P((int, char *[]));
public char *save __P((char *));
public VOID_POINTER ecalloc __P((int, unsigned int));
public char *skipsp __P((char *));
public void quit __P((int)) __attribute__((__noreturn__));

/* mark.c */
public void init_mark __P((void));
public int badmark __P((int));
public void setmark __P((int));
public void lastmark __P((void));
public void gomark __P((int));
public POSITION markpos __P((int));

/* optfunc.c */
public void opt_o __P((int, char *));
public void opt__O __P((int, char *));
public void opt_l __P((int, char *));
public void opt_k __P((int, char *));
public void opt_t __P((int, char *));
public void opt__T __P((int, char *));
public void opt_p __P((int, char *));
public void opt__P __P((int, char *));
public void opt_b __P((int, char *));
public void opt_i __P((int, char *));
public void opt__V __P((int, char *));
public void opt_D __P((int, char *));
public void opt_quote __P((int, char *));
public void opt_query __P((int, char *));
public int get_swindow __P((void));

/* option.c */
public void scan_option __P((char *));
public void toggle_option __P((int, char *, int));
public int single_char_option __P((int));
public char *opt_prompt __P((int));
public int isoptpending __P((void));
public void nopendopt __P((void));
public int getnum __P((char **, int, int *));

/* opttbl.c */
public void init_option __P((void));
public struct option *findopt __P((int));

/* os.c */
public int iread __P((int, char *, unsigned int));
public void intread __P((void));
public long get_time __P((void));
public char *errno_message __P((char *));
public int percentage __P((POSITION, POSITION ));
public POSITION percent_pos __P((POSITION, int));
public int os9_signal __P((int, RETSIGTYPE (*)(int)));
public int isatty __P((int));

/* output.c */
public void put_line __P((void));
public void flush __P((void));
public int putchr __P((int));
public void putstr __P((char *));
public void get_return __P((void));
public void error __P((char *, PARG *));
public void ierror __P((char *, PARG *));
public int query __P((char *, PARG *));

/* position.c */
public POSITION position __P((int));
public void add_forw_pos __P((POSITION));
public void add_back_pos __P((POSITION));
public void pos_clear __P((void));
public void pos_init __P((void));
public int onscreen __P((POSITION));
public int empty_screen __P((void));
public int empty_lines __P((int, int));
public void get_scrpos __P((struct scrpos *));
public int adjsline __P((int));

/* prompt.c */
public void init_prompt __P((void));
public char *pr_expand __P((char *, int));
public char *eq_message __P((void));
public char *pr_string __P((void));

/* screen.c */
public void raw_mode __P((int));
public void scrsize __P((void));
public void get_editkeys __P((void));
public void get_term __P((void));
public void init __P((void));
public void deinit __P((void));
public void home __P((void));
public void add_line __P((void));
public void remove_top __P((int));
public void lower_left __P((void));
public void check_winch __P((void));
public void goto_line __P((int));
public void vbell __P((void));
public void bell __P((void));
public void clear __P((void));
public void clear_eol __P((void));
public void clear_bot __P((void));
public void so_enter __P((void));
public void so_exit __P((void));
public void ul_enter __P((void));
public void ul_exit __P((void));
public void bo_enter __P((void));
public void bo_exit __P((void));
public void bl_enter __P((void));
public void bl_exit __P((void));
public void backspace __P((void));
public void putbs __P((void));
public char WIN32getch __P((int));

/* search.c */
public void repaint_hilite __P((int));
public void undo_search __P((void));
public void clr_hilite __P((void));
public int is_hilited __P((POSITION, POSITION, int));
public void chg_caseless __P((void));
public void chg_hilite __P((void));
public int search __P((int, char *, int));
public void prep_hilite __P((POSITION, POSITION, int));
public void clear_attn __P((void));

/* signal.c */
public RETSIGTYPE winch __P((int));
public RETSIGTYPE winch __P((int));
public void init_signals __P((int));
public void psignals __P((void));

/* tags.c */
public void findtag __P((char *));
public int edit_tagfile __P((void));
public POSITION tagsearch __P((void));

/* ttyin.c */
public void open_getchr __P((void));
public void close_getchr __P((void));
public int getchr __P((void));
