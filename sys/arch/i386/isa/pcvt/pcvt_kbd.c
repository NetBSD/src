/*
 * Copyright (c) 1992,1993,1994 Hellmuth Michaelis, Brian Dunford-Shore,
 *                              Joerg Wunsch and Holger Veit.
 *
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
 *
 * This code is derived from software contributed to 386BSD by
 * Holger Veit.
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
 *	This product includes software developed by Hellmuth Michaelis,
 *	Brian Dunford-Shore and Joerg Wunsch.
 * 4. The name authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @(#)pcvt_kbd.c, 3.00, Last Edit-Date: [Sun Feb 27 17:04:53 1994]
 *
 */

/*---------------------------------------------------------------------------*
 *
 *	pcvt_kbd.c	VT220 Driver Keyboard Interface Code
 *	----------------------------------------------------
 *	-jw	include rather primitive X stuff
 *	-hm	patch from Gordon L. Burditt to fix repeat of CAPS LOCK etc.
 *	-hm	netbsd: no file ddb.h available
 *	-hm	new keyboard routines from netbsd-current
 *	-hm	re-introduced keyboard id detection
 *	-hm	keyboard id debugging, hp sends special id ...
 *	-jw	keyboard overlay table now malloc'ed, SCROLL_SLEEP
 *	-hm	keyboard initialization for ddb activists ...
 *	-hm	keyboard security no longer ignored
 *	-hm	reset cursor when soft reset fkey
 *	-jw	USL VT compatibility
 *	-hm	CTRL-ALT-Fx switches to virtual terminal x
 *	-hm	added totalscreen checking, removed bug in vt220-like kbd part
 *	-hm	SCROLL_SLEEPing works
 *	-hm	kbd scancode sets 1 and 2 (patch from Onno van der Linden)
 *	-hm	printscreen key fix
 *	-jw	mouse emulation mode
 *	-jw/hm	all ifdef's converted to if's 
 *	-jw	cleaned up the key prefix recognition
 *	-jw	included PCVT_META_ESC
 *	-hm	removed typo in cfkey11() (pillhuhns have good eyes ..)
 *	-hm	Debugger("kbd") patch from Joerg for FreeBSD
 *	-hm	bugfix, ALT-F12 steps over screen 0
 *	-hm	vgapage() -> do_vgapage(), checks if in scrolling
 *	-hm	------------ Release 3.00 --------------
 *
 *---------------------------------------------------------------------------*/

#include "vt.h"
#if NVT > 0

#include "pcvt_hdr.h"		/* global include */

static void fkey1(void), fkey2(void),  fkey3(void),  fkey4(void);
static void fkey5(void), fkey6(void),  fkey7(void),  fkey8(void);
static void fkey9(void), fkey10(void), fkey11(void), fkey12(void);

static void sfkey1(void), sfkey2(void),  sfkey3(void),  sfkey4(void);
static void sfkey5(void), sfkey6(void),  sfkey7(void),  sfkey8(void);
static void sfkey9(void), sfkey10(void), sfkey11(void), sfkey12(void);

static void cfkey1(void), cfkey2(void),  cfkey3(void),  cfkey4(void);
static void cfkey5(void), cfkey6(void),  cfkey7(void),  cfkey8(void);
static void cfkey9(void), cfkey10(void), cfkey11(void), cfkey12(void);

static void	doreset ( void );
static void	ovlinit ( int force );
static void 	settpmrate ( int rate );
static void	setlockkeys ( int snc );
static int	kbc_8042cmd ( int val );
static int	getokeydef ( unsigned key, struct kbd_ovlkey *thisdef );
static int 	getckeydef ( unsigned key, struct kbd_ovlkey *thisdef );
static int	rmkeydef ( int key );
static int	setkeydef ( struct kbd_ovlkey *data );
static u_char *	xlatkey2ascii( U_short key );

static int	ledstate  = 0;	/* keyboard led's */
static int	tpmrate   = KBD_TPD500|KBD_TPM100;
static u_char	altkpflag = 0;
static u_short	altkpval  = 0;

#if PCVT_SHOWKEYS
u_char rawkeybuf[80];	
#endif	/* PCVT_SHOWKEYS */

/*---------------------------------------------------------------------------*
 *	function bound to control function key 12
 *---------------------------------------------------------------------------*/
static void
do_vgapage(int page)
{
	if(critical_scroll)
		switch_page = page;
	else
		vgapage(page);
}

/*---------------------------------------------------------------------------*
 *	this is one sub-entry for the table. the type can be either
 *	"pointer to a string" or "pointer to a function"
 *---------------------------------------------------------------------------*/
typedef struct
{
	u_char subtype;			/* subtype, string or function */
	union what
	{
		u_char *string;		/* ptr to string, null terminated */
		void (*func)();		/* ptr to function */
	} what;
} entry;

/*---------------------------------------------------------------------------*
 *	this is the "outer" table 
 *---------------------------------------------------------------------------*/
typedef struct
{
	u_short	type;			/* type of key */
	u_short	ovlindex;		/* -hv- index into overload table */
	entry 	unshift;		/* normal default codes/funcs */
	entry	shift;			/* shifted default codes/funcs */
	entry 	ctrl;			/* control default codes/funcs */
} Keycap_def;

#define IDX0		0	/* default indexvalue into ovl table */

#define STR		KBD_SUBT_STR	/* subtype = ptr to string */
#define FNC		KBD_SUBT_FNC	/* subtype = ptr to function */

#define CODE_SIZE	5

/*---------------------------------------------------------------------------*
 * the overlaytable table is a static fixed size scratchpad where all the
 * overloaded definitions are stored.
 * an entry consists of a short (holding the new type attribute) and
 * four entries for a new keydefinition.
 *---------------------------------------------------------------------------*/

#define OVLTBL_SIZE	64		/* 64 keys can be overloaded */

#define Ovl_tbl struct kbd_ovlkey

static Ovl_tbl *ovltbl;			/* the table itself */

static ovlinitflag = 0;			/* the init flag for the table */

/*
 * key codes >= 128 denote "virtual" shift/control
 * They are resolved before any keymapping is handled
 */

#if PCVT_SCANSET == 2
static u_char scantokey[] = {
/*      -0- -1- -2- -3- -4- -5- -6- -7-    This layout is valid for US only */
/*00*/   0,120,  0,116,114,112,113,123,  /* ??  F9  ??  F5  F3  F1  F2  F12 */
/*08*/   0,121,119,117,115, 16,  1,  0,  /* ??  F10 F8  F6  F4  TAB `   ??  */
/*10*/   0, 60, 44,  0, 58, 17,  2,  0,  /* ??  ALl SHl ??  CTl Q   1   ??  */
/*18*/   0,  0, 46, 32, 31, 18,  3,  0,  /* ??  Z   S   A   W   2   ??  ??  */
/*20*/   0, 48, 47, 33, 19,  5,  4,  0,  /* ??  C   X   D   E   4   3   ??  */
/*28*/   0, 61, 49, 34, 21, 20,  6,  0,  /* ??  SP  V   F   T   R   5   ??  */
/*30*/   0, 51, 50, 36, 35, 22,  7,  0,  /* ??  N   B   H   G   Y   6   ??  */
/*38*/   0,  0, 52, 37, 23,  8,  9,  0,  /* ??  ??  M   J   U   7   8   ??  */
/*40*/   0, 53, 38, 24, 25, 11, 10,  0,  /* ??  ,   K   I   O   0   9   ??  */
/*48*/   0, 54, 55, 39, 40, 26, 12,  0,  /* ??  .   /   L   ;   P   -   ??  */
/*50*/   0,  0, 41,  0, 27, 13,  0,  0,  /* ??  ??  "   ??  [   =   ??  ??  */
/*58*/  30, 57, 43, 28,  0, 29,  0,  0,  /* CAP SHr ENT ]   ??  \   ??  ??  */
/*60*/   0, 45,  0,  0,  0,  0, 15,  0,  /* ??  NL1 ??  ??  ??  ??  BS  ??  */
/*68*/   0, 93,  0, 92, 91,  0,  0,  0,  /* ??  KP1 ??  KP4 KP7 ??  ??  ??  */
/*70*/  99,104, 98, 97,102, 96,110, 90,  /* KP0 KP. KP2 KP5 KP6 KP8 ESC NUM */
/*78*/ 122,106,103,105,100,101,125,  0,  /* F11 KP+ KP3 KP- KP* KP9 LOC ??  */
/*80*/   0,  0,  0,118,127               /* ??  ??  ??  F7 SyRQ */
};

static u_char extscantokey[] = {
/*      -0- -1- -2- -3- -4- -5- -6- -7-    This layout is valid for US only */
/*00*/   0,120,  0,116,114,112,113,123,  /* ??  F9  ??  F5  F3  F1  F2  F12 */
/*08*/   0,121,119,117,115, 16,  1,  0,  /* ??  F10 F8  F6  F4  TAB `   ??  */
/*10*/   0, 62,128,  0, 58, 17,  2,  0,  /* ??  ALr vSh ??  CTr Q   1   ??  */
/*18*/   0,  0, 46, 32, 31, 18,  3,  0,  /* ??  Z   S   A   W   2   ??  ??  */
/*20*/   0, 48, 47, 33, 19,  5,  4,  0,  /* ??  C   X   D   E   4   3   ??  */
/*28*/   0, 61, 49, 34, 21, 20,  6,  0,  /* ??  SP  V   F   T   R   5   ??  */
/*30*/   0, 51, 50, 36, 35, 22,  7,  0,  /* ??  N   B   H   G   Y   6   ??  */
/*38*/   0,  0, 52, 37, 23,  8,  9,  0,  /* ??  ??  M   J   U   7   8   ??  */
/*40*/   0, 53, 38, 24, 25, 11, 10,  0,  /* ??  ,   K   I   O   0   9   ??  */
/*48*/   0, 54, 95, 39, 40, 26, 12,  0,  /* ??  .   KP/ L   ;   P   -   ??  */
/*50*/   0,  0, 41,  0, 27, 13,  0,  0,  /* ??  ??  "   ??  [   =   ??  ??  */
/*58*/  30, 57,108, 28,  0, 29,  0,  0,  /* CAP  SHr KPE ]   ??  \  ??  ??  */
/*60*/   0, 45,  0,  0,  0,  0, 15,  0,  /* ??  NL1 ??  ??  ??  ??  BS  ??  */
/*68*/   0, 81,  0, 79, 80,  0,  0,  0,  /* ??  END ??  LA  HOM ??  ??  ??  */
/*70*/  75, 76, 84, 97, 89, 83,110, 90,  /* INS DEL DA  KP5 RA  UA  ESC NUM */
/*78*/ 122,106, 86,105,124, 85,126,  0,  /* F11 KP+ PD  KP- PSc PU  Brk ??  */
/*80*/   0,  0,  0,118,127               /* ??  ??  ??  F7 SysRq */
};

#else	/* PCVT_SCANSET != 2 */

