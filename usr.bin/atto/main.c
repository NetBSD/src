/* main.c, Atto Emacs, Public Domain, Hugh Barney, 2016, Derived from: Anthony's Editor January 93 */

#include "header.h"

int done;
int use_color;
point_t nscrap;
char_t *scrap;

char_t *input;
int msgflag;
char msgline[TEMPBUF];
char temp[TEMPBUF];
char searchtext[STRBUF_M];
char replace[STRBUF_M];

buffer_t *curbp;			/* current buffer */
buffer_t *bheadp;			/* head of list of buffers */
window_t *curwp;
window_t *wheadp;

static void
usage(void)
{
	(void)fprintf(stderr, "usage:\tatto [-C] [file ...]\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	void (*command)(void);
	int ch;
	int i;

	while ((ch = getopt(argc, argv, "C")) != -1) {
		switch (ch) {
		case 'C':
			use_color = 1;
			break;
		default:
			usage();
			break;
		}
	}

	setlocale(LC_ALL, "") ; /* required for 3,4 byte UTF8 chars */
	if (initscr() == NULL) fatal("%s: Failed to initialize the screen.\n");
	raw();
	noecho();
	keypad(stdscr, TRUE);
	idlok(stdscr, TRUE);

	argc -= optind;
	argv += optind;

	if (use_color) {
		start_color();
		init_pair(ID_DEFAULT, COLOR_WHITE, COLOR_BLACK);         /* alpha */
		init_pair(ID_SYMBOL, COLOR_CYAN, COLOR_BLACK);           /* non alpha, non digit */
		init_pair(ID_MODELINE, COLOR_BLACK, COLOR_WHITE);        /* modeline */
		init_pair(ID_DIGITS, COLOR_YELLOW, COLOR_BLACK);         /* digits */
		init_pair(ID_BLOCK_COMMENT, COLOR_GREEN, COLOR_BLACK);   /* block comments */
		init_pair(ID_LINE_COMMENT, COLOR_GREEN, COLOR_BLACK);    /* line comments */
		init_pair(ID_SINGLE_STRING, COLOR_YELLOW, COLOR_BLACK);  /* single quoted strings */
		init_pair(ID_DOUBLE_STRING, COLOR_YELLOW, COLOR_BLACK);  /* double quoted strings */
	}

	if (argc) {
		for (i = 0; i < argc; ++i) {
			curbp = find_buffer(argv[i], TRUE);
			(void) insert_file(argv[i], FALSE);
			/* Save filename regardless of load() success. */
			strlcpy(curbp->b_fname, argv[i], sizeof(curbp->b_fname));
		}
	} else {
		curbp = create_scratch();
	}

	wheadp = curwp = new_window();
	one_window(curwp);
	associate_b2w(curbp, curwp);

#ifndef SMALL
	create_menu();
#endif

	if (!growgap(curbp, CHUNK)) fatal("%s: Failed to allocate required memory.\n");

	while (!done) {
#ifndef SMALL
		if (menu_mode) {
			ch = getch();
			handle_menus(ch);
			continue;
		}
#endif

		update_display();

		input = get_key(&command);

		if (command != NULL) {
			command();
		} else {
			/* allow TAB and NEWLINE, otherwise any Control Char is 'Not bound' */
			if (*input > 31 || *input == 10 || *input == 9)
				insert();
                        else {
				flushinp(); /* discard without writing in buffer */
				msg("Not bound - try ctrl+x+w to write and ctrl+x+c to exit");
			}
		}
	}

	if (scrap != NULL) free(scrap);
	move(LINES-1, 0);
	refresh();
	noraw();
	endwin();
	return 0;
}

void fatal(const char *msg)
{
	if (curscr != NULL) {
		move(LINES-1, 0);
		refresh();
		noraw();
		endwin();
		putchar('\n');
	}
	fprintf(stderr, msg, PROG_NAME);
	exit(1);
}

void msg(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	(void)vsnprintf(msgline, sizeof(msgline), msg, args);
	va_end(args);
	msgflag = TRUE;
}
