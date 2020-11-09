/* $Header: /cvsroot/src/games/warp/sm.c,v 1.1 2020/11/09 23:37:05 kamil Exp $ */

/* $Log: sm.c,v $
/* Revision 1.1  2020/11/09 23:37:05  kamil
/* Add Warp Kit, Version 7.0 by Larry Wall
/*
/* Warp is a real-time space war game that doesn't get boring very quickly.
/* Read warp.doc and the manual page for more information.
/*
/* games/warp originally distributed with 4.3BSD-Reno, is back to the BSD
/* world via NetBSD. Its remnants were still mentioned in games/Makefile.
/*
/* Larry Wall, the original author and the copyright holder, generously
/* donated the game and copyright to The NetBSD Foundation, Inc.
/*
/* Import the game sources as-is from 4.3BSD-Reno, with the cession
/* of the copyright and license to BSD-2-clause NetBSD-style.
/*
/* Signed-off-by: Larry Wall <larry@wall.org>
/* Signed-off-by: Kamil Rytarowski <kamil@netbsd.org>
/*
 * Revision 7.0  86/10/08  15:13:35  lwall
 * Split into separate files.  Added amoebas and pirates.
 * 
 */

#include <stdio.h>
#include <ctype.h>
#include "config.h"

main()
{
    char screen[23][90], buf[10];
    Reg1 int y;
    Reg2 int x;
    int tmpy, tmpx;

    for (x=0; x<79; x++)
	screen[0][x] = ' ';
    screen[0][79] = '\0';
    
    fgets(screen[0],90,stdin);
    if (isdigit(screen[0][0])) {
	int numstars = atoi(screen[0]);

	for (y=0; y<23; y++) {
	    for (x=0; x<79; x++)
		screen[y][x] = ' ';
	    screen[y][79] = '\0';
	}
	
	for ( ; numstars; numstars--) {
	    scanf("%d %d\n",&tmpy,&tmpx);
	    y = tmpy;
	    x = tmpx;
	    screen[y][x+x] = '*';
	}

	for (y=0; y<23; y++) {
	    printf("%s\n",screen[y]);
	}
    }
    else {
	Reg3 int numstars = 0;

	for (y=1; y<23; y++) {
	    for (x=0; x<79; x++)
		screen[y][x] = ' ';
	    screen[y][79] = '\0';
	}
	
	for (y=1; y<23; y++) {
	    fgets(screen[y],90,stdin);
	}

	for (y=0; y<23; y++) {
	    for (x=0; x<80; x += 2) {
		if (screen[y][x] == '*') {
		    numstars++;
		}
		else if (screen[y][x] == '\t' || screen[y][x+1] == '\t') {
		    fprintf(stderr,"Cannot have tabs in starmap--please expand.\n");
		    exit(1);
		}
	    }
	}

	printf("%d\n",numstars);

	for (y=0; y<23; y++) {
	    for (x=0; x<80; x += 2) {
		if (screen[y][x] == '*') {
		    printf("%d %d\n",y,x/2);
		}
	    }
	}
    }
    exit(0);
}
