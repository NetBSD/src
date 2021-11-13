/* $NetBSD: testpat.c,v 1.6 2021/11/13 20:59:13 nat Exp $ */

/*-
 * Copyright (c) 2016 Nathanial Sloss <nathanialsloss@yahoo.com.au>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: testpat.c,v 1.6 2021/11/13 20:59:13 nat Exp $");

#include <sys/types.h>
#include <sys/time.h>

#include <curses.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int colour_list[6] = {
	COLOR_YELLOW,
	COLOR_CYAN,
	COLOR_GREEN,
	COLOR_MAGENTA,
	COLOR_RED,
	COLOR_BLUE,
};
static int numcolours = (int)__arraycount(colour_list);

int main(int argc, char *argv[]) {
	int i, col, colour, line, x_limit, y_limit, colourOK, spacing;
	int xpos, ypos, spacing_residual, spacing_start, spacing_end;
	int grid_x, grid_y, **circle_pos;
	size_t ncpos;
	float grid_unit;
	const char *title = "NetBSD";
	float coord_x, circle_int;
	float a_axis, b_axis;

	if (!initscr()) {
		errx(EXIT_FAILURE, "Unknown terminal type");
	}

	curs_set(0);

	if (argc > 2) {
		endwin();
		fprintf(stderr, "Usage: %s <title>", getprogname());
		return EXIT_FAILURE;
	}

	if (argc == 2) {
		title = argv[1];
		if (strlen(title) >= (size_t)COLS) {
			endwin();
			errx(EXIT_FAILURE,
			    "Title string is longer than display cols");
		}
	}

	colourOK = has_colors();

	if (COLS < 13 || LINES < 13) {
		endwin();
		errx(EXIT_FAILURE, "Terminal size must be at least 72x25.");
	}

	if (colourOK) {
		start_color();

	    	init_pair(0, COLOR_WHITE, COLOR_BLACK);
	    	init_pair(1, COLOR_WHITE, COLOR_RED);
	    	init_pair(2, COLOR_WHITE, COLOR_GREEN);
	    	init_pair(3, COLOR_WHITE, COLOR_YELLOW);
	    	init_pair(4, COLOR_WHITE, COLOR_BLUE);
	    	init_pair(5, COLOR_WHITE, COLOR_MAGENTA);
	    	init_pair(6, COLOR_WHITE, COLOR_CYAN);
	    	init_pair(7, COLOR_BLACK, COLOR_WHITE);

		attrset(COLOR_PAIR(0));
	}

	x_limit = (COLS - 1) / 2;
	x_limit = x_limit * 2;
	y_limit = (LINES - 2) / 2;
	y_limit = y_limit * 2;
	spacing = 2 * y_limit / numcolours;
	spacing_residual = ((2 * y_limit) % numcolours) / 2;
	a_axis = y_limit / 2;
	b_axis = y_limit;
	grid_unit = b_axis / 13;
	grid_y = grid_unit;
	grid_x = grid_unit * 2;


	ncpos = y_limit * sizeof(*circle_pos)
	    + y_limit * 2 * sizeof(**circle_pos);
	circle_pos = malloc(ncpos);
	if (circle_pos == NULL) {
		endwin();
		errx(EXIT_FAILURE, "Can't allocate circle positions");
	}
	for (i = 0; i < y_limit; i++) {
	    circle_pos[i] = (void *)&circle_pos[y_limit + i * 2];
	    circle_pos[i][0] = circle_pos[i][1] = -1;
	}

	for (i = 0; i < y_limit; i++) {
		/* Draw an ellipse (looks more circular.) */
		circle_int = (i - a_axis) / a_axis;
		circle_int = 1 - powf(circle_int, 2);
		circle_int = circle_int * powf(b_axis, 2);
#if 0
		/* Draw a circle, commented out as elipse looks better.*/
		circle_int = powf(a_axis, 2) - powf(i - a_axis, 2);
