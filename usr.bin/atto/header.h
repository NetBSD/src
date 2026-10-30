/* header.h, Atto Emacs, Public Domain, Hugh Barney, 2016, Derived from: Anthony's Editor January 93 */
#ifndef _NETBSD_SOURCE
#define _NETBSD_SOURCE
#endif
#define _XOPEN_SOURCE 600
#include <locale.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <curses.h>
#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#define VERSION	 "Atto 1.22, Public Domain, Dec 2020, by Hugh Barney,  No warranty."
#define PROG_NAME "atto"
#define B_MODIFIED	0x01		/* modified buffer */
#define B_OVERWRITE	0x02		/* overwrite mode */
#define MSGLINE         (LINES-1)
#define NOMARK          -1
#define CHUNK           8096L
#define K_BUFFER_LENGTH 256
#define TEMPBUF         512
#define STRBUF_L        256
#define STRBUF_M        64
#define STRBUF_S        16
#define MIN_GAP_EXPAND  512
#define TEMPFILE        "/tmp/feXXXXXX"
#define F_NONE          0
#define F_CLEAR         1
#define ID_DEFAULT         1
#define ID_SYMBOL          2
#define ID_MODELINE        3
#define ID_DIGITS          4
#define ID_LINE_COMMENT    5
#define ID_BLOCK_COMMENT   6
#define ID_DOUBLE_STRING   7
#define ID_SINGLE_STRING   8

typedef unsigned char char_t;
typedef long point_t;

typedef struct buffer_t
{
	struct buffer_t *b_next;  /* Link to next buffer_t */
	point_t b_mark;	     	  /* the mark */
	point_t b_point;          /* the point */
	point_t b_cpoint;         /* the original current point, used for multiple window displaying */
	point_t b_page;           /* start of page */
	point_t b_epage;          /* end of page */
	point_t b_reframe;        /* force a reframe of the display */
	int b_cnt;                /* count of windows referencing this buffer */
	int b_size;               /* current size of text being edited (not including gap) */
	int b_psize;              /* previous size */
	char_t *b_buf;            /* start of buffer */
	char_t *b_ebuf;           /* end of buffer */
	char_t *b_gap;            /* start of gap */
	char_t *b_egap;           /* end of gap */
	int b_row;                /* cursor row */
	int b_col;                /* cursor col */
	char b_fname[NAME_MAX + 1]; /* filename */
	char b_bname[STRBUF_S];   /* buffer name */
	char b_flags;             /* buffer flags */
} buffer_t;

typedef struct window_t
{
	struct window_t *w_next;   /* Next window */
	struct buffer_t *w_bufp;   /* Buffer displayed in window */
	point_t w_point;
	point_t w_mark;
	point_t w_page;
	point_t w_epage;
	char w_top;	    /* Origin 0 top row of window */
	char w_rows;        /* no. of rows of text in window */
	int w_row;          /* cursor row */
	int w_col;          /* cursor col */
	int w_update;
	char w_name[STRBUF_S];
} window_t;

extern buffer_t *curbp;			/* current buffer */
extern buffer_t *bheadp;		/* head of list of buffers */
extern window_t *curwp;
extern window_t *wheadp;

/*
 * Some compilers define size_t as a unsigned 16 bit number while
 * point_t and off_t might be defined as a signed 32 bit number.  
 * malloc(), realloc(), fread(), and fwrite() take size_t parameters,
 * which means there will be some size limits because size_t is too
 * small of a type.
 */
#define MAX_SIZE_T      ((unsigned long) (size_t) ~0)

extern int done;                /* Quit flag. */
extern int use_color;           /* Optional syntax highlighting. */
extern int msgflag;             /* True if msgline should be displayed. */
extern int menu_mode;           /* True if menu is open. */
extern point_t nscrap;          /* Length of scrap buffer. */
extern char_t *scrap;           /* Allocated scrap buffer. */
extern char_t *input;
extern char msgline[TEMPBUF];   /* Message line input/output buffer. */
extern char temp[TEMPBUF];      /* Temporary buffer. */
extern char searchtext[STRBUF_M];
extern char replace[STRBUF_M];

extern void fatal(const char *);
extern void msg(const char *, ...) __printflike(1, 2);
extern void display(window_t *, int);
extern void dispmsg(void);
extern void modeline(window_t *);
extern int utf8_size(char_t);
extern int prev_utf8_char_size(void);
extern void display_utf8(buffer_t *, int);
extern point_t lnstart(buffer_t *, point_t);
extern point_t lncolumn(buffer_t *, point_t, int);
extern point_t segstart(buffer_t *, point_t, point_t);
extern point_t segnext(buffer_t *, point_t, point_t);
extern point_t upup(buffer_t *, point_t);
extern point_t dndn(buffer_t *, point_t);
extern char_t *get_key(void (**command_out)(void));
extern int getinput(const char *, char *, int, int);
extern int getfilename(const char *, char *, int);
extern void display_prompt_and_response(const char *, const char *);
extern int growgap(buffer_t *, point_t);
extern point_t movegap(buffer_t *, point_t);
extern point_t pos(buffer_t *, char_t *);
extern char_t *ptr(buffer_t *, point_t);
extern int posix_file(char *);
extern int save(char *);
extern int load_file(char *);
extern int insert_file(char *, int);
extern void backsp(void);
extern void block(void);
extern void iblock(void);
extern void bottom(void);
extern void cut(void);
extern void copy(void);
extern void copy_cut(int);
extern void delete(void);
extern void toggle_overwrite_mode(void);
extern void down(void);
extern void insert(void);
extern void left(void);
extern void lnbegin(void);
extern void lnend(void);
extern void paste(void);
extern void pgdown(void);
extern void pgup(void);
extern void quit(void);
extern int yesno(int);
extern void quit_ask(void);
extern void redraw(void);
extern void readfile(void);
extern void insertfile(void);
extern void right(void);
extern void top(void);
extern void up(void);
extern void version(void);
extern void wleft(void);
extern void wright(void);
extern void writefile(void);
extern void savebuffer(void);
extern void showpos(void);
extern void killtoeol(void);
extern void gotoline(void);
extern void search(void);
extern void query_replace(void);
extern point_t line_to_point(int);
extern point_t search_forward(buffer_t *, point_t, char *);
extern point_t search_backwards(buffer_t *, point_t, char *);
extern void update_search_prompt(char *, char *);
extern void display_search_result(point_t, int, const char *, const char *);
extern buffer_t* find_buffer(const char *, int);
extern void buffer_init(buffer_t *);
extern int delete_buffer(buffer_t *);
extern void next_buffer(void);
extern int count_buffers(void);
extern int modified_buffers(void);
extern void killbuffer(void);
extern char* get_buffer_name(buffer_t *);
extern void get_line_stats(int *, int *);
extern void query_replace(void);
extern window_t *new_window(void);
extern void one_window(window_t *);
extern void split_window(void);
extern void next_window(void);
extern void delete_other_windows(void);
extern void free_other_windows(window_t *);
extern void update_display(void);
extern void w2b(window_t *);
extern void b2w(window_t *);
extern void associate_b2w(buffer_t *, window_t *);
extern void disassociate_b(window_t *);
extern void set_parse_state(buffer_t *, point_t);
extern void set_parse_state2(buffer_t *, point_t);
extern int parse_text(buffer_t *, point_t);
extern void resize_terminal(void);
extern int create_menu(void);
extern void handle_menus(int);
extern void menu_escape(void);
extern buffer_t *create_scratch(void);
