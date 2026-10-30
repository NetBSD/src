/* key.c, Atto Emacs, Public Domain, Hugh Barney, 2016, Derived from: Anthony's Editor January 93 */

#include "header.h"

struct mapkey {
	const char *key_desc;   	/* name of bound function */
	const char *key_bytes;  	/* the string of bytes when this key is pressed */
	void (*func)(void);
};

struct cursorkey {
        const char *key_desc;
        int key_val;
        void (*func)(void);
};

static const struct cursorkey curskeymap[] = {
	{"backspace delete-left    ", KEY_BACKSPACE, backsp },
	{"down next-line           ", KEY_DOWN, down },
	{"up previous-line         ", KEY_UP, up },
	{"left backward-character  ", KEY_LEFT, left },
	{"right forward-character  ", KEY_RIGHT, right },
	{"home beginning-of-line   ", KEY_HOME, lnbegin },
	{"end end-of-line          ", KEY_END, lnend },
	{"pgup backward-page       ", KEY_PPAGE, pgup },
	{"pgdn forward-page        ", KEY_NPAGE, pgdown },
	{"resize resize-terminal   ", KEY_RESIZE, resize_terminal },
	{"del forward-delete-char  ", KEY_DC, delete },
	{"ins toggle-overwrite-mode", KEY_IC, toggle_overwrite_mode },
	{"K_ERROR                  ", -1, NULL }
};

/* desc, keys, func */
static const struct mapkey keymap[] = {
	{"C-a beginning-of-line    ", "\x01", lnbegin },
	{"C-b backward-char        ", "\x02", left },
	{"C-d delete               ", "\x04", delete },
	{"C-e end-of-line          ", "\x05", lnend },
	{"C-f foward-char          ", "\x06", right },
	{"C-h backspace            ", "\x7F", backsp },
	{"C-k kill-to-eol          ", "\x0B", killtoeol },
	{"C-l refresh              ", "\x0C", redraw },
	{"C-n next-line            ", "\x0E", down },
	{"C-p previous-line        ", "\x10", up },
	{"C-r search               ", "\x12", search },
	{"C-s search               ", "\x13", search },
	{"C-v forward-page         ", "\x16", pgdown },
	{"C-w kill-region          ", "\x17", cut},
	{"C-y yank                 ", "\x19", paste},
	{"C-space set-mark         ", "\x00", iblock },  /* ctrl-space */
	{"C-x 1 delete-other-window", "\x18\x31", delete_other_windows },
	{"C-x 2 split-window       ", "\x18\x32", split_window },
	{"C-x o other-window       ", "\x18\x6F", next_window },
	{"C-x = cursor-position    ", "\x18\x3D", showpos },
	{"C-x i insert-file        ", "\x18\x69", insertfile },
	{"C-x k kill-buffer        ", "\x18\x6B", killbuffer },
	{"C-x C-n next-buffer      ", "\x18\x0E", next_buffer },
	{"C-x n next-buffer        ", "\x18\x6E", next_buffer },
	{"C-x C-f find-file        ", "\x18\x06", readfile },
	{"C-x C-s save-buffer      ", "\x18\x13", savebuffer },  
	{"C-x C-w write-file       ", "\x18\x17", writefile },  /* write and prompt for name */
	{"C-x C-c exit             ", "\x18\x03", quit_ask },
	{"esc b back-word          ", "\x1B\x62", wleft },
	{"esc f forward-word       ", "\x1B\x66", wright },
	{"esc g gotoline           ", "\x1B\x67", gotoline },
	{"esc k kill-region        ", "\x1B\x6B", cut },
	{"esc r query-replace      ", "\x1B\x72", query_replace },
	{"esc v backward-page      ", "\x1B\x76", pgup },
	{"esc w copy-region        ", "\x1B\x77", copy},
	{"esc @ set-mark           ", "\x1B\x40", iblock },  /* esc-@ */
	{"esc < beg-of-buf         ", "\x1B\x3C", top },
	{"esc > end-of-buf         ", "\x1B\x3E", bottom },
#ifndef SMALL
	{"esc esc show-menu        ", "\x1B\x1B", menu_escape },
#endif
	{"K_ERROR                  ", NULL, NULL }
};

char_t *get_key(void (**command_out)(void))
{
	const struct cursorkey *ck;
	const struct mapkey *k;
	int submatch;
	int res;
	static char_t buffer[K_BUFFER_LENGTH];
	static char_t *record = buffer;

	*command_out = NULL;

	/* if recorded bytes remain, return next recorded byte. */
	if (*record != '\0') {
		return record++;
	}
	/* reset record buffer. */
	record = buffer;

	do {
		assert(K_BUFFER_LENGTH > record - buffer);
		/* read and record one byte. */
		res = getch();
		if (res == ERR) {
			*record = '\0';
			return record; /* empty string */
		}

		for (ck = curskeymap; ck->func != NULL; ++ck) {
			if (ck->key_val == res) {
				*record = '\0';
				*command_out = ck->func;
				return record; /* empty string */
			}
		}

		*record++ = (char_t)res;
		*record = '\0';

		/* if recorded bytes match any multi-byte sequence... */
		for (k = keymap, submatch = 0; k->key_bytes != NULL; ++k) {
			const char_t *p, *q;

			for (p = buffer, q = (const char_t *)k->key_bytes; *p == *q; ++p, ++q) {
			        /* an exact match */
				if (*q == '\0' && *p == '\0') {
	    				record = buffer;
					*record = '\0';
					*command_out = k->func;
					return record; /* empty string */
				}
			}
			/* record bytes match part of a command sequence */
			if (*p == '\0' && *q != '\0') {
				submatch = 1;
			}
		}
	} while (submatch);
	/* nothing matched, return recorded bytes. */
	record = buffer;
	return (record++);
}

int getinput(const char *prompt, char *buf, int nbuf, int flag)
{
	int cpos = 0;
	int c;
	int start_col = strlen(prompt);

	mvaddstr(MSGLINE, 0, prompt);
	clrtoeol();

	if (flag == F_CLEAR) buf[0] = '\0';

	/* if we have a default value print it and go to end of it */
	if (buf[0] != '\0') {
		addstr(buf);
		cpos = strlen(buf);
	}

	for (;;) {
		refresh();
		c = getch();
		/* ignore control keys other than backspace, cr, lf */
		if (c < 32 && c != 0x07 && c != 0x08 && c != 0x0a && c != 0x0d)
			continue;

		switch(c) {
		case 0x0a: /* cr, lf */
		case 0x0d:
			buf[cpos] = '\0';
			return (cpos > 0 ? TRUE : FALSE);

		case 0x07: /* ctrl-g */
			return FALSE;

		case 0x7f: /* del, erase */
		case 0x08: /* backspace */
			if (cpos == 0)
				continue;

			move(MSGLINE, start_col + cpos - 1);
			addch(' ');
			move(MSGLINE, start_col + cpos - 1);
			buf[--cpos] = '\0';
			break;

		default:
			if (cpos < nbuf -1) {
				addch(c);
				buf[cpos++] = c;
				buf[cpos] ='\0';
			}
			break;
		}
	}
}