#endif
		coord_x = sqrtf(circle_int);
		circle_pos[i][0] = (-coord_x + ((float)x_limit / 2));
		circle_pos[i][1] = (coord_x + ((float)x_limit / 2));
	}

	clear();

	attron(A_ALTCHARSET);
	move(0, 0);

	/* Draw a grid. */
	for (line = 1; line < y_limit; line += grid_y) {
		for (col = 1; col < x_limit; col = col + grid_x) {
			xpos = col;
			while ((xpos < col + grid_x - 1) && (xpos <
			    x_limit)) {
				mvaddch(line + grid_y - 1, xpos, 113 |
				    A_ALTCHARSET);
				xpos++;
			}
			if (xpos < x_limit)
				mvaddch(line + grid_y - 1, xpos, 110 |
				    A_ALTCHARSET);
		}
		ypos = line;
		while (ypos < line + grid_y - 1) {
			for (col = grid_x - 1; col < x_limit; col += grid_x) {
				mvaddch(ypos, col + 1, 120 | A_ALTCHARSET);
			}
			ypos++;
		}
	}

	for (line = 1; line < y_limit; line += grid_y) {
		mvaddch(line + grid_y - 1, 0, 116 | A_ALTCHARSET);
		mvaddch(line + grid_y - 1, x_limit, 117 | A_ALTCHARSET);

		ypos = line;
		while (ypos < line + grid_y - 1) {
			mvaddch(ypos, 0, 120 | A_ALTCHARSET);
			mvaddch(ypos, x_limit, 120 | A_ALTCHARSET);
			ypos++;
		}
	}

	for (col = 1; col < x_limit; col += grid_x) {
		mvaddch(0, col + grid_x - 1, 119 | A_ALTCHARSET);
		mvaddch(y_limit, col + grid_x - 1, 118 | A_ALTCHARSET);

		xpos = col;
		while ((xpos < col + grid_x - 1) && (xpos < x_limit)) {
			mvaddch(0, xpos, 113 | A_ALTCHARSET);
			mvaddch(y_limit, xpos, 113 | A_ALTCHARSET);
			xpos++;
		}
	}

	mvaddch(0, 0, 108 | A_ALTCHARSET);
	mvaddch(0, x_limit, 107 | A_ALTCHARSET);
	mvaddch(y_limit, 0, 109 | A_ALTCHARSET);
	mvaddch(y_limit, x_limit, 106 | A_ALTCHARSET);

	/* Draw a white circle. */
	for (i = 1; i < y_limit; i++) {
		for (col = circle_pos[i][0]; col <= circle_pos[i][1]; col++) {
			mvaddch(i, col, 32 | A_REVERSE);
		}
	}

	/* Add title segment. */
	for (i = roundf(1 * grid_unit); i < roundf(2 * grid_unit); i++) {
		if (colourOK)
    			attrset(COLOR_PAIR(COLOR_BLACK));
		else
			attrset(A_NORMAL);

		for (col = roundf((4 * grid_unit * 2) +
		    circle_pos[y_limit / 2][0]); col <= roundf((9 * grid_unit
		    * 2) + circle_pos[y_limit / 2][0]); col++)
			mvaddch(i, col, ' ');
	}

	i = roundf(1.4 * grid_unit);

	if (!colourOK)
		attrset(A_NORMAL);

	col = y_limit - (strlen(title) / 2) + circle_pos[y_limit / 2][0];
		mvprintw(i, col, "%s", title);

	/* Add black segments at top. */
	for (line = roundf(2 * grid_unit); line < 4 * grid_unit; line++) {
		if (colourOK)
    			attrset(COLOR_PAIR(COLOR_BLACK));
		else
			attrset(A_NORMAL);

		for (col = 0; col <= roundf((3.5 * grid_unit * 2)); col++) {
				xpos = col + circle_pos[y_limit / 2][0];
				if (xpos >= circle_pos[line][0] &&
				    xpos <= circle_pos[line][1])
					mvaddch(line, xpos, ' ');
		}

		for (col = roundf((9.5 * grid_unit * 2)); col <
		    roundf((13 * grid_unit * 2)); col++) {
				xpos = col + circle_pos[y_limit / 2][0];
				if (xpos >= circle_pos[line][0] &&
				    xpos <= circle_pos[line][1])
					mvaddch(line, xpos, ' ');
		}
	}

	/* Add black and white squares close to top. */
	int gap = (circle_pos[(int)(5 * grid_unit)][1] -
	    circle_pos[(int)(5 * grid_unit)][0]) / 13;

	for (i = roundf(3 * grid_unit); i < roundf(4 * grid_unit); i++) {
		for (xpos = 0; xpos <= x_limit; xpos += 2 * gap) {
			if (colourOK)
    				attrset(COLOR_PAIR(COLOR_BLACK));
			else
				attrset(A_NORMAL);

			for (col = xpos; col < xpos + gap; col++) {
				if (col >= circle_pos[i][0] &&
				    col <= circle_pos[i][1])
					mvaddch(i, col, ' ');
			}

			if (colourOK)
    				attrset(COLOR_PAIR(COLOR_WHITE));
			else
				attrset(A_REVERSE);

			for (col = xpos + gap ; col < xpos + (2 * gap);
			    col++) {
				if (col >= circle_pos[i][0] &&
				    col <= circle_pos[i][1])
					mvaddch(i, col, ' ');
			}
		}
	}

	/* Add colour bars. */
	for (i = 0; i < numcolours; i++) {
		colour = colour_list[i];
		if (colourOK)
	    		attrset(COLOR_PAIR(colour));
		else if (i & 1)
			attrset(A_NORMAL);
		else
			attrset(A_REVERSE);

		if (i == 0)
			spacing_start = 0;
		else
			spacing_start = (spacing * i) + spacing_residual;

		if (i == numcolours - 1)
			spacing_end = circle_pos[y_limit / 2][1];
		else
			spacing_end = (spacing * (i + 1)) + spacing_residual;

	    	for (line = roundf(4 * grid_unit); line < (y_limit / 2);
		    line++) {
			for (col = spacing_start; col < spacing_end; col++) {
				xpos = col + circle_pos[y_limit / 2][0];
				if (xpos >= circle_pos[line][0] &&
				    xpos <= circle_pos[line][1])
	    				mvprintw(line, xpos, " ");
			}
	    	}
	}

	/* Add black segment under centre line. */
	for (line = y_limit / 2; line < (9.5 * grid_unit); line++) {
		if (colourOK)
    			attrset(COLOR_PAIR(COLOR_BLACK));
		else
			attrset(A_NORMAL);

		for (col = circle_pos[line][0]; col <= circle_pos[line][1];
		    col++)
			mvaddch(line, col, ' ');

		for (col = roundf((1.5 * grid_unit * 2)); col <
		    roundf((4.3 * grid_unit * 2)); col++) {
				xpos = col + circle_pos[y_limit / 2][0];
				if (xpos >= circle_pos[line][0] &&
				    xpos < circle_pos[line][1])
					mvaddch(line, xpos, 120 | A_ALTCHARSET);
		}

		for (col = roundf((4.3 * grid_unit * 2)); col <
		    roundf((7.6 * grid_unit * 2)); col++) {
				xpos = col + circle_pos[y_limit / 2][0];
				if (xpos >= circle_pos[line][0] &&
				    xpos < circle_pos[line][1])
					mvaddch(line, xpos, '|');
		}

		for (col = roundf((7.6 * grid_unit * 2)); col <
		    roundf((11.5 * grid_unit * 2)); col++) {
				xpos = col + circle_pos[y_limit / 2][0];
				if (xpos >= circle_pos[line][0] &&
				    xpos < circle_pos[line][1])
					mvaddch(line, xpos, 97 | A_ALTCHARSET);
		}
	}

	/* Add black segment close to bottom. */
	for (line = roundf(9.5 * grid_unit); line <= (10.5 * grid_unit);
	    line++) {
		if (colourOK)
    			attrset(COLOR_PAIR(COLOR_BLACK));
		else
			attrset(A_NORMAL);

		for (col = roundf((0 * grid_unit * 2)); col <
		    roundf((4 * grid_unit * 2)); col++) {
				xpos = col + circle_pos[y_limit / 2][0];
				if (xpos >= circle_pos[line][0] &&
				    xpos < circle_pos[line][1])
					mvaddch(line, xpos, ' ');
		}

		for (col = roundf((4 * grid_unit * 2)); col <
		    roundf((6.5 * grid_unit * 2)); col++) {
				xpos = col + circle_pos[y_limit / 2][0];
				if (xpos >= circle_pos[line][0] &&
				    xpos < circle_pos[line][1])
					mvaddch(line, xpos, 97 | A_ALTCHARSET);
		}

		if (colourOK)
    			attrset(COLOR_PAIR(COLOR_WHITE));
		else
			attrset(A_REVERSE);

		for (col = roundf((6.5 * grid_unit * 2)); col <
		    roundf((9 * grid_unit * 2)); col++) {
				xpos = col + circle_pos[y_limit / 2][0];
				if (xpos >= circle_pos[line][0] &&
				    xpos < circle_pos[line][1])
					mvaddch(line, xpos, 97 | A_ALTCHARSET);
		}

		for (col = roundf((9 * grid_unit * 2)); col <
		    roundf((13 * grid_unit * 2)); col++) {
				xpos = col + circle_pos[y_limit / 2][0];
				if (xpos >= circle_pos[line][0] &&
				    xpos < circle_pos[line][1])
					mvaddch(line, xpos, ' ');
		}
	}

	/* Add name segment close to bottom. */
	for (line = roundf(10.5 * grid_unit); line < (12 * grid_unit);
	    line++) {
		if (colourOK)
    			attrset(COLOR_PAIR(COLOR_BLACK));
		else
			attrset(A_NORMAL);

		for (col = roundf(3.5 * grid_unit * 2); col <= roundf(9.5 *
		    grid_unit * 2); col++) {
			xpos = col + circle_pos[y_limit / 2][0];
			if (xpos >= circle_pos[line][0] &&
			    xpos < circle_pos[line][1])
				mvaddch(line, xpos, ' ');
		}

		if (colourOK)
    			attrset(COLOR_PAIR(COLOR_WHITE));
		else
			attrset(A_REVERSE);

		for (col = roundf(0 * grid_unit * 2); col <= roundf(3.5 *
		    grid_unit * 2); col++) {
			xpos = col + circle_pos[y_limit / 2][0];
			if (xpos >= circle_pos[line][0] &&
			    xpos < circle_pos[line][1])
				mvaddch(line, xpos, ' ');
		}

		for (col = roundf(9.5 * grid_unit * 2); col <= roundf(13 *
		    grid_unit * 2); col++) {
			xpos = col + circle_pos[y_limit / 2][0];
			if (xpos >= circle_pos[line][0] &&
			    xpos < circle_pos[line][1])
				mvaddch(line, xpos, ' ');
		}
	}

	/* Add yellow segment at bottom. */
	for (line = 12 * grid_unit; line < y_limit; line++) {
		if (colourOK)
    			attrset(COLOR_PAIR(COLOR_YELLOW));
		else
    			attrset(A_REVERSE);


		for (col = circle_pos[line][0]; col <= circle_pos[line][1];
		    col++)
			mvaddch(line, col, ' ');

		if (colourOK)
    			attrset(COLOR_PAIR(COLOR_RED));
		else
    			attrset(A_NORMAL);

		for (col = roundf((6 * grid_unit * 2)); col <
		    roundf((7 * grid_unit * 2)); col++) {
				xpos = col + circle_pos[y_limit / 2][0];
				if (xpos >= circle_pos[line][0] &&
				    xpos < circle_pos[line][1])
					mvaddch(line, xpos, ' ');
		}
	}

	if (colourOK)
    		attrset(COLOR_PAIR(COLOR_BLACK));
	else
    		attrset(A_NORMAL);

	for (line = 6 * grid_unit; line <= (7 * grid_unit) + 1; line++) {
		if (colourOK)
    			attrset(COLOR_PAIR(COLOR_BLACK));
		else
    			attrset(A_NORMAL);

		col = x_limit / 2;
		if (line != a_axis) {
			mvaddch(line, col - 1, ' ');
			mvaddch(line, col, 120 | A_ALTCHARSET);
			mvaddch(line, col + 1, ' ');
		}
	}

	line = y_limit / 2;
	for (col = 1; col < x_limit; col = col + grid_x) {
		xpos = col;
		while (xpos < col + grid_x - 1) {
			if (xpos >= circle_pos[line][0]
			    && xpos < circle_pos[line][1])
				mvaddch(line, xpos, 113 | A_ALTCHARSET);
			xpos++;
		}
		if (xpos >= circle_pos[line][0] && xpos < circle_pos[line][1])
			mvaddch(line, xpos, 110 | A_ALTCHARSET);
	}

	line = y_limit / 2;
	col = x_limit / 2;
	mvaddch(line, col, 110 | A_ALTCHARSET);
	mvaddch(line, col - 1, 113 | A_ALTCHARSET);
	mvaddch(line, col + 1, 113 | A_ALTCHARSET);

	refresh();

	getch();

	endwin();

	return EXIT_SUCCESS;
}