static u_char scantokey[] = {
/*       -0- -1- -2- -3- -4- -5- -6- -7-    This layout is valid for US only */
/*00*/    0,110,  2,  3,  4,  5,  6,  7,  /* ??  ESC 1   2   3   4   5   6   */
/*08*/    8,  9, 10, 11, 12, 13, 15, 16,  /* 7   8   9   0   -   =   BS  TAB */
/*10*/   17, 18, 19, 20, 21, 22, 23, 24,  /* Q   W   E   R   T   Y   U   I   */
/*18*/   25, 26, 27, 28, 43, 58, 31, 32,  /* O   P   [   ]   ENT CTl A   S   */
/*20*/   33, 34, 35, 36, 37, 38, 39, 40,  /* D   F   G   H   J   K   L   ;   */
/*28*/   41,  1, 44, 29, 46, 47, 48, 49,  /* '   `   SHl \   Z   X   C   V   */
/*30*/   50, 51, 52, 53, 54, 55, 57,100,  /* B   N   M   ,   .   /   SHr KP* */
/*38*/   60, 61, 30,112,113,114,115,116,  /* ALl SP  CAP F1  F2  F3  F4  F5  */
/*40*/  117,118,119,120,121, 90,125, 91,  /* F6  F7  F8  F9  F10 NUM LOC KP7 */
/*48*/   96,101,105, 92, 97,102,106, 93,  /* KP8 KP9 KP- KP4 KP5 KP6 KP+ KP1 */
/*50*/   98,103, 99,104,127,  0, 45,122,  /* KP2 KP3 KP0 KP. SyRq??  NL1 F11 */
/*58*/  123                               /* F12 */
};

static u_char extscantokey[] = {
/*       -0- -1- -2- -3- -4- -5- -6- -7-    This layout is valid for US only */
/*00*/    0,110,  2,  3,  4,  5,  6,  7,  /* ??  ESC 1   2   3   4   5   6   */
/*08*/    8,  9, 10, 11, 12, 13, 15, 16,  /* 7   8   9   0   -   =   BS  TAB */
/*10*/   17, 18, 19, 20, 21, 22, 23, 24,  /* Q   W   E   R   T   Y   U   I   */
/*18*/   25, 26, 27, 28,108, 58, 31, 32,  /* O   P   [   ]   KPE CTr A   S   */
/*20*/   33, 34, 35, 36, 37, 38, 39, 40,  /* D   F   G   H   J   K   L   ;   */
/*28*/   41,  1,128, 29, 46, 47, 48, 49,  /* '   `   vSh \   Z   X   C   V   */
/*30*/   50, 51, 52, 53, 54, 95, 57,124,  /* B   N   M   ,   .   KP/ SHr KP* */
/*38*/   62, 61, 30,112,113,114,115,116,  /* ALr SP  CAP F1  F2  F3  F4  F5  */
/*40*/  117,118,119,120,121, 90,126, 80,  /* F6  F7  F8  F9  F10 NUM Brk HOM */
/*48*/   83, 85,105, 79, 97, 89,106, 81,  /* UA  PU  KP- LA  KP5 RA  KP+ END */
/*50*/   84, 86, 75, 76,  0,  0, 45,122,  /* DA  PD  INS DEL ??  ??  NL1 F11 */
/*58*/  123,                              /* F12 */
};
#endif	/* PCVT_SCANSET == 2 */

static Keycap_def	key2ascii[] =
{

/* define some shorthands to make the table (almost) fit into 80 columns */
#define C (u_char *)
#define V (void *)
#define S STR
#define F FNC
#define I IDX0

/* DONT EVER OVERLOAD KEY 0, THIS IS A KEY THAT MUSTN'T EXIST */

/*      type   index   unshift        shift           ctrl         */
/*      ---------------------------------------------------------- */
/*  0*/ KBD_NONE,  I, {S,C "df"},    {S,C ""},      {S,C ""},
/*  1*/ KBD_ASCII, I, {S,C "`"},     {S,C "~"},     {S,C "`"},
/*  2*/ KBD_ASCII, I, {S,C "1"},     {S,C "!"},     {S,C "1"},
/*  3*/ KBD_ASCII, I, {S,C "2"},     {S,C "@"},     {S,C "\000"},   
/*  4*/ KBD_ASCII, I, {S,C "3"},     {S,C "#"},     {S,C "3"},      
/*  5*/ KBD_ASCII, I, {S,C "4"},     {S,C "$"},     {S,C "4"},
/*  6*/ KBD_ASCII, I, {S,C "5"},     {S,C "%"},     {S,C "5"},
/*  7*/ KBD_ASCII, I, {S,C "6"},     {S,C "^"},     {S,C "\036"},
/*  8*/ KBD_ASCII, I, {S,C "7"},     {S,C "&"},     {S,C "7"},
/*  9*/ KBD_ASCII, I, {S,C "8"},     {S,C "*"},     {S,C "9"},
/* 10*/ KBD_ASCII, I, {S,C "9"},     {S,C "("},     {S,C "9"},
/* 11*/ KBD_ASCII, I, {S,C "0"},     {S,C ")"},     {S,C "0"},
/* 12*/ KBD_ASCII, I, {S,C "-"},     {S,C "_"},     {S,C "\037"},
/* 13*/ KBD_ASCII, I, {S,C "="},     {S,C "+"},     {S,C "="},
/* 14*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 15*/ KBD_ASCII, I, {S,C "\177"},  {S,C "\010"},  {S,C "\177"}, /* BS */
/* 16*/ KBD_ASCII, I, {S,C "\t"},    {S,C "\t"},    {S,C "\t"},   /* TAB */
/* 17*/ KBD_ASCII, I, {S,C "q"},     {S,C "Q"},     {S,C "\021"},
/* 18*/ KBD_ASCII, I, {S,C "w"},     {S,C "W"},     {S,C "\027"},
/* 19*/ KBD_ASCII, I, {S,C "e"},     {S,C "E"},     {S,C "\005"},
/* 20*/ KBD_ASCII, I, {S,C "r"},     {S,C "R"},     {S,C "\022"},
/* 21*/ KBD_ASCII, I, {S,C "t"},     {S,C "T"},     {S,C "\024"},
/* 22*/ KBD_ASCII, I, {S,C "y"},     {S,C "Y"},     {S,C "\031"},
/* 23*/ KBD_ASCII, I, {S,C "u"},     {S,C "U"},     {S,C "\025"},
/* 24*/ KBD_ASCII, I, {S,C "i"},     {S,C "I"},     {S,C "\011"},
/* 25*/ KBD_ASCII, I, {S,C "o"},     {S,C "O"},     {S,C "\017"},
/* 26*/ KBD_ASCII, I, {S,C "p"},     {S,C "P"},     {S,C "\020"},
/* 27*/ KBD_ASCII, I, {S,C "["},     {S,C "{"},     {S,C "\033"},
/* 28*/ KBD_ASCII, I, {S,C "]"},     {S,C "}"},     {S,C "\035"},
/* 29*/ KBD_ASCII, I, {S,C "\\"},    {S,C "|"},     {S,C "\034"},
/* 30*/ KBD_CAPS,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 31*/ KBD_ASCII, I, {S,C "a"},     {S,C "A"},     {S,C "\001"},
/* 32*/ KBD_ASCII, I, {S,C "s"},     {S,C "S"},     {S,C "\023"},
/* 33*/ KBD_ASCII, I, {S,C "d"},     {S,C "D"},     {S,C "\004"},
/* 34*/ KBD_ASCII, I, {S,C "f"},     {S,C "F"},     {S,C "\006"},
/* 35*/ KBD_ASCII, I, {S,C "g"},     {S,C "G"},     {S,C "\007"},
/* 36*/ KBD_ASCII, I, {S,C "h"},     {S,C "H"},     {S,C "\010"},
/* 37*/ KBD_ASCII, I, {S,C "j"},     {S,C "J"},     {S,C "\n"},
/* 38*/ KBD_ASCII, I, {S,C "k"},     {S,C "K"},     {S,C "\013"},
/* 39*/ KBD_ASCII, I, {S,C "l"},     {S,C "L"},     {S,C "\014"},
/* 40*/ KBD_ASCII, I, {S,C ";"},     {S,C ":"},     {S,C ";"},
/* 41*/ KBD_ASCII, I, {S,C "'"},     {S,C "\""},    {S,C "'"},
/* 42*/ KBD_ASCII, I, {S,C "\\"},    {S,C "|"},     {S,C "\034"}, /* special */
/* 43*/ KBD_RETURN,I, {S,C "\r"},    {S,C "\r"},    {S,C "\r"},    /* RETURN */
/* 44*/ KBD_SHIFT, I, {S,C ""},      {S,C ""},      {S,C ""},  /* SHIFT left */
/* 45*/ KBD_ASCII, I, {S,C "<"},     {S,C ">"},     {S,C ""},
/* 46*/ KBD_ASCII, I, {S,C "z"},     {S,C "Z"},     {S,C "\032"},
/* 47*/ KBD_ASCII, I, {S,C "x"},     {S,C "X"},     {S,C "\030"},
/* 48*/ KBD_ASCII, I, {S,C "c"},     {S,C "C"},     {S,C "\003"},
/* 49*/ KBD_ASCII, I, {S,C "v"},     {S,C "V"},     {S,C "\026"},
/* 50*/ KBD_ASCII, I, {S,C "b"},     {S,C "B"},     {S,C "\002"},
/* 51*/ KBD_ASCII, I, {S,C "n"},     {S,C "N"},     {S,C "\016"},
/* 52*/ KBD_ASCII, I, {S,C "m"},     {S,C "M"},     {S,C "\r"},
/* 53*/ KBD_ASCII, I, {S,C ","},     {S,C "<"},     {S,C ","},
/* 54*/ KBD_ASCII, I, {S,C "."},     {S,C ">"},     {S,C "."},
/* 55*/ KBD_ASCII, I, {S,C "/"},     {S,C "?"},     {S,C "/"},
/* 56*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 57*/ KBD_SHIFT, I, {S,C ""},      {S,C ""},      {S,C ""}, /* SHIFT right */
/* 58*/ KBD_CTL,   I, {S,C ""},      {S,C ""},      {S,C ""},    /* CTL left */
/* 59*/ KBD_ASCII, I, {S,C ""},      {S,C ""},      {S,C ""}, 
/* 60*/ KBD_META,  I, {S,C ""},      {S,C ""},      {S,C ""},    /* ALT left */
#if !PCVT_NULLCHARS					      
/* 61*/ KBD_ASCII, I, {S,C " "},     {S,C " "},     {S,C " "},      /* SPACE */
#else
/* 61*/ KBD_ASCII, I,   {S,C " "},   {S,C " "},     {S,C "\000"},   /* SPACE */
#endif /* PCVT_NULLCHARS */
/* 62*/ KBD_META,  I, {S,C ""},      {S,C ""},      {S,C ""},   /* ALT right */
/* 63*/ KBD_ASCII, I, {S,C ""},      {S,C ""},      {S,C ""},
/* 64*/ KBD_CTL,   I, {S,C ""},      {S,C ""},      {S,C ""},   /* CTL right */
/* 65*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 66*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 67*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 68*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 69*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 70*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 71*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 72*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 73*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 74*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 75*/ KBD_FUNC,  I, {S,C "\033[2~"},{S,C "\033[2~"},{S,C "\033[2~"},/* INS */
/* 76*/ KBD_FUNC,  I, {S,C "\033[3~"},{S,C "\033[3~"},{S,C "\033[3~"},/* DEL */
/* 77*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 78*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 79*/ KBD_CURSOR,I, {S,C "\033[D"},{S,C "\033OD"},{S,C "\033[D"}, /* CU <- */
/* 80*/ KBD_FUNC,  I, {S,C "\033[1~"},{S,C "\033[1~"},{S,C "\033[1~"},/* HOME = FIND*/
/* 81*/ KBD_FUNC,  I, {S,C "\033[4~"},{S,C "\033[4~"},{S,C "\033[4~"},/* END = SELECT */
/* 82*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 83*/ KBD_CURSOR,I, {S,C "\033[A"},{S,C "\033OA"},{S,C "\033[A"}, /* CU ^ */
/* 84*/ KBD_CURSOR,I, {S,C "\033[B"},{S,C "\033OB"},{S,C "\033[B"}, /* CU v */
/* 85*/ KBD_FUNC,  I, {S,C "\033[5~"},{S,C "\033[5~"},{S,C "\033[5~"},/*PG UP*/
/* 86*/ KBD_FUNC,  I, {S,C "\033[6~"},{S,C "\033[6~"},{S,C "\033[6~"},/*PG DN*/
/* 87*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 88*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 89*/ KBD_CURSOR,I, {S,C "\033[C"},{S,C "\033OC"},{S,C "\033[C"}, /* CU -> */
/* 90*/ KBD_NUM,   I, {S,C ""},      {S,C ""},      {S,C ""},
/* 91*/ KBD_KP,    I, {S,C "7"},     {S,C "\033Ow"},{S,C "7"},
/* 92*/ KBD_KP,    I, {S,C "4"},     {S,C "\033Ot"},{S,C "4"},
/* 93*/ KBD_KP,    I, {S,C "1"},     {S,C "\033Oq"},{S,C "1"},
/* 94*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/* 95*/ KBD_KP,    I, {S,C "/"},     {S,C "/"},     {S,C "/"},
/* 96*/ KBD_KP,    I, {S,C "8"},     {S,C "\033Ox"},{S,C "8"},
/* 97*/ KBD_KP,    I, {S,C "5"},     {S,C "\033Ou"},{S,C "5"},
/* 98*/ KBD_KP,    I, {S,C "2"},     {S,C "\033Or"},{S,C "2"},
/* 99*/ KBD_KP,    I, {S,C "0"},     {S,C "\033Op"},{S,C "0"},
/*100*/ KBD_KP,    I, {S,C "*"},     {S,C "*"},     {S,C "*"},
/*101*/ KBD_KP,    I, {S,C "9"},     {S,C "\033Oy"},{S,C "9"},
/*102*/ KBD_KP,    I, {S,C "6"},     {S,C "\033Ov"},{S,C "6"},
/*103*/ KBD_KP,    I, {S,C "3"},     {S,C "\033Os"},{S,C "3"},
/*104*/ KBD_KP,    I, {S,C "."},     {S,C "\033On"},{S,C "."},
/*105*/ KBD_KP,    I, {S,C "-"},     {S,C "\033Om"},{S,C "-"},
/*106*/ KBD_KP,    I, {S,C "+"},     {S,C "+"},     {S,C "+"},
/*107*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/*108*/ KBD_RETURN,I, {S,C "\r"},    {S,C "\033OM"},{S,C "\r"},  /* KP ENTER */
/*109*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},       
/*110*/ KBD_ASCII, I, {S,C "\033"},  {S,C "\033"},  {S,C "\033"},
/*111*/ KBD_NONE,  I, {S,C ""},      {S,C ""},      {S,C ""},
/*112*/ KBD_FUNC,  I, {F,V fkey1},   {F,V sfkey1},  {F,V cfkey1},  /* F1 */
/*113*/ KBD_FUNC,  I, {F,V fkey2},   {F,V sfkey2},  {F,V cfkey2},  /* F2 */
/*114*/ KBD_FUNC,  I, {F,V fkey3},   {F,V sfkey3},  {F,V cfkey3},  /* F3 */
/*115*/ KBD_FUNC,  I, {F,V fkey4},   {F,V sfkey4},  {F,V cfkey4},  /* F4 */
/*116*/ KBD_FUNC,  I, {F,V fkey5},   {F,V sfkey5},  {F,V cfkey5},  /* F5 */
/*117*/ KBD_FUNC,  I, {F,V fkey6},   {F,V sfkey6},  {F,V cfkey6},  /* F6 */
/*118*/ KBD_FUNC,  I, {F,V fkey7},   {F,V sfkey7},  {F,V cfkey7},  /* F7 */
/*119*/ KBD_FUNC,  I, {F,V fkey8},   {F,V sfkey8},  {F,V cfkey8},  /* F8 */
/*120*/ KBD_FUNC,  I, {F,V fkey9},   {F,V sfkey9},  {F,V cfkey9},  /* F9 */
/*121*/ KBD_FUNC,  I, {F,V fkey10},  {F,V sfkey10}, {F,V cfkey10}, /* F10 */
/*122*/ KBD_FUNC,  I, {F,V fkey11},  {F,V sfkey11}, {F,V cfkey11}, /* F11 */
/*123*/ KBD_FUNC,  I, {F,V fkey12},  {F,V sfkey12}, {F,V cfkey12}, /* F12 */
/*124*/ KBD_KP,    I, {S,C ""},      {S,C ""},      {S,C ""},
/*125*/ KBD_SCROLL,I, {S,C ""},      {S,C ""},      {S,C ""},
/*126*/ KBD_BREAK, I, {S,C ""},      {S,C ""},      {S,C ""},
/*127*/ KBD_FUNC,  I, {S,C ""},      {S,C ""},      {S,C ""},      /* SysRq */

#undef C
#undef V
#undef S
#undef F
#undef I
};
static short	keypad2num[] = {
	7, 4, 1, -1, -1, 8, 5, 2, 0, -1, 9, 6, 3, -1, -1, -1, -1
};

