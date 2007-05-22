/*	$NetBSD: kbdmap.c,v 1.5.38.1 2007/05/22 17:27:44 matt Exp $	*/

/* from: arch/amiga/dev/kbdmap.c */
/* modified for X680x0 by Masaru Oki and Makoto MINOURA */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kbdmap.c,v 1.5.38.1 2007/05/22 17:27:44 matt Exp $");

#include "kbdmap.h"

/* define a default keymap. This can be changed by keyboard ioctl's */

/* mode shortcuts: */
#define	S KBD_MODE_STRING
#define DG (KBD_MODE_DEAD | KBD_MODE_GRAVE)
#define DA (KBD_MODE_DEAD | KBD_MODE_ACUTE)
#define DC (KBD_MODE_DEAD | KBD_MODE_CIRC)
#define DT (KBD_MODE_DEAD | KBD_MODE_TILDE)
#define DD (KBD_MODE_DEAD | KBD_MODE_DIER)
#define C KBD_MODE_CAPS
#define K KBD_MODE_KPAD
#define D KBD_MODE_DEAD

struct kbdmap kbdmap;
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
	   {0, '^'},
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
	   {0, '@'},
	   {0, '['},
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
	   {0, ':'},	/* 0x28 */
	   {0, ']'},
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
	   {S, 0x4a},	/* HOME */
	   {0, '\b'},	/* really DEL, BS & DEL swapped */
	   {S, 0x43},	/* 0x38 ROLLUP */
	   {S, 0x3e},	/* ROLLDOWN */
	   {S, 0x6a},	/* UNDO */
	   {S, 0x53},	/* CRSR LEFT */
	   {S, 0x5b},	/* CRSR UP */
	   {S, 0x57},	/* CRSR RIGHT */
	   {S, 0x4f},	/* CRSR DOWN */
	   {S, 0x5f},	/* CLR */
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
	   {S, 0x64},	/* HELP */
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},	/* 0x58 */
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {S,0x39},	/* INS: not used */
	   {0, 0},
	   {0, 0},	/* 0x60 */
	   {0, 0},
	   {0, 0},
	   {S, 0x00},	/* F1 */
	   {S, 0x04},	/* F2 */
	   {S, 0x08},	/* F3 */
	   {S, 0x0c},	/* F4 */
	   {S, 0x10},	/* F5 */
	   {S, 0x16},	/* F6 */
	   {S, 0x1c},	/* F7 */
	   {S, 0x22},	/* 0x68 F8 */
	   {S, 0x28},	/* F9 */
	   {S, 0x2e},	/* F10 */
	},

	/* shifted map */
	{
	   {0, 0},	/* 0x00 */
	   {0, ESC},
	   {0, '!'},
	   {0, '\"'},
	   {0, '#'},
	   {0, '$'},
	   {0, '%'},
	   {0, '&'},
	   {0, '\''},	/* 0x08 */
	   {0, '('},
	   {0, ')'},
	   {0, 0},
	   {0, '='},
	   {0, '~'},
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
	   {0, '`'},
	   {0, '{'},
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
	   {0, '+'},
	   {0, '*'},	/* 0x28 */
	   {0, '}'},
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
	   {S, 0x4a},	/* HOME */
	   {0, '\b'},	/* really DEL, BS & DEL swapped */
	   {S, 0x43},	/* 0x38 ROLLUP */
	   {S, 0x3e},	/* ROLLDOWN */
	   {S, 0x6a},	/* UNDO */
	   {D, 0},	/* shift CRSR LEFT */
	   {D, 0},	/* shift CRSR UP */
	   {D, 0},	/* shift CRSR RIGHT */
	   {D, 0},	/* shift CRSR DOWN */
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
	   {S, 0x64},	/* HELP */
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},	/* 0x58 */
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {0, 0},
	   {S, 0x39},	/* INS: not used */
	   {0, 0},
	   {0, 0},	/* 0x60 */
	   {0, 0},
	   {0, 0},
	   {S, 0x00},	/* shift F1 */
	   {S, 0x04},	/* shift F2 */
	   {S, 0x08},	/* shift F3 */
	   {S, 0x0c},	/* shift F4 */
	   {S, 0x10},	/* shift F5 */
	   {S, 0x16},	/* shift F6 */
	   {S, 0x1c},	/* shift F7 */
	   {S, 0x22},	/* 0x68 shift F8 */
	   {S, 0x28},	/* shift F9 */
	   {S, 0x2e},	/* shift F10 */
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

	  3, ESC, 'O', 'P',		/* 0x00: F1 (k1) */
	  3, ESC, 'O', 'Q',		/* 0x04: F2 (k2) */
	  3, ESC, 'O', 'R',		/* 0x08: F3 (k3) */
	  3, ESC, 'O', 'S',		/* 0x0c: F4 (k4) */
	  5, ESC, '[', '1', '7', '~',	/* 0x10: F5 (k5) */
	  5, ESC, '[', '1', '8', '~',	/* 0x16: F6 (k6) */
	  5, ESC, '[', '1', '9', '~',	/* 0x1c: F7 (k7) */
	  5, ESC, '[', '2', '0', '~',	/* 0x22: F8 (k8) */
	  5, ESC, '[', '2', '1', '~',	/* 0x28: F9 (k9) */
	  5, ESC, '[', '2', '9', '~',	/* 0x2e: F10 (k;) */

	  4, ESC, '[', '3', '~',	/* 0x34: DEL (kD) */
	  4, ESC, '[', '2', '~',	/* 0x39: INS (kI) */
	  4, ESC, '[', '6', '~',	/* 0x3e: RDN (kN) */
	  4, ESC, '[', '5', '~',	/* 0x43: RUP (kP) */
	  1, 8,				/* 0x48: BS (kb) */

	  4, ESC, '[', '1', '~',	/* 0x4a: HOME (kh) */
	  3, ESC, '[', 'B',		/* 0x4f: down (kd) */
	  3, ESC, '[', 'D',		/* 0x53: left (kl) */
	  3, ESC, '[', 'C',		/* 0x57: right (kr) */
	  3, ESC, '[', 'A',		/* 0x5b: up (ku) */

	  4, ESC, '[', '9', '~',	/* 0x5f: CLR (kC) */
	  5, ESC, '[', '2', '8', '~',	/* 0x64: HELP (kq) */
	  4, ESC, '[', '7', '~',	/* 0x6a: UNDO */
	},
};

unsigned char acctable[KBD_NUM_ACC][64] = {
};
