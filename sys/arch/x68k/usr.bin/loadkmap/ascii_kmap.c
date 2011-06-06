/*	$NetBSD: ascii_kmap.c,v 1.6.106.1 2011/06/06 09:07:04 jruoho Exp $	*/
/* from: arch/amiga/dev/kbdmap.c */
/* modified for X680x0 by Masaru Oki */

#include <sys/cdefs.h>
__RCSID("$NetBSD: ascii_kmap.c,v 1.6.106.1 2011/06/06 09:07:04 jruoho Exp $");

#include <machine/kbdmap.h>

/* define a default keymap. This can be changed by keyboard ioctl's 
   (later at least..) */

/* mode shortcuts: */
#define	S KBD_MODE_STRING
#define C KBD_MODE_CAPS
#define K KBD_MODE_KPAD
#define D KBD_MODE_DEAD

struct kbdmap ascii_kbdmap = {
	/* normal map */
	{
	   {0, 0},	/* 0x00 */
	   {0, ESC},
	   {0, '1'},
	   {0, '2'},
	   {0, '3'},
	   {0, '4'},
	   {0, '5'},
	   {0, '6'},
	   {0, '7'},	/* 0x08 */
	   {0, '8'},
	   {0, '9'},
	   {0, '0'},
	   {0, '-'},
	   {0, '='},
	   {0, '\\'},
	   {0, DEL},	/* really BS, DEL & BS swapped */
	   {0, '\t'},	/* 0x10 */
	   {C, 'q'},
	   {C, 'w'},
	   {C, 'e'},
	   {C, 'r'},
	   {C, 't'},
	   {C, 'y'},
	   {C, 'u'},
	   {C, 'i'},	/* 0x18 */
	   {C, 'o'},
	   {C, 'p'},
	   {0, '['},
	   {0, ']'},
	   {0, '\r'},	/* return */
	   {C, 'a'},
	   {C, 's'},
	   {C, 'd'},	/* 0x20 */
	   {C, 'f'},
	   {C, 'g'},
	   {C, 'h'},
	   {C, 'j'},
	   {C, 'k'},
	   {C, 'l'},
	   {0, ';'},
	   {0, '\''},	/* 0x28 */
	   {0, '`'},
	   {C, 'z'},
	   {C, 'x'},
	   {C, 'c'},
	   {C, 'v'},
	   {C, 'b'},
	   {C, 'n'},
	   {C, 'm'},	/* 0x30 */
	   {0, ','},
	   {0, '.'},
	   {0, '/'},
	   {0, '_'},
	   {0, ' '},
	   {0, 0},	/* HOME */
	   {0, '\b'},	/* really DEL, BS & DEL swapped */
	   {0, 0},	/* 0x38 ROLLUP */
	   {0, 0},	/* ROLLDOWN */
	   {0, 0},	/* UNDO */
	   {S, 0x0C},	/* CRSR LEFT */
	   {S, 0x00},	/* now it gets hairy.. CRSR UP */
	   {S, 0x08},	/* CRSR RIGHT */
	   {S, 0x04},	/* CRSR DOWN */
	   {K, 0},	/* CLR */
	   {K, '/'},	/* 0x40 */
	   {K, '*'},
	   {K, '-'},
	   {K, '7'},
	   {K, '8'},
	   {K, '9'},
	   {K, '+'},
	   {K, '4'},
	   {K, '5'},	/* 0x48 */
	   {K, '6'},
	   {K, '='},
	   {K, '1'},
	   {K, '2'},
	   {K, '3'},
	   {K, '\r'},	/* enter */
	   {K, '0'},
	   {K, ','},	/* 0x50 */
	   {K, '.'},
	   {0, 0},
	   {0, 0},
	   {S, 0x42},	/* HELP */
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},	/* 0x58 */
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},	/* 0x60 */
	   {0, 0},
	   {0, 0},
	   {S, 0x10},	/* F1 */
	   {S, 0x15},	/* F2 */
	   {S, 0x1A},	/* F3 */
	   {S, 0x1F},	/* F4 */
	   {S, 0x24},	/* F5 */
	   {S, 0x29},	/* F6 */
	   {S, 0x2E},	/* F7 */
	   {S, 0x33},	/* 0x58 F8 */
	   {S, 0x38},	/* F9 */
	   {S, 0x3D},	/* F10 */
	},

	/* shifted map */
	{
	   {0, 0},	/* 0x00 */
	   {0, ESC},
	   {0, '!'},
	   {0, '@'},
	   {0, '#'},
	   {0, '$'},
	   {0, '%'},
	   {0, '^'},
	   {0, '&'},	/* 0x08 */
	   {0, '*'},
	   {0, '('},
	   {0, ')'},
	   {0, '_'},
	   {0, '+'},
	   {0, '|'},
	   {0, DEL},	/* really BS, DEL & BS swapped */
	   {0, '\t'},	/* 0x10 shift TAB */
	   {C, 'Q'},
	   {C, 'W'},
	   {C, 'E'},
	   {C, 'R'},
	   {C, 'T'},
	   {C, 'Y'},
	   {C, 'U'},
	   {C, 'I'},	/* 0x18 */
	   {C, 'O'},
	   {C, 'P'},
	   {0, '{'},
	   {0, '}'},
	   {0, '\r'},	/* return */
	   {C, 'A'},
	   {C, 'S'},
	   {C, 'D'},	/* 0x20 */
	   {C, 'F'},
	   {C, 'G'},
	   {C, 'H'},
	   {C, 'J'},
	   {C, 'K'},
	   {C, 'L'},
	   {0, ':'},
	   {0, '\"'},	/* 0x28 */
	   {0, '~'},
	   {C, 'Z'},	
	   {C, 'X'},
	   {C, 'C'},
	   {C, 'V'},
	   {C, 'B'},
	   {C, 'N'},
	   {C, 'M'},	/* 0x30 */
	   {0, '<'},	/* 0x38 */
	   {0, '>'},
	   {0, '?'},
	   {0, '_'},
	   {0, ' '},
	   {0, 0},	/* HOME */
	   {0, '\b'},	/* really DEL, BS & DEL swapped */
	   {0, 0},	/* 0x38 ROLLUP */
	   {0, 0},	/* ROLLDOWN */
	   {0, 0},	/* UNDO */
	   {0, 0},	/* shift CRSR LEFT */
	   {0, 0},	/* shift CRSR UP */
	   {0, 0},	/* shift CRSR RIGHT */
	   {0, 0},	/* shift CRSR DOWN */
	   {D, 0},	/* CLR */
	   {D, '/'},	/* 0x40 */
	   {D, '*'},
	   {D, '-'},
	   {D, '7'},
	   {D, '8'},
	   {D, '9'},
	   {D, '+'},
	   {D, '4'},
	   {D, '5'},	/* 0x48 */
	   {D, '6'},
	   {D, '='},
	   {D, '1'},
	   {D, '2'},
	   {D, '3'},
	   {K, '\r'},	/* enter */
	   {D, '0'},
	   {D, ','},	/* 0x50 */
	   {D, '.'},
	   {0, 0},
	   {0, 0},
	   {S, 0x42},	/* HELP */
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},	/* 0x58 */
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},	/* 0x60 */
	   {0, 0},
	   {0, 0},
	   {S, 0x5D},	/* shift F1 */
	   {S, 0x63},	/* shift F2 */
	   {S, 0x69},	/* shift F3 */
	   {S, 0x6F},	/* shift F4 */
	   {S, 0x75},	/* shift F5 */
	   {S, 0x7B},	/* shift F6 */
	   {S, 0x81},	/* shift F7 */
	   {S, 0x87},	/* 0x58 shift F8 */
	   {S, 0x8D},	/* shift F9 */
	   {S, 0x93},	/* shift F10 */
	},


	/* alt map */
	{
	},

	/* shift alt map */
	{
	},

	{	   
	  /* string table. If there's a better way to get the offsets into the
	     above table, please tell me..
	     
	     NOTE: save yourself and others a lot of grief by *not* using
	           CSI == 0x9b, using the two-character sequence gives
	           much less trouble, especially in GNU-Emacs.. */
	  
	  3, ESC, '[', 'A',		/* 0x00: CRSR UP */
	  3, ESC, '[', 'B',		/* 0x04: CRSR DOWN */
	  3, ESC, '[', 'C',		/* 0x08: CRSR RIGHT */
	  3, ESC, '[', 'D',		/* 0x0C: CRSR LEFT */
	  4, ESC, '[', '0', '~',	/* 0x10: F1 */
	  4, ESC, '[', '1', '~',	/* 0x15: F2 */
	  4, ESC, '[', '2', '~',	/* 0x1A: F3 */
	  4, ESC, '[', '3', '~',	/* 0x1F: F4 */
	  4, ESC, '[', '4', '~',	/* 0x24: F5 */
	  4, ESC, '[', '5', '~',	/* 0x29: F6 */
	  4, ESC, '[', '6', '~',	/* 0x2E: F7 */
	  4, ESC, '[', '7', '~',	/* 0x33: F8 */
	  4, ESC, '[', '8', '~',	/* 0x38: F9 */
	  4, ESC, '[', '9', '~',	/* 0x3D: F10 */
	  4, ESC, '[', '?', '~',	/* 0x42: HELP */

	  4, ESC, '[', 'T', '~',	/* 0x47: shift CRSR UP */
	  4, ESC, '[', 'S', '~',	/* 0x4C: shift CRSR DOWN */
	  5, ESC, '[', ' ', '@', '~',	/* 0x51: shift CRSR RIGHT */
	  5, ESC, '[', ' ', 'A', '~',	/* 0x57: shift CRSR LEFT */
	  5, ESC, '[', '1', '0', '~',	/* 0x5D: shift F1 */
	  5, ESC, '[', '1', '1', '~',	/* 0x63: shift F2 */
	  5, ESC, '[', '1', '2', '~',	/* 0x69: shift F3 */
	  5, ESC, '[', '1', '3', '~',	/* 0x6F: shift F4 */
	  5, ESC, '[', '1', '4', '~',	/* 0x75: shift F5 */
	  5, ESC, '[', '1', '5', '~',	/* 0x7B: shift F6 */
	  5, ESC, '[', '1', '6', '~',	/* 0x81: shift F7 */
	  5, ESC, '[', '1', '7', '~',	/* 0x87: shift F8 */
	  5, ESC, '[', '1', '8', '~',	/* 0x8D: shift F9 */
	  5, ESC, '[', '1', '9', '~',	/* 0x93: shift F10 */
	  3, ESC, '[', 'Z',		/* 0x99: shift TAB */
	  2, ESC, '[',			/* 0x9d: alt ESC == CSI */
	},
};