#if PCVT_USL_VT_COMPAT

#define N_KEYNUMS 128

/*
 * this is the reverse mapping from keynumbers to scanset 1 codes
 * it is used to emulate the SysV-style GIO_KEYMAP ioctl cmd
 */

static u_char key2scan1[N_KEYNUMS] = {
	   0,0x29,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09, /*   0 */
	0x0a,0x0b,0x0c,0x0d,   0,0x0e,0x0f,0x10,0x11,0x12, /*  10 */
	0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x2b, /*  20 */
	0x3a,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26, /*  30 */
	0x27,0x28,   0,0x1c,0x2a,0x56,0x2c,0x2d,0x2e,0x2f, /*  40 */
	0x30,0x31,0x32,0x33,0x34,0x35,0x56,0x36,0x1d,   0, /*  50 */
	0x38,0x39,   0,   0,   0,   0,   0,   0,   0,   0, /*  60 */
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*  70 */
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*  80 */
	0x45,0x47,0x4b,0x4f,   0,   0,0x48,0x4c,0x50,0x52, /*  90 */
	0x37,0x49,0x4d,0x51,0x53,0x4a,0x4e,   0,   0,   0, /* 100 */
	0x01,   0,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42, /* 110 */
	0x43,0x44,0x57,0x58,   0,0x46,   0,0x54		   /* 120 */
};

/*
 * SysV is brain-dead enough to stick on the IBM code page 437. So we
 * have to translate our keymapping into IBM 437 (possibly losing keys),
 * in order to have the X server convert it back into ISO8859.1
 */

/* NB: this table only contains the mapping for codes >= 128 */

static u_char iso2ibm437[] =
{
	   0,     0,     0,     0,     0,     0,     0,     0,
	   0,     0,     0,     0,     0,     0,     0,     0,
	   0,     0,     0,     0,     0,     0,     0,     0,
	   0,     0,     0,     0,     0,     0,     0,     0,
	0xff,  0xad,  0x9b,  0x9c,     0,  0x9d,     0,  0x40,
	0x6f,  0x63,  0x61,  0xae,     0,     0,     0,     0,
	0xf8,  0xf1,  0xfd,  0x33,     0,  0xe6,     0,  0xfa,
	   0,  0x31,  0x6f,  0xaf,  0xac,  0xab,     0,  0xa8,
	0x41,  0x41,  0x41,  0x41,  0x8e,  0x8f,  0x92,  0x80,
	0x45,  0x90,  0x45,  0x45,  0x49,  0x49,  0x49,  0x49,
	0x81,  0xa5,  0x4f,  0x4f,  0x4f,  0x4f,  0x99,  0x4f,
	0x4f,  0x55,  0x55,  0x55,  0x9a,  0x59,     0,  0xe1,
	0x85,  0xa0,  0x83,  0x61,  0x84,  0x86,  0x91,  0x87,
	0x8a,  0x82,  0x88,  0x89,  0x8d,  0xa1,  0x8c,  0x8b,
	   0,  0xa4,  0x95,  0xa2,  0x93,  0x6f,  0x94,  0x6f,
	0x6f,  0x97,  0xa3,  0x96,  0x81,  0x98,     0,     0
};

#endif /* PCVT_USL_VT_COMPAT */


/*---------------------------------------------------------------------------*
 *	update keyboard led's	
 *---------------------------------------------------------------------------*/
void
update_led(void)
{
	ledstate = ((vsp->scroll_lock) |
		    (vsp->num_lock * 2) |
		    (vsp->caps_lock * 4));
		    
	if(kbd_cmd(KEYB_C_LEDS) != 0)
		printf("Keyboard LED command timeout\n");
	else if(kbd_cmd(ledstate) != 0)
		printf("Keyboard LED data timeout\n");	
}

/*---------------------------------------------------------------------------*
 *	set typamatic rate
 *---------------------------------------------------------------------------*/
static void
settpmrate(int rate)
{
	tpmrate = rate & 0x7f;
	if(kbd_cmd(KEYB_C_TYPEM) != 0)
		printf("Keyboard TYPEMATIC command timeout\n");
	else if(kbd_cmd(tpmrate) != 0)
		printf("Keyboard TYPEMATIC data timeout\n");	
}
 
/*---------------------------------------------------------------------------*
 *	Pass command to keyboard controller (8042)
 *---------------------------------------------------------------------------*/
static int
kbc_8042cmd(int val)
{
	unsigned timeo;

	timeo = 100000; 	/* > 100 msec */
	while (inb(CONTROLLER_CTRL) & STATUS_INPBF)
		if (--timeo == 0)
			return (-1);
	delay(6);
	outb(CONTROLLER_CTRL, val);
	return (0);
}

/*---------------------------------------------------------------------------*
 *	Pass command to keyboard itself
 *---------------------------------------------------------------------------*/
int
kbd_cmd(int val)
{
	unsigned timeo;

	timeo = 100000; 	/* > 100 msec */
	while (inb(CONTROLLER_CTRL) & STATUS_INPBF)
		if (--timeo == 0)
			return (-1);
	delay(6);
	outb(CONTROLLER_DATA, val);
	return (0);
}

/*---------------------------------------------------------------------------*
 *	Read response from keyboard
 *---------------------------------------------------------------------------*/
int
kbd_response(void)
{
	unsigned timeo;

	timeo = 500000; 	/* > 500 msec (KEYB_R_SELFOK requires 87) */
	while (!(inb(CONTROLLER_CTRL) & STATUS_OUTPBF))
		if (--timeo == 0)
			return (-1);
	delay(6);
	return ((u_char) inb(CONTROLLER_DATA));
}

/*---------------------------------------------------------------------------*
 *	try to force keyboard into a known state ..
 *---------------------------------------------------------------------------*/
