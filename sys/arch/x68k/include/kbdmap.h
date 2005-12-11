/*	$NetBSD: kbdmap.h,v 1.2 2005/12/11 12:19:44 christos Exp $	*/

#ifndef _X68K_KBDMAP_H_
#define _X68K_KBDMAP_H_

#define NUL	0
#define SOH	1
#define STX	2
#define ETX	3
#define EOT	4
#define ENQ	5
#define ACK	6
#define	BEL	7
#define BS	8
#define HT	9
#define LF	10
#define VT	11
#define FF	12
#define CR	13
#define SO	14
#define SI	15
#define DLE	16
#define DC1	17
#define DC2	18
#define DC3	19
#define DC4	20
#define NAK	21
#define SYN	22
#define ETB	23
#define CAN	24
#define EM	25
#define SUB	26
#define ESC	27
#define FS	28
#define GS	29
#define RS	30
#define US	31
#define	DEL	127
#define IND	132
#define NEL	133
#define SSA	134
#define ESA	135
#define HTS	136
#define HTJ	137
#define VTS	138
#define PLD	139
#define PLU	140
#define RI	141
#define SS2	142
#define SS3	143
#define DCS	144
#define PU1	145
#define PU2	146
#define STS	147
#define CCH	148
#define MW	149
#define SPA	150
#define EPA	151
#define CSI	155
#define ST	156
#define OSC	157
#define PM	158
#define APC	159


/* a normal x68k keyboard has 0x6d `normal' keys, plus modifier keys that
   don't appear in the keymap */
#define KBD_NUM_KEYS	0x6d

/* size of string table */
#define KBD_STRTAB_SIZE	255

/* for dead keys, index into acctable */
#define	KBD_ACC_GRAVE	0
#define KBD_ACC_ACUTE	1
#define KBD_ACC_CIRC	2
#define KBD_ACC_TILDE	3
#define KBD_ACC_DIER	4
#define KBD_NUM_ACC	5


struct key {
	unsigned char	mode;	/* see possible values below */
	unsigned char	code;
};

#define KBD_MODE_STRING	(1<<0)	/* code is index into strings[] */
#define KBD_MODE_DEAD	(1<<1)	/* acc-index in upper nibble, code = plain acc */
#define KBD_MODE_CAPS	(1<<2)	/* key is capsable. Only used in non-shifted maps */
#define KBD_MODE_KPAD	(1<<3)	/* key is on keypad */
#define KBD_MODE_GRAVE	(KBD_ACC_GRAVE << 4)
#define KBD_MODE_ACUTE	(KBD_ACC_ACUTE << 4)
#define KBD_MODE_CIRC	(KBD_ACC_CIRC  << 4)
#define KBD_MODE_TILDE	(KBD_ACC_TILDE << 4)
#define KBD_MODE_DIER	(KBD_ACC_DIER  << 4)
#define KBD_MODE_ACCENT(m) ((m) >> 4)	/* get accent from mode */
#define KBD_MODE_ACCMASK  (0xf0)

struct kbdmap {
	struct key 	keys[KBD_NUM_KEYS],
		   	shift_keys[KBD_NUM_KEYS],
		   	alt_keys[KBD_NUM_KEYS],
			alt_shift_keys[KBD_NUM_KEYS];
	unsigned char	strings[KBD_STRTAB_SIZE];
};

#endif