static
void doreset(void)
{
	int again = 0;
	int response;

	/* Enable interrupts and keyboard, etc. */
	if (kbc_8042cmd(CONTR_WRITE) != 0)
		printf("Timeout specifying load of keyboard command byte\n");

#if PCVT_USEKBDSEC		/* security enabled */

#  if PCVT_SCANSET == 2
#    define KBDINITCMD COMMAND_SYSFLG|COMMAND_IRQEN
#  else /* PCVT_SCANSET != 2 */
#    define KBDINITCMD COMMAND_PCSCAN|COMMAND_SYSFLG|COMMAND_IRQEN
#  endif /* PCVT_SCANSET == 2 */

#else /* ! PCVT_USEKBDSEC */	/* security disabled */

#  if PCVT_SCANSET == 2
#    define KBDINITCMD COMMAND_INHOVR|COMMAND_SYSFLG|COMMAND_IRQEN
#  else /* PCVT_SCANSET != 2 */
#    define KBDINITCMD COMMAND_PCSCAN|COMMAND_INHOVR|COMMAND_SYSFLG\
	|COMMAND_IRQEN
#  endif /* PCVT_SCANSET == 2 */

#endif /* PCVT_USEKBDSEC */

	if (kbd_cmd(KBDINITCMD) != 0)
		printf("Timeout writing keyboard command byte\n");
	
	/*
	 * Discard any stale keyboard activity.  The 0.1 boot code isn't
	 * very careful and sometimes leaves a KEYB_R_RESEND.
	 */
	while (inb(CONTROLLER_CTRL) & STATUS_OUTPBF)
		kbd_response();

	/* Start keyboard reset */
	if (kbd_cmd(KEYB_C_RESET) != 0)
		printf("Timeout for keyboard reset command\n");

	/* Wait for the first response to reset and handle retries */
	while ((response = kbd_response()) != KEYB_R_ACK) {
		if (response < 0) {
			printf("Timeout for keyboard reset ack byte #1\n");
			response = KEYB_R_RESEND;
		}
		if (response == KEYB_R_RESEND) {
			if (!again) {
				printf("KEYBOARD disconnected: RECONNECT\n");
				again = 1;
			}
			if (kbd_cmd(KEYB_C_RESET) != 0)
				printf("Timeout for keyboard reset command\n");
		}
		/*
		 * Other responses are harmless.  They may occur for new
		 * keystrokes.
		 */
	}

	/* Wait for the second response to reset */
	while ((response = kbd_response()) != KEYB_R_SELFOK) {
		if (response < 0) {
			printf("Timeout for keyboard reset ack byte #2\n");
			/*
			 * If KEYB_R_SELFOK never arrives, the loop will
			 * finish here unless the keyboard babbles or
			 * STATUS_OUTPBF gets stuck.
			 */
			break;
		}
	}

#if PCVT_KEYBDID

	if(kbd_cmd(KEYB_C_ID) != 0)
	{
		printf("Timeout for keyboard ID command\n");
		keyboard_type = KB_UNKNOWN;
	}
	else
	{

r_entry:

		if((response = kbd_response()) == KEYB_R_MF2ID1)
		{
			if((response = kbd_response()) == KEYB_R_MF2ID2)
			{
				keyboard_type = KB_MFII;
			}
			else if(response == KEYB_R_MF2ID2HP)
			{
				keyboard_type = KB_MFII;
			}				
			else
			{
				printf("\nkbdid, response 2 = [%d]\n",
				       response);
				keyboard_type = KB_UNKNOWN;
			}
		}	
		else if (response == KEYB_R_ACK)
		{
			goto r_entry;
		}
		else if (response == -1)		
		{
			keyboard_type = KB_AT;
		}			
		else
		{
			printf("\nkbdid, response 1 = [%d]\n", response);
		}
	}

#else /* PCVT_KEYBDID */

	keyboard_type = KB_MFII;	/* force it .. */

#endif /* PCVT_KEYBDID */

}

/*---------------------------------------------------------------------------*
 *	init keyboard code
 *---------------------------------------------------------------------------*/
void
kbd_code_init(void)
{
	doreset();
	ovlinit(0);
	keyboard_is_initialized = 1;
}

/*---------------------------------------------------------------------------*
 *	init keyboard code, this initializes the keyboard subsystem
 *	just "a bit" so the very very first ddb session is able to
 *	get proper keystrokes, in other words, it's a hack ....
 *---------------------------------------------------------------------------*/
void
kbd_code_init1(void)
{
	doreset();
	keyboard_is_initialized = 1;
}

/*---------------------------------------------------------------------------*
 *	init keyboard overlay table	
 *---------------------------------------------------------------------------*/
static
void ovlinit(int force) 
{
	register i;
	
	if(force || ovlinitflag==0)
	{
		if(ovlinitflag == 0 &&
		   (ovltbl = (Ovl_tbl *)malloc(sizeof(Ovl_tbl) * OVLTBL_SIZE,
					       M_DEVBUF, M_WAITOK)) == NULL)
			panic("pcvt_kbd: malloc of Ovl_tbl failed");

		for(i=0; i<OVLTBL_SIZE; i++)
		{
			ovltbl[i].keynum = 
			ovltbl[i].type = 0;
			ovltbl[i].unshift[0] =
			ovltbl[i].shift[0] = 
			ovltbl[i].ctrl[0] =
			ovltbl[i].altgr[0] = 0;
			ovltbl[i].subu =
			ovltbl[i].subs =
			ovltbl[i].subc =
			ovltbl[i].suba = KBD_SUBT_STR;	/* just strings .. */
		}
		for(i=0; i<=MAXKEYNUM; i++)
			key2ascii[i].type &= KBD_MASK;
		ovlinitflag = 1;
	}
}

/*---------------------------------------------------------------------------*
 *	get original key definition
 *---------------------------------------------------------------------------*/
static int
getokeydef(unsigned key, Ovl_tbl *thisdef)
{
	if(key == 0 || key > MAXKEYNUM)
		return EINVAL;
	
	thisdef->keynum = key;
	thisdef->type = key2ascii[key].type;

	if(key2ascii[key].unshift.subtype == STR)
	{
		bcopy((u_char *)(key2ascii[key].unshift.what.string),
		       thisdef->unshift, CODE_SIZE);
		thisdef->subu = KBD_SUBT_STR;
	}
	else
	{
		bcopy("", thisdef->unshift, CODE_SIZE);
		thisdef->subu = KBD_SUBT_FNC;
	}

	if(key2ascii[key].shift.subtype == STR)
	{
		bcopy((u_char *)(key2ascii[key].shift.what.string),
		       thisdef->shift, CODE_SIZE);
		thisdef->subs = KBD_SUBT_STR;
	}
	else
	{
		bcopy("",thisdef->shift,CODE_SIZE);
		thisdef->subs = KBD_SUBT_FNC;
	}		
	
	if(key2ascii[key].ctrl.subtype == STR)
	{
		bcopy((u_char *)(key2ascii[key].ctrl.what.string),
		       thisdef->ctrl, CODE_SIZE);
		thisdef->subc = KBD_SUBT_STR;
	}
	else
	{
		bcopy("",thisdef->ctrl,CODE_SIZE);	
		thisdef->subc = KBD_SUBT_FNC;
	}
	
	/* deliver at least anything for ALTGR settings ... */		

	if(key2ascii[key].unshift.subtype == STR)
	{
		bcopy((u_char *)(key2ascii[key].unshift.what.string),
		       thisdef->altgr, CODE_SIZE);
		thisdef->suba = KBD_SUBT_STR;
	}
	else
	{
		bcopy("",thisdef->altgr, CODE_SIZE);
		thisdef->suba = KBD_SUBT_FNC;
	}
	return 0;
}

/*---------------------------------------------------------------------------*
 *	get current key definition
 *---------------------------------------------------------------------------*/
static int
getckeydef(unsigned key, Ovl_tbl *thisdef) 
{
	u_short type = key2ascii[key].type;

	if(key>MAXKEYNUM) 
		return EINVAL;

	if(type & KBD_OVERLOAD)
		*thisdef = ovltbl[key2ascii[key].ovlindex];
	else
		getokeydef(key,thisdef);

	return 0;
}

/*---------------------------------------------------------------------------*
 *	translate keynumber and returns ptr to associated ascii string
 *	if key is bound to a function, executes it, and ret empty ptr
 *---------------------------------------------------------------------------*/
static u_char *
xlatkey2ascii(U_short key)
{
	static u_char	capchar[2] = {0, 0};
#if PCVT_META_ESC
	static u_char	metachar[3] = {0x1b, 0, 0};
#else
	static u_char	metachar[2] = {0, 0};
#endif
	static Ovl_tbl	thisdef;
	int		n;
	void		(*fnc)();
	
	if(key==0)			/* ignore the NON-KEY */
		return 0;
	
	getckeydef(key&0x7F, &thisdef);	/* get the current ASCII value */

	thisdef.type &= KBD_MASK;

	if(key&0x80)			/* special handling of ALT-KEYPAD */
	{
		/* is the ALT Key released? */
		if(thisdef.type==KBD_META || thisdef.type==KBD_ALTGR)
		{
			if(altkpflag)	/* have we been in altkp mode? */
			{
				capchar[0] = altkpval;
				altkpflag = 0;
				altkpval = 0;
				return capchar;
			}
		}
		return 0;
	}

	switch(thisdef.type)		/* convert the keys */
	{
		case KBD_BREAK:
		case KBD_ASCII:
		case KBD_FUNC:
			fnc = NULL;
			more_chars = NULL;
			
			if(altgr_down)
				more_chars = (u_char *)thisdef.altgr;
	
			else if(shift_down || vsp->shift_lock)
			{
				if(key2ascii[key].shift.subtype == STR)
					more_chars = (u_char *)thisdef.shift;
				else
					fnc = key2ascii[key].shift.what.func;
			}
				
			else if(ctrl_down)
			{
				if(key2ascii[key].ctrl.subtype == STR)
					more_chars = (u_char *)thisdef.ctrl;
				else
					fnc = key2ascii[key].ctrl.what.func;
			}
	
			else
			{
				if(key2ascii[key].unshift.subtype == STR)
					more_chars = (u_char *)thisdef.unshift;
				else
					fnc = key2ascii[key].unshift.what.func;
			}
	
			if(fnc)
				(*fnc)();	/* execute function */
					
			if((more_chars != NULL) && (more_chars[1] == 0))
			{
				if(vsp->caps_lock && more_chars[0] >= 'a'
				   && more_chars[0] <= 'z')
				{
					capchar[0] = *more_chars - ('a'-'A');
					more_chars = capchar;
				}
				if(meta_down)
				{
#if PCVT_META_ESC
					metachar[1] = *more_chars;
#else
					metachar[0] = *more_chars | 0x80;
#endif
					more_chars = metachar;
				}
			}
			return(more_chars);

		case KBD_KP:
			fnc = NULL;
			more_chars = NULL;
	
			if(meta_down)
			{
				switch(key)
				{
					case 95:	/* / */
						altkpflag = 0;
						more_chars =
						 (u_char *)"\033OQ";
						return(more_chars);
						
					case 100:	/* * */
						altkpflag = 0;
						more_chars =
						 (u_char *)"\033OR";
						return(more_chars);
						
					case 105:	/* - */
						altkpflag = 0;
						more_chars =
						 (u_char *)"\033OS";
						return(more_chars);
				}
			}

			if(meta_down || altgr_down)				
			{
				if((n = keypad2num[key-91]) >= 0)
				{
					if(!altkpflag)
					{
						/* start ALT-KP mode */
						altkpflag = 1;
						altkpval = 0;
					}
					altkpval *= 10;
					altkpval += n;
				}
				else
					altkpflag = 0;
				return 0;
			} 
	
			if(!(vsp->num_lock))
			{
				if(key2ascii[key].shift.subtype == STR)
					more_chars = (u_char *)thisdef.shift;
				else
					fnc = key2ascii[key].shift.what.func;
			}
			else
			{
				if(key2ascii[key].unshift.subtype == STR)
					more_chars = (u_char *)thisdef.unshift;
				else
					fnc = key2ascii[key].unshift.what.func;
			}
	
			if(fnc)
				(*fnc)();	/* execute function */
			return(more_chars);
	
		case KBD_CURSOR:
			fnc = NULL;
			more_chars = NULL;
	
			if(vsp->ckm)
			{
				if(key2ascii[key].shift.subtype == STR)
					more_chars = (u_char *)thisdef.shift;
				else
					fnc = key2ascii[key].shift.what.func;
			}
			else
			{
				if(key2ascii[key].unshift.subtype == STR)
					more_chars = (u_char *)thisdef.unshift;
				else
					fnc = key2ascii[key].unshift.what.func;
			}
	
			if(fnc)
				(*fnc)();	/* execute function */
			return(more_chars);

		case KBD_NUM:		/*  special kp-num handling */
			more_chars = NULL;
			
			if(meta_down)
			{
				more_chars = (u_char *)"\033OP"; /* PF1 */
			}
			else
			{
				vsp->num_lock ^= 1;
				update_led();
			}
			return(more_chars);

		case KBD_RETURN:
			more_chars = NULL;

			if(!(vsp->num_lock))
			{
				more_chars = (u_char *)thisdef.shift;
			}
			else
			{
				more_chars = (u_char *)thisdef.unshift;
			}
			if(vsp->lnm && (*more_chars == '\r'))
			{
				more_chars = (u_char *)"\r\n"; /* CR LF */
			}
			if(meta_down)
			{
#if PCVT_META_ESC
				metachar[1] = *more_chars;
#else
				metachar[0] = *more_chars | 0x80;
#endif
				more_chars = metachar;
			}
			return(more_chars);
			
		case KBD_META:		/* these keys are	*/
		case KBD_ALTGR:		/*  handled directly	*/
		case KBD_SCROLL:	/*  by the keyboard	*/
		case KBD_CAPS:		/*  handler - they are	*/
		case KBD_SHFTLOCK:	/*  ignored here	*/
		case KBD_CTL:
		case KBD_NONE:
		default:
			return 0;
	}
}

/*---------------------------------------------------------------------------*
 *	get keystrokes from the keyboard.
 *	if noblock = 0, wait until a key is pressed.
 *	else return NULL if no characters present.
 *---------------------------------------------------------------------------*/
u_char *
sgetc(int noblock)
{
	u_char		*cp;
	u_char		dt;
	u_char		key;
	u_short		type;
	
	static u_char	kbd_lastkey = 0; /* last keystroke */
	static struct
	{
		u_char extended: 1;	/* extended prefix seen */
		u_char ext1: 1;		/* extended prefix 1 seen */
		u_char breakseen: 1;	/* break code seen */
		u_char vshift: 1;	/* virtual shift pending */
		u_char vcontrol: 1;	/* virtual control pending */
		u_char sysrq: 1;	/* sysrq pressed */
	} kbd_status = {0};

#ifdef XSERVER
	static char	keybuf[2] = {0}; /* the second 0 is a delimiter! */
#endif /* XSERVER */

loop:
	/* see if there is something from the keyboard in the input buffer */

#ifdef XSERVER
	if (inb(CONTROLLER_CTRL) & STATUS_OUTPBF)
	{
		dt = inb(CONTROLLER_DATA);		/* yes, get it ! */

		/*
		 * If x mode is active, only care for locking keys, then
		 * return the scan code instead of any key translation.
		 * Additionally, this prevents us from any attempts to
		 * execute pcvt internal functions caused by keys (such
		 * as screen flipping).
		 * XXX For now, only the default locking key definitions
		 * are recognized (i.e. if you have overloaded you "A" key
		 * as NUMLOCK, that wont effect X mode:-)
		 * Changing this would be nice, but would require modifi-
		 * cations to the X server. After having this, X will
		 * deal with the LEDs itself, so we are committed.
		 */
		/*
		 * Iff PCVT_USL_VT_COMPAT is defined, the behaviour has
		 * been fixed. We need not care about any keys here, since
		 * there are ioctls that deal with the lock key / LED stuff.
		 */
		if (pcvt_kbd_raw)
		{
			keybuf[0] = dt;
#if !PCVT_USL_VT_COMPAT
			if ((dt & 0x80) == 0)
				/* key make */
				switch(dt)
				{
				case 0x45:
					/* XXX on which virt screen? */					vsp->num_lock ^= 1;
					update_led();
					break;

				case 0x3a:
					vsp->caps_lock ^= 1;
					update_led();
					break;

				case 0x46:
					vsp->scroll_lock ^= 1;
					update_led();
					break;
				}
#endif /* !PCVT_USL_VT_COMPAT */

#if PCVT_EMU_MOUSE
			/*
			 * The (mouse systems) mouse emulator. The mouse
			 * device allocates the first device node that is
			 * not used by a virtual terminal. (E.g., you have
			 * eight vtys, /dev/ttyv0 thru /dev/ttyv7, so the
			 * mouse emulator were /dev/ttyv8.)
			 * Currently the emulator only works if the keyboard
			 * is in raw (PC scan code) mode. This is the typic-
			 * al case when running the X server.
			 * It is activated if the num locks LED is active
			 * for the current vty, and if the mouse device
			 * has been opened by at least one process. It
			 * grabs the numerical keypad events (but only
			 * the "non-extended", so the separate arrow keys
			 * continue to work), and three keys for the "mouse
			 * buttons", preferrably F1 thru F3. Any of the
			 * eight directions (N, NE, E, SE, S, SW, W, NW)
			 * is supported, and frequent key presses (less
			 * than e.g. half a second between key presses)
			 * cause the emulator to accelerate the pointer
			 * movement by 6, while single presses result in
			 * single moves, so each point can be reached.
			 */
			/*
			 * NB: the following code is spagghetti.
			 * Only eat it with lotta tomato ketchup and
			 * Parmesan cheese:-)
			 */
			/*
			 * look whether we will have to steal the keys
			 * and cook them into mouse events
			 */
			if(vsp->num_lock && mouse.opened)
			{
				int button, accel, i;
				enum mouse_dir
				{
					MOUSE_NW, MOUSE_N, MOUSE_NE,
					MOUSE_W,  MOUSE_0, MOUSE_E,
					MOUSE_SW, MOUSE_S, MOUSE_SE
				}
				move;
				struct timeval now;
				/* from sys/kern/kern_time.c */
				extern void timevalsub
					(struct timeval *, struct timeval *);
				dev_t dummy = makedev(0, mouse.minor);
				struct tty *mousetty = get_pccons(dummy);
				/*
				 * strings to send for each mouse event,
				 * indexed by the movement direction and
				 * the "accelerator" value (TRUE for frequent
				 * key presses); note that the first byte
				 * of each string is actually overwritten
				 * by the current button value before sending
				 * the string
				 */
				static u_char mousestrings[2][MOUSE_SE+1][5] =
				{
				{
					/* first, the non-accelerated strings*/
					{0x87,  -1,   1,   0,   0}, /* NW */
					{0x87,   0,   1,   0,   0}, /* N */
					{0x87,   1,   1,   0,   0}, /* NE */
					{0x87,  -1,   0,   0,   0}, /* W */
					{0x87,   0,   0,   0,   0}, /* 0 */
					{0x87,   1,   0,   0,   0}, /* E */
					{0x87,  -1,  -1,   0,   0}, /* SW */
					{0x87,   0,  -1,   0,   0}, /* S */
					{0x87,   1,  -1,   0,   0}  /* SE */
				},
				{
					/* now, 6 steps at once */
					{0x87,  -4,   4,   0,   0}, /* NW */
					{0x87,   0,   6,   0,   0}, /* N */
					{0x87,   4,   4,   0,   0}, /* NE */
					{0x87,  -6,   0,   0,   0}, /* W */
					{0x87,   0,   0,   0,   0}, /* 0 */
					{0x87,   6,   0,   0,   0}, /* E */
					{0x87,  -4,  -4,   0,   0}, /* SW */
					{0x87,   0,  -6,   0,   0}, /* S */
					{0x87,   4,  -4,   0,   0}  /* SE */
				}
				};
				
				if(dt == 0xe0)
				{
					/* ignore extended scan codes */
					mouse.extendedseen = 1;
					goto no_mouse_event;
				}
				if(mouse.extendedseen)
				{
					mouse.extendedseen = 0;
					goto no_mouse_event;
				}
				mouse.extendedseen = 0;

				/*
				 * Note that we cannot use a switch here
				 * since we want to have the keycodes in
				 * a variable
				 */
				if((dt & 0x7f) == mousedef.leftbutton) {
					button = 4;
					goto do_button;
				}
				else if((dt & 0x7f) == mousedef.middlebutton) {
					button = 2;
					goto do_button;
				}
				else if((dt & 0x7f) == mousedef.rightbutton) {
					button = 1;
				do_button:

					/*
					 * i would really like to give
					 * some acustical support
					 * (pling/plong); i am not sure
					 * whether it is safe to call
					 * sysbeep from within an intr
					 * service, since it calls
					 * timeout in turn which mani-
					 * pulates the spl mask - jw
					 */

# define PLING sysbeep(PCVT_SYSBEEPF / 1500, 2)
# define PLONG sysbeep(PCVT_SYSBEEPF / 1200, 2)

					if(mousedef.stickybuttons)
					{
						if(dt & 0x80) {
							mouse.breakseen = 1;
							return (u_char *)0;
						}
						else if(mouse.buttons == button
							&& !mouse.breakseen) {
							/* ignore repeats */
							return (u_char *)0;
						}
						else
							mouse.breakseen = 0;
						if(mouse.buttons == button) {
							/* release it */
							mouse.buttons = 0;
							PLONG;
						} else {
							/*
							 * eventually, release
							 * any other button,
							 * and stick this one
							 */
							mouse.buttons = button;
							PLING;
						}
					}
					else
					{
						if(dt & 0x80) {
							mouse.buttons &=
								~button;
							PLONG;
						}
						else if((mouse.buttons
							& button) == 0) {
							mouse.buttons |=
								button;
							PLING;
						}
						/*else: ignore same btn press*/
					}
					move = MOUSE_0;
					accel = 0;
				}
# undef PLING
# undef PLONG
				else switch(dt & 0x7f)
				{
				/* the arrow keys - KP 1 thru KP 9 */
				case 0x47:	move = MOUSE_NW; goto do_move;
				case 0x48:	move = MOUSE_N; goto do_move;
				case 0x49:	move = MOUSE_NE; goto do_move;
				case 0x4b:	move = MOUSE_W; goto do_move;
				case 0x4c:	move = MOUSE_0; goto do_move;
				case 0x4d:	move = MOUSE_E; goto do_move;
				case 0x4f:	move = MOUSE_SW; goto do_move;
				case 0x50:	move = MOUSE_S; goto do_move;
				case 0x51:	move = MOUSE_SE;
				do_move:
					if(dt & 0x80)
						/*
						 * arrow key break events are
						 * of no importance for us
						 */
						return (u_char *)0;
					/*
					 * see whether the last move did
					 * happen "recently", i.e. before
					 * less than half a second
					 */
					now = time;
					timevalsub(&now, &mouse.lastmove);
					mouse.lastmove = time;
					accel = (now.tv_sec == 0
						 && now.tv_usec
						 < mousedef.acceltime);
					break;
					
				default: /* not a mouse-emulating key */
					goto no_mouse_event;
				}
				mousestrings[accel][move][0] =
					0x80 + (~mouse.buttons & 7);
				/* finally, send the string */
				for(i = 0; i < 5; i++)
					(*linesw[mousetty->t_line].l_rint)
						(mousestrings[accel][move][i],
						 mousetty);
				return (u_char *)0; /* not a kbd event */
			}
no_mouse_event:

#endif /* PCVT_EMU_MOUSE */

			return ((u_char *)keybuf);
		}
	}

#else /* !XSERVER */

	if(inb(CONTROLLER_CTRL) & STATUS_OUTPBF)
		dt = inb(CONTROLLER_DATA);		/* yes, get it ! */

#endif /* !XSERVER */

	else
	{
		if(noblock)
			return NULL;
		else
			goto loop;
	}

#if PCVT_SHOWKEYS
	{
	int rki;
	
	
	for(rki = 3; rki < 80; rki++)		/* shift left buffer */
		rawkeybuf[rki-3] = rawkeybuf[rki];

	rawkeybuf[77] = ' ';		/* delimiter */

	rki = (dt & 0xf0) >> 4;		/* ms nibble */

	if(rki <= 9)
		rki = rki + '0';
	else
		rki = rki - 10 + 'A';

	rawkeybuf[78] = rki;
		
	rki = dt & 0x0f;			/* ls nibble */

	if(rki <= 9)
		rki = rki + '0';
	else
		rki = rki - 10 + 'A';

	rawkeybuf[79] = rki;
	}
#endif	/* PCVT_SHOWKEYS */

	/* lets look what we got */
	switch(dt)
	{
		case KEYB_R_OVERRUN0:	/* keyboard buffer overflow */

#if PCVT_SCANSET == 2
		case KEYB_R_SELFOK:	/* keyboard selftest ok */
#endif /* PCVT_SCANSET == 2 */

		case KEYB_R_ECHO:	/* keyboard response to KEYB_C_ECHO */
		case KEYB_R_ACK:	/* acknowledge after command has rx'd*/
		case KEYB_R_SELFBAD:	/* keyboard selftest FAILED */
		case KEYB_R_DIAGBAD:	/* keyboard self diagnostic failure */
		case KEYB_R_RESEND:	/* keyboard wants us to resend cmnd */
		case KEYB_R_OVERRUN1:	/* keyboard buffer overflow */
			break;

		case KEYB_R_EXT1:	/* keyboard extended scancode pfx 2 */
			kbd_status.ext1 = 1;
			/* FALLTHROUGH */
		case KEYB_R_EXT0:	/* keyboard extended scancode pfx 1 */
			kbd_status.extended = 1; 
			break;

#if PCVT_SCANSET == 2
		case KEYB_R_BREAKPFX:	/* break code prefix for set 2 and 3 */
			kbd_status.breakseen = 1; 
			break;
#endif /* PCVT_SCANSET == 2 */

		default:
			goto regular;	/* regular key */
	}

	if(noblock)
		return NULL;
	else
		goto loop;

	/* got a normal scan key */
regular:

#if PCVT_SCANSET == 1
	kbd_status.breakseen = dt & 0x80 ? 1 : 0;
	dt &= 0x7f;
#endif	/* PCVT_SCANSET == 1 */

	/*   make a keycode from scan code	*/
	if(dt >= sizeof scantokey / sizeof(u_char))
		key = 0;
	else
		key = kbd_status.extended ? extscantokey[dt] : scantokey[dt];
	if(kbd_status.ext1 && key == 58)
		/* virtual control key */
		key = 129;
	kbd_status.extended = kbd_status.ext1 = 0;

#if PCVT_CTRL_ALT_DEL		/*   Check for cntl-alt-del	*/
	if((key == 76) && ctrl_down && (meta_down||altgr_down))
		cpu_reset();
#endif /* PCVT_CTRL_ALT_DEL */
		
#if !PCVT_NETBSD
#include "ddb.h"
#endif /* !PCVT_NETBSD */

#if NDDB > 0 || defined(DDB)		 /*   Check for cntl-alt-esc	*/

  	if((key == 110) && ctrl_down && (meta_down || altgr_down))
 	{
 		static u_char in_Debugger;
 
 		if(!in_Debugger)
 		{
 			in_Debugger = 1;
#if PCVT_FREEBSD
			/* the string is actually not used... */
			Debugger("kbd");
#else
 			Debugger();
#endif
 			in_Debugger = 0;
 			if(noblock)
 				return NULL;
 			else
 				goto loop;
 		}
 	}
#endif /* NDDB > 0 || defined(DDB) */

	/* look for keys with special handling */
	if(key == 128)
	{
		/*
		 * virtual shift; sent around PrtScr, and around the arrow
		 * keys if the NumLck LED is on
		 */
		kbd_status.vshift = !kbd_status.breakseen;
		key = 0;	/* no key */
	}
	else if(key == 129)
	{
		/*
		 * virtual control - the most ugly thingie at all
		 * the Pause key sends:
		 * <virtual control make> <numlock make> <virtual control
		 * break> <numlock break>
		 */
		if(!kbd_status.breakseen)
			kbd_status.vcontrol = 1;
		/* else: let the numlock hook clear this */
		key = 0;	/* no key */
	}
	else if(key == 90)
	{
		/* NumLock, look whether this is rather a Pause */
		if(kbd_status.vcontrol)
			key = 126;
		/*
		 * if this is the final break code of a Pause key,
		 * clear the virtual control status, too
		 */
		if(kbd_status.vcontrol && kbd_status.breakseen)
			kbd_status.vcontrol = 0;
	}
	else if(key == 127)
	{
		/*
		 * a SysRq; some keyboards are brain-dead enough to
		 * repeat the SysRq key make code by sending PrtScr
		 * make codes; other keyboards do not repeat SysRq
		 * at all. We keep track of the SysRq state here.
		 */
		kbd_status.sysrq = !kbd_status.breakseen;
	}
	else if(key == 124)
	{
		/*
		 * PrtScr; look whether this is really PrtScr or rather
		 * a silly repeat of a SysRq key
		 */
		if(kbd_status.sysrq)
			/* ignore the garbage */
			key = 0;
	}

	/* in NOREPEAT MODE ignore the key if it was the same as before */

	if(!kbrepflag && key == kbd_lastkey && !kbd_status.breakseen)
	{
		if(noblock)
			return NULL;
		else
			goto loop;
	}
	
	type = key2ascii[key].type;

	if(type & KBD_OVERLOAD)
		type = ovltbl[key2ascii[key].ovlindex].type;

	type &= KBD_MASK;

	switch(type)
	{
		case KBD_SHFTLOCK:
			if(!kbd_status.breakseen && key != kbd_lastkey)
			{
				vsp->shift_lock ^= 1;
			}
			break;
			
		case KBD_CAPS:
			if(!kbd_status.breakseen && key != kbd_lastkey)
			{
				vsp->caps_lock ^= 1;
				update_led();
			}
			break;
			
		case KBD_SCROLL:
			if(!kbd_status.breakseen && key != kbd_lastkey)
			{
				vsp->scroll_lock ^= 1;
				update_led();

				if(!(vsp->scroll_lock))
				{
					/* someone may be sleeping */
					wakeup((caddr_t)&(vsp->scroll_lock));
				}
			}
			break;
			
		case KBD_SHIFT:
			shift_down = kbd_status.breakseen ? 0 : 1;
			break;

		case KBD_META:
			meta_down = kbd_status.breakseen ? 0 : 0x80;
			break;

		case KBD_ALTGR:
			altgr_down = kbd_status.breakseen ? 0 : 1;
			break;

		case KBD_CTL:
			ctrl_down = kbd_status.breakseen ? 0 : 1;
			break;

		case KBD_NONE:
		default:
			break;			/* deliver a key */
	}

	if(kbd_status.breakseen)
	{
		key |= 0x80;
		kbd_status.breakseen = 0;
		kbd_lastkey = 0; /* -hv- I know this is a bug with */
	}			 /* N-Key-Rollover, but I ignore that */
	else			 /* because avoidance is too complicated */
		kbd_lastkey = key;
	
	cp = xlatkey2ascii(key);	/* have a key */
	if(cp == NULL && !noblock)
		goto loop;
	return cp;
}

/*---------------------------------------------------------------------------*
 *	reflect status of locking keys & set led's
 *---------------------------------------------------------------------------*/
static void
setlockkeys(int snc)
{
	vsp->scroll_lock = snc & 1;
	vsp->num_lock	 = (snc & 2) ? 1 : 0;
	vsp->caps_lock	 = (snc & 4) ? 1 : 0;
	update_led();
}

/*---------------------------------------------------------------------------*
 *	remove a key definition
 *---------------------------------------------------------------------------*/
static int
rmkeydef(int key)
{
	register Ovl_tbl *ref;
	
	if(key==0 || key > MAXKEYNUM)
		return EINVAL;

	if(key2ascii[key].type & KBD_OVERLOAD)
	{
		ref = &ovltbl[key2ascii[key].ovlindex];		
		ref->keynum = 0;
		ref->type = 0;
		ref->unshift[0] =
		ref->shift[0] =
		ref->ctrl[0] =
		ref->altgr[0] = 0;
		key2ascii[key].type &= KBD_MASK;
	}
	return 0;
}	

/*---------------------------------------------------------------------------*
 *	overlay a key
 *---------------------------------------------------------------------------*/
static int
setkeydef(Ovl_tbl *data)
{
	register i;

	if( data->keynum > MAXKEYNUM		 ||
	    (data->type & KBD_MASK) == KBD_BREAK ||
	    (data->type & KBD_MASK) > KBD_SHFTLOCK)
		return EINVAL;

	data->unshift[KBDMAXOVLKEYSIZE] =
	data->shift[KBDMAXOVLKEYSIZE] =
	data->ctrl[KBDMAXOVLKEYSIZE] =
	data->altgr[KBDMAXOVLKEYSIZE] = 0;

	data->subu =
	data->subs =
	data->subc =
	data->suba = KBD_SUBT_STR;		/* just strings .. */
	
	data->type |= KBD_OVERLOAD;		/* mark overloaded */

	/* if key already overloaded, use that slot else find free slot */

	if(key2ascii[data->keynum].type & KBD_OVERLOAD)
	{
		i = key2ascii[data->keynum].ovlindex;
	}
	else
	{
		for(i=0; i<OVLTBL_SIZE; i++)
			if(ovltbl[i].keynum==0)
				break;

		if(i==OVLTBL_SIZE) 
			return ENOSPC;	/* no space, abuse of ENOSPC(!) */
	}

	ovltbl[i] = *data;		/* copy new data string */

	key2ascii[data->keynum].type |= KBD_OVERLOAD; 	/* mark key */
	key2ascii[data->keynum].ovlindex = i;
	
	return 0;
}

/*---------------------------------------------------------------------------*
 *	keyboard ioctl's entry
 *---------------------------------------------------------------------------*/
int
kbdioctl(Dev_t dev, int cmd, caddr_t data, int flag)
{
	int key;

	switch(cmd)
	{
		case KBDRESET:
			doreset();
			ovlinit(1);
			settpmrate(KBD_TPD500|KBD_TPM100);
			setlockkeys(0);
			break;

		case KBDGTPMAT:
			*(int *)data = tpmrate;
			break;

		case KBDSTPMAT:
			settpmrate(*(int *)data);
			break;

		case KBDGREPSW:
			*(int *)data = kbrepflag;
			break;

		case KBDSREPSW:
			kbrepflag = (*(int *)data) & 1;
			break;

		case KBDGLEDS:
			*(int *)data = ledstate;
			break;

		case KBDSLEDS:
			update_led();	/* ??? */
			break;

		case KBDGLOCK:
			*(int *)data = ( (vsp->scroll_lock) |
					 (vsp->num_lock * 2) |
					 (vsp->caps_lock * 4));
			break;

		case KBDSLOCK:
			setlockkeys(*(int *)data);
			break;

		case KBDGCKEY:
			key = ((Ovl_tbl *)data)->keynum;
			return getckeydef(key,(Ovl_tbl *)data);		

		case KBDSCKEY:
			key = ((Ovl_tbl *)data)->keynum;
			return setkeydef((Ovl_tbl *)data);

		case KBDGOKEY:
			key = ((Ovl_tbl *)data)->keynum;
			return getokeydef(key,(Ovl_tbl *)data);

		case KBDRMKEY:
			key = *(int *)data;
			return rmkeydef(key);

		case KBDDEFAULT:
			ovlinit(1);
			break;	

		default:
			/* proceed with vga ioctls */
			return -1;
	}
	return 0;
}

#if PCVT_EMU_MOUSE
/*--------------------------------------------------------------------------*
 *	mouse emulator ioctl
 *--------------------------------------------------------------------------*/
int
mouse_ioctl(Dev_t dev, int cmd, caddr_t data)
{
	struct mousedefs *def = (struct mousedefs *)data;
	
	switch(cmd)
	{
		case KBDMOUSEGET:
			*def = mousedef;
			break;
			
		case KBDMOUSESET:
			mousedef = *def;
			break;
			
		default:
			return -1;
	}
	return 0;
}
#endif	/* PCVT_EMU_MOUSE */

#if PCVT_USL_VT_COMPAT
/*---------------------------------------------------------------------------*
 *	convert ISO-8859 style keycode into IBM 437
 *---------------------------------------------------------------------------*/
static inline u_char
iso2ibm(u_char c)
{
	if(c < 0x80)
		return c;
	return iso2ibm437[c - 0x80];
}

/*---------------------------------------------------------------------------*
 *	build up a USL style keyboard map
 *---------------------------------------------------------------------------*/
void
get_usl_keymap(keymap_t *map)
{
	int i;

	bzero((caddr_t)map, sizeof(keymap_t));

	map->n_keys = 0x59;	/* that many keys we know about */
	
	for(i = 1; i < N_KEYNUMS; i++)
	{
		Ovl_tbl kdef;
		u_char c;
		int j;
		int idx = key2scan1[i];
		
		if(idx == 0 || idx >= map->n_keys)
			continue;

		getckeydef(i, &kdef);
		kdef.type &= KBD_MASK;
		switch(kdef.type)
		{
		case KBD_ASCII:
		case KBD_RETURN:
			map->key[idx].map[0] = iso2ibm(kdef.unshift[0]);
			map->key[idx].map[1] = iso2ibm(kdef.shift[0]);
			map->key[idx].map[2] = map->key[idx].map[3] =
				iso2ibm(kdef.ctrl[0]);
			map->key[idx].map[4] = map->key[idx].map[5] =
				iso2ibm(c = kdef.altgr[0]);
			/*
			 * XXX this is a hack
			 * since we currently do not map strings to AltGr +
			 * shift, we attempt to use the unshifted AltGr
			 * definition here and try to toggle the case
			 * this should at least work for ISO8859 letters,
			 * but also for (e.g.) russian KOI-8 style
			 */
			if((c & 0x7f) >= 0x40)
				map->key[idx].map[5] = iso2ibm(c ^ 0x20);
			break;
			
		case KBD_FUNC:
			/* we are only interested in F1 thru F12 here */
			if(i >= 112 && i <= 123) {
				map->key[idx].map[0] = i - 112 + 27;
				map->key[idx].spcl = 0x80;
			}
			break;

		case KBD_SHIFT:
			c = i == 44? 2 /* lSh */: 3 /* rSh */; goto special;

		case KBD_CAPS:
			c = 4; goto special;

		case KBD_NUM:
			c = 5; goto special;

		case KBD_SCROLL:
			c = 6; goto special;

		case KBD_META:
			c = 7; goto special;

		case KBD_CTL:
			c = 9; goto special;
		special:
			for(j = 0; j < NUM_STATES; j++)
				map->key[idx].map[j] = c;
			map->key[idx].spcl = 0xff;
			break;
			
		default:
			break;
		}
	}
}		

#endif /* PCVT_USL_VT_COMPAT */

/*---------------------------------------------------------------------------*
 *	switch keypad to numeric mode
 *---------------------------------------------------------------------------*/
void
vt_keynum(struct video_state *svsp)
{
	svsp->num_lock = 1;
	update_led();
}

/*---------------------------------------------------------------------------*
 *	switch keypad to application mode
 *---------------------------------------------------------------------------*/
void
vt_keyappl(struct video_state *svsp)
{
	svsp->num_lock = 0;
	update_led();
}

#if PCVT_VT220KEYB

/*---------------------------------------------------------------------------*
 *	function bound to function key 1
 *---------------------------------------------------------------------------*/
static void
fkey1(void)
{
	if(meta_down)
		more_chars = (u_char *)"\033[23~"; /* F11 */
	else
		do_vgapage(0);
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 2
 *---------------------------------------------------------------------------*/
static void
fkey2(void)
{
	if(meta_down)
		more_chars = (u_char *)"\033[24~"; /* F12 */
	else
		do_vgapage(1);
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 3
 *---------------------------------------------------------------------------*/
static void
fkey3(void)
{
	if(meta_down)
		more_chars = (u_char *)"\033[25~"; /* F13 */
	else
		do_vgapage(2);
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 4
 *---------------------------------------------------------------------------*/
static void
fkey4(void)
{
	if(meta_down)
		more_chars = (u_char *)"\033[26~"; /* F14 */
	else
		do_vgapage(3);
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 5
 *---------------------------------------------------------------------------*/
static void
fkey5(void)
{
	if(meta_down)
		more_chars = (u_char *)"\033[28~"; /* Help */
	else
	{
		if((current_video_screen + 1) > totalscreens-1)
			do_vgapage(0);
		else
			do_vgapage(current_video_screen + 1);
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 6
 *---------------------------------------------------------------------------*/
static void
fkey6(void)
{
	if(meta_down)	
		more_chars = (u_char *)"\033[29~"; /* DO */
	else
		more_chars = (u_char *)"\033[17~"; /* F6 */
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 7
 *---------------------------------------------------------------------------*/
static void
fkey7(void)
{
	if(meta_down)
		more_chars = (u_char *)"\033[31~"; /* F17 */
	else
		more_chars = (u_char *)"\033[18~"; /* F7 */
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 8
 *---------------------------------------------------------------------------*/
static void
fkey8(void)
{
	if(meta_down)
		more_chars = (u_char *)"\033[32~"; /* F18 */
	else
		more_chars = (u_char *)"\033[19~"; /* F8 */
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 9
 *---------------------------------------------------------------------------*/
static void
fkey9(void)
{
	if(meta_down)
		more_chars = (u_char *)"\033[33~"; /* F19 */
	else
		more_chars = (u_char *)"\033[20~"; /* F9 */
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 10
 *---------------------------------------------------------------------------*/
static void
fkey10(void)
{
	if(meta_down)
		more_chars = (u_char *)"\033[34~"; /* F20 */
	else
		more_chars = (u_char *)"\033[21~"; /* F10 */
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 11
 *---------------------------------------------------------------------------*/
static void
fkey11(void)
{
	if(meta_down)
		more_chars = (u_char *)"\0x8FP"; /* PF1 */
	else
		more_chars = (u_char *)"\033[23~"; /* F11 */
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 12
 *---------------------------------------------------------------------------*/
static void
fkey12(void)
{
	if(meta_down)
		more_chars = (u_char *)"\0x8FQ"; /* PF2 */
	else
		more_chars = (u_char *)"\033[24~"; /* F12 */
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 1
 *---------------------------------------------------------------------------*/
static void
sfkey1(void)
{
	if(meta_down)
	{
		if(vsp->ukt.length[6])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[6]]);
		else
			more_chars = (u_char *)"\033[23~"; /* F11 */
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 2
 *---------------------------------------------------------------------------*/
static void
sfkey2(void)
{
	if(meta_down)
	{
		if(vsp->ukt.length[7])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[7]]);
		else
			more_chars = (u_char *)"\033[24~"; /* F12 */
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 3
 *---------------------------------------------------------------------------*/
static void
sfkey3(void)
{
	if(meta_down)
	{
		if(vsp->ukt.length[8])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[8]]);
		else
			more_chars = (u_char *)"\033[25~"; /* F13 */
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 4
 *---------------------------------------------------------------------------*/
static void
sfkey4(void)
{
	if(meta_down)
	{
		if(vsp->ukt.length[9])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[9]]);
		else
			more_chars = (u_char *)"\033[26~"; /* F14 */
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 5
 *---------------------------------------------------------------------------*/
static void
sfkey5(void)
{
	if(meta_down)
	{
		if(vsp->ukt.length[11])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[11]]);
		else
			more_chars = (u_char *)"\033[28~"; /* Help */
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 6
 *---------------------------------------------------------------------------*/
static void
sfkey6(void)
{
	if(!meta_down)
	{
		if(vsp->ukt.length[0])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[0]]);
		else
			more_chars = (u_char *)"\033[17~"; /* F6 */
	}
	else if(vsp->ukt.length[12])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[12]]);
	     else
			more_chars = (u_char *)"\033[29~"; /* DO */
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 7
 *---------------------------------------------------------------------------*/
static void
sfkey7(void)
{
	if(!meta_down)
	{
		if(vsp->ukt.length[1])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[1]]);
		else
			more_chars = (u_char *)"\033[18~"; /* F7 */
	}
	else if(vsp->ukt.length[14])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[14]]);
	     else
			more_chars = (u_char *)"\033[31~"; /* F17 */
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 8
 *---------------------------------------------------------------------------*/
static void
sfkey8(void)
{
	if(!meta_down)
	{
		if(vsp->ukt.length[2])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[2]]);
		else
			more_chars = (u_char *)"\033[19~"; /* F8 */
	}
	else if(vsp->ukt.length[14])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[15]]);
	     else
			more_chars = (u_char *)"\033[32~"; /* F18 */
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 9
 *---------------------------------------------------------------------------*/
static void
sfkey9(void)
{
	if(!meta_down)
	{
		if(vsp->ukt.length[3])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[3]]);
		else
			more_chars = (u_char *)"\033[20~"; /* F9 */
	}
	else if(vsp->ukt.length[16])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[16]]);
	     else
			more_chars = (u_char *)"\033[33~"; /* F19 */
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 10
 *---------------------------------------------------------------------------*/
static void
sfkey10(void)
{
	if(!meta_down)
	{
		if(vsp->ukt.length[4])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[4]]);
		else
			more_chars = (u_char *)"\033[21~"; /* F10 */
	}
	else if(vsp->ukt.length[17])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[17]]);
	     else
			more_chars = (u_char *)"\033[34~"; /* F20 */
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 11
 *---------------------------------------------------------------------------*/
static void
sfkey11(void)
{
	if(!meta_down)
	{
		if(vsp->ukt.length[6])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[6]]);
		else
			more_chars = (u_char *)"\033[23~"; /* F11 */
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 12
 *---------------------------------------------------------------------------*/
static void
sfkey12(void)
{
	if(!meta_down)
	{
		if(vsp->ukt.length[7])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[7]]);
		else
			more_chars = (u_char *)"\033[24~"; /* F12 */
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 1
 *---------------------------------------------------------------------------*/
static void
cfkey1(void)
{
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 2
 *---------------------------------------------------------------------------*/
static void
cfkey2(void)
{
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 3
 *---------------------------------------------------------------------------*/
static void
cfkey3(void)
{
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 4
 *---------------------------------------------------------------------------*/
static void
cfkey4(void)
{
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 5
 *---------------------------------------------------------------------------*/
static void
cfkey5(void)
{
	if(vsp->which_fkl == SYS_FKL)
		toggl_bell(vsp);
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 6
 *---------------------------------------------------------------------------*/
static void
cfkey6(void)
{
	if(vsp->which_fkl == SYS_FKL)	
		toggl_sevenbit(vsp);
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 7
 *---------------------------------------------------------------------------*/
static void
cfkey7(void)
{
	if(vsp->which_fkl == SYS_FKL)
		toggl_dspf(vsp);
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 8
 *---------------------------------------------------------------------------*/
static void
cfkey8(void)
{
	if(vsp->which_fkl == SYS_FKL)	
		toggl_awm(vsp);
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 9
 *---------------------------------------------------------------------------*/
static void
cfkey9(void)
{
	if(vsp->labels_on)	/* toggle label display on/off */
	        fkl_off(vsp);
	else
	        fkl_on(vsp);
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 10
 *---------------------------------------------------------------------------*/
static void
cfkey10(void)
{
	if(vsp->labels_on)	/* toggle user/system fkey labels */
	{
		if(vsp->which_fkl == USR_FKL)
			sw_sfkl(vsp);
		else if(vsp->which_fkl == SYS_FKL)
			sw_ufkl(vsp);
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 11
 *---------------------------------------------------------------------------*/
static void
cfkey11(void)
{
	if(vsp->vt_pure_mode == M_PUREVT)
	        set_emulation_mode(vsp, M_HPVT);
	else if(vsp->vt_pure_mode == M_HPVT)
	        set_emulation_mode(vsp, M_PUREVT);
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 12
 *---------------------------------------------------------------------------*/
static void
cfkey12(void)
{
}

#else	/* !PCVT_VT220KEYB, HP-like Keyboard layout */

/*---------------------------------------------------------------------------*
 *	function bound to function key 1
 *---------------------------------------------------------------------------*/
static void
fkey1(void)
{
	if(!meta_down)
	{
		if((vsp->vt_pure_mode == M_HPVT)
		   && (vsp->which_fkl == SYS_FKL))
			toggl_columns(vsp);
		else
			more_chars = (u_char *)"\033[17~";	/* F6 */
	}
	else
	{
		if(vsp->vt_pure_mode == M_PUREVT
		   || (vsp->which_fkl == USR_FKL))
			more_chars = (u_char *)"\033[26~";	/* F14 */
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 2
 *---------------------------------------------------------------------------*/
static void
fkey2(void)
{
	if(!meta_down)
	{
		if((vsp->vt_pure_mode == M_HPVT)
		   && (vsp->which_fkl == SYS_FKL))
			vt_ris(vsp);
		else
			more_chars = (u_char *)"\033[18~";	/* F7 */
	}
	else
	{
		if(vsp->vt_pure_mode == M_PUREVT
		   || (vsp->which_fkl == USR_FKL))
			more_chars = (u_char *)"\033[28~";	/* HELP */
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 3
 *---------------------------------------------------------------------------*/
static void
fkey3(void)
{
	if(!meta_down)
	{
		if((vsp->vt_pure_mode == M_HPVT)
		   && (vsp->which_fkl == SYS_FKL))
			toggl_24l(vsp);
		else
			more_chars = (u_char *)"\033[19~";	/* F8 */
	}
	else
	{
		if(vsp->vt_pure_mode == M_PUREVT
		   || (vsp->which_fkl == USR_FKL))
			more_chars = (u_char *)"\033[29~";	/* DO */
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 4
 *---------------------------------------------------------------------------*/
static void
fkey4(void)
{
	if(!meta_down)
	{

#if PCVT_SHOWKEYS
		if((vsp->vt_pure_mode == M_HPVT)
		   && (vsp->which_fkl == SYS_FKL))
			toggl_kbddbg(vsp);
		else
			more_chars = (u_char *)"\033[20~";	/* F9 */
#else
		if(vsp->vt_pure_mode == M_PUREVT
		   || (vsp->which_fkl == USR_FKL))
			more_chars = (u_char *)"\033[20~";	/* F9 */
#endif /* PCVT_SHOWKEYS */

	}
	else
	{
		if(vsp->vt_pure_mode == M_PUREVT
		   || (vsp->which_fkl == USR_FKL))
			more_chars = (u_char *)"\033[31~";	/* F17 */
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 5
 *---------------------------------------------------------------------------*/
static void
fkey5(void)
{
	if(!meta_down)
	{
		if((vsp->vt_pure_mode == M_HPVT)
		   && (vsp->which_fkl == SYS_FKL))	
			toggl_bell(vsp);
		else
			more_chars = (u_char *)"\033[21~";	/* F10 */
	}
	else
	{
		if(vsp->vt_pure_mode == M_PUREVT
		   || (vsp->which_fkl == USR_FKL))
			more_chars = (u_char *)"\033[32~";	/* F18 */
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 6
 *---------------------------------------------------------------------------*/
static void
fkey6(void)
{
	if(!meta_down)
	{
		if((vsp->vt_pure_mode == M_HPVT)
		   && (vsp->which_fkl == SYS_FKL))	
			toggl_sevenbit(vsp);
		else
			more_chars = (u_char *)"\033[23~";	/* F11 */
	}
	else
	{
		if(vsp->vt_pure_mode == M_PUREVT
		   || (vsp->which_fkl == USR_FKL))
			more_chars = (u_char *)"\033[33~";	/* F19 */
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 7
 *---------------------------------------------------------------------------*/
static void
fkey7(void)
{
	if(!meta_down)
	{
		if((vsp->vt_pure_mode == M_HPVT)
		   && (vsp->which_fkl == SYS_FKL))
			toggl_dspf(vsp);
		else
			more_chars = (u_char *)"\033[24~";	/* F12 */
	}
	else
	{
		if(vsp->vt_pure_mode == M_PUREVT
		   || (vsp->which_fkl == USR_FKL))
			more_chars = (u_char *)"\033[34~";	/* F20 */
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 8
 *---------------------------------------------------------------------------*/
static void
fkey8(void)
{
	if(!meta_down)
	{
		if((vsp->vt_pure_mode == M_HPVT)
		   && (vsp->which_fkl == SYS_FKL))	
			toggl_awm(vsp);
		else
			more_chars = (u_char *)"\033[25~";	/* F13 */
	}
	else
	{
		if(vsp->vt_pure_mode == M_PUREVT
		   || (vsp->which_fkl == USR_FKL))
			more_chars = (u_char *)"\033[35~";	/* F21 ??!! */
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 9
 *---------------------------------------------------------------------------*/
static void
fkey9(void)
{
	if(meta_down)
	{
		if(vsp->vt_pure_mode == M_PUREVT)
			return;
	
		if(vsp->labels_on)	/* toggle label display on/off */
			fkl_off(vsp);
		else
			fkl_on(vsp);
	}
	else
	{
		do_vgapage(0);
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 10
 *---------------------------------------------------------------------------*/
static void
fkey10(void)
{
	if(meta_down)
	{
		if(vsp->vt_pure_mode != M_PUREVT && vsp->labels_on)
		{
			if(vsp->which_fkl == USR_FKL)
				sw_sfkl(vsp);
			else if(vsp->which_fkl == SYS_FKL)
				sw_ufkl(vsp);
		}
	}
	else
	{
		do_vgapage(1);
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 11
 *---------------------------------------------------------------------------*/
static void
fkey11(void)
{
	if(meta_down)
	{
		if(vsp->vt_pure_mode == M_PUREVT)
			set_emulation_mode(vsp, M_HPVT);
		else if(vsp->vt_pure_mode == M_HPVT)
			set_emulation_mode(vsp, M_PUREVT);
	}
	else
	{
		do_vgapage(2);
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to function key 12
 *---------------------------------------------------------------------------*/
static void
fkey12(void)
{
	if(meta_down)
	{
		if(current_video_screen + 1 > totalscreens-1)
			do_vgapage(0);
		else
			do_vgapage(current_video_screen + 1);
	}
	else
	{
		do_vgapage(3);
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 1
 *---------------------------------------------------------------------------*/
static void
sfkey1(void)
{
	if(!meta_down)
	{
		if(vsp->ukt.length[0])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[0]]);
	}
	else
	{
		if(vsp->ukt.length[9])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[9]]);
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 2
 *---------------------------------------------------------------------------*/
static void
sfkey2(void)
{
	if(!meta_down)
	{
		if(vsp->ukt.length[1])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[1]]);
	}
	else
	{
		if(vsp->ukt.length[11])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[11]]);
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 3
 *---------------------------------------------------------------------------*/
static void
sfkey3(void)
{
	if(!meta_down)
	{
		if(vsp->ukt.length[2])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[2]]);
	}
	else
	{
		if(vsp->ukt.length[12])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[12]]);
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 4
 *---------------------------------------------------------------------------*/
static void
sfkey4(void)
{
	if(!meta_down)
	{
		if(vsp->ukt.length[3])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[3]]);
	}
	else
	{
		if(vsp->ukt.length[13])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[13]]);
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 5
 *---------------------------------------------------------------------------*/
static void
sfkey5(void)
{
	if(!meta_down)
	{
		if(vsp->ukt.length[4])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[4]]);
	}
	else
	{
		if(vsp->ukt.length[14])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[14]]);
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 6
 *---------------------------------------------------------------------------*/
static void
sfkey6(void)
{
	if(!meta_down)
	{
		if(vsp->ukt.length[6])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[6]]);
	}
	else
	{
		if(vsp->ukt.length[15])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[15]]);
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 7
 *---------------------------------------------------------------------------*/
static void
sfkey7(void)
{
	if(!meta_down)
	{
		if(vsp->ukt.length[7])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[7]]);
	}
	else
	{
		if(vsp->ukt.length[16])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[16]]);
	}
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 8
 *---------------------------------------------------------------------------*/
static void
sfkey8(void)
{
	if(!meta_down)
	{
		if(vsp->ukt.length[8])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[8]]);
	}
	else
	{
		if(vsp->ukt.length[17])	/* entry available ? */
			more_chars = (u_char *)
				&(vsp->udkbuf[vsp->ukt.first[17]]);
	}
}
/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 9
 *---------------------------------------------------------------------------*/
static void
sfkey9(void)
{
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 10
 *---------------------------------------------------------------------------*/
static void
sfkey10(void)
{
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 11
 *---------------------------------------------------------------------------*/
static void
sfkey11(void)
{
}

/*---------------------------------------------------------------------------*
 *	function bound to SHIFTED function key 12
 *---------------------------------------------------------------------------*/
static void
sfkey12(void)
{
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 1
 *---------------------------------------------------------------------------*/
static void
cfkey1(void)
{
	if(meta_down)
		do_vgapage(0);
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 2
 *---------------------------------------------------------------------------*/
static void
cfkey2(void)
{
	if(meta_down)
		do_vgapage(1);
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 3
 *---------------------------------------------------------------------------*/
static void
cfkey3(void)
{
	if(meta_down)
		do_vgapage(2);
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 4
 *---------------------------------------------------------------------------*/
static void
cfkey4(void)
{
	if(meta_down)
		do_vgapage(3);
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 5
 *---------------------------------------------------------------------------*/
static void
cfkey5(void)
{
	if(meta_down)
		do_vgapage(4);
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 6
 *---------------------------------------------------------------------------*/
static void
cfkey6(void)
{
	if(meta_down)
		do_vgapage(5);
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 7
 *---------------------------------------------------------------------------*/
static void
cfkey7(void)
{
	if(meta_down)
		do_vgapage(6);
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 8
 *---------------------------------------------------------------------------*/
static void
cfkey8(void)
{
	if(meta_down)
		do_vgapage(7);
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 9
 *---------------------------------------------------------------------------*/
static void
cfkey9(void)
{
	if(meta_down)
		do_vgapage(8);
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 10
 *---------------------------------------------------------------------------*/
static void
cfkey10(void)
{
	if(meta_down)
		do_vgapage(9);
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 11
 *---------------------------------------------------------------------------*/
static void
cfkey11(void)
{
	if(meta_down)
		do_vgapage(10);
}

/*---------------------------------------------------------------------------*
 *	function bound to control function key 12
 *---------------------------------------------------------------------------*/
static void
cfkey12(void)
{
	if(meta_down)
		do_vgapage(11);
}

#endif	/* PCVT_VT220KEYB */

#endif	/* NVT > 0 */

/* ------------------------------- EOF -------------------------------------*/
