/*	$NetBSD: vrkiu.c,v 1.1.1.1 1999/09/16 12:23:33 takemura Exp $	*/

/*-
 * Copyright (c) 1999 SASAKI Takesi
 * Copyright (c) 1999 PocketBSD Project. All rights reserved.
 * All rights reserved.
 *
 * This code is a part of the PocketBSD.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#define VRKIUDEBUG

#include <sys/param.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <machine/intr.h>
#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/platid.h>

#include <hpcmips/vr/vr.h>
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vrkiuvar.h>
#include <hpcmips/vr/vrkiureg.h>
#include <hpcmips/vr/icureg.h>

#ifdef VRKIUDEBUG
int vrkiu_debug = 0;
#define DPRINTF(arg) if (vrkiu_debug) printf arg;
#else
#define	DPRINTF(arg)
#endif

static int vrkiumatch __P((struct device *, struct cfdata *, void *));
static void vrkiuattach __P((struct device *, struct device *, void *));

static void vrkiu_write __P((struct vrkiu_softc *, int, unsigned short));
static unsigned short vrkiu_read __P((struct vrkiu_softc *, int));

int vrkiu_intr __P((void *));

static void detect_key __P((struct vrkiu_softc *));
static void process_key __P((struct vrkiu_softc *, int, int));

struct cfattach vrkiu_ca = {
	sizeof(struct vrkiu_softc), vrkiumatch, vrkiuattach
};

static struct vrkiu_softc *the_vrkiu = NULL; /* XXX: kludge!! */

/* XXX: This tranlation table may depend on each machine.
        Should I build it in? */
static char keytrans[] = {
	-1,  28,  25,  52,  21,  48,  44,  57,  /* - enter p . y b z space */
	-1,  53,  24,  51,  20,  47,  30,  -1,  /* - / o , t v a - */
	-1,  -1,  23,  50,  19,  46,  17,  -1,  /* - - i m r c w - */
	13,  -1,  22,  -1,  18,  45,  16,   2,  /* = - u - e x q 1 */
	-1,  -1,  11,  38,  40,  34,  15,  59,  /* - - 0 l ' g tab f1 */
	-1,  39,  10,  49,   6,  33,   3,  37,  /* - ; 9 n 5 f 2 k */
	-1,  27,   9,  36,   5,  32,   7,  -1,  /* - ] 8 j 4 d 6 - */
	12,  26,   8,  35,   4,  41,  31,  -1,  /* - [ 7 h 3 ` s - */
	58,  -1,  -1,  -1,  14,  -1,  66,  61,  /* caps - - - bs - f8 f3 */
	-1,  56,  -1,  -1,  43,  -1,  65,  62,  /* - alt - - \ - f7 f4 */
	-1,  -1,  29,  -1,  68,  -1,  64,  60,  /* - - ctrl - f10 - f6 f2 */
	-1,  -1,  -1,  42,  -1,  67,  63,   1,  /* - - - shift - f9 f5 esc */
};
/* XXX: fill the field of funct. keys, ex. arrow, fnc, nfer... */

#define	SCROLL		0x0001	/* stop output */
#define	NUM		0x0002	/* numeric shift  cursors vs. numeric */
#define	CAPS		0x0004	/* caps shift -- swaps case of letter */
#define	SHIFT		0x0008	/* keyboard shift */
#define	CTL		0x0010	/* control shift  -- allows ctl function */
#define	ASCII		0x0020	/* ascii code for this key */
#define	ALT		0x0080	/* alternate shift -- alternate chars */
#define	FUNC		0x0100	/* function key */
#define	KP		0x0200	/* Keypad keys */
#define	NONE		0x0400	/* no function */

#define	CODE_SIZE	4		/* Use a max of 4 for now... */

typedef struct {
	u_short	type;
	char unshift[CODE_SIZE];
	char shift[CODE_SIZE];
	char ctl[CODE_SIZE];
} Scan_def;

#if !defined(PCCONS_REAL_BS)
#define PCCONS_REAL_BS 1
#endif
#if !defined(CAPS_IS_CONTROL)
#define CAPS_IS_CONTROL 1
#endif
#if !defined(CAPS_ADD_CONTROL)
#define CAPS_ADD_CONTROL 0
#endif

static Scan_def	scan_codes[] = {
	{ NONE,	"",		"",		"" },		/* 0 unused */
	{ ASCII,"\033",		"\033",		"\033" },	/* 1 ESCape */
	{ ASCII,"1",		"!",		"!" },		/* 2 1 */
	{ ASCII,"2",		"@",		"\000" },	/* 3 2 */
	{ ASCII,"3",		"#",		"#" },		/* 4 3 */
	{ ASCII,"4",		"$",		"$" },		/* 5 4 */
	{ ASCII,"5",		"%",		"%" },		/* 6 5 */
	{ ASCII,"6",		"^",		"\036" },	/* 7 6 */
	{ ASCII,"7",		"&",		"&" },		/* 8 7 */
	{ ASCII,"8",		"*",		"\010" },	/* 9 8 */
	{ ASCII,"9",		"(",		"(" },		/* 10 9 */
	{ ASCII,"0",		")",		")" },		/* 11 0 */
	{ ASCII,"-",		"_",		"\037" },	/* 12 - */
	{ ASCII,"=",		"+",		"+" },		/* 13 = */
#ifndef PCCONS_REAL_BS
	{ ASCII,"\177",		"\177",		"\010" },	/* 14 backspace */
#else
	{ ASCII,"\010",		"\010",		"\177" },	/* 14 backspace */
#endif
	{ ASCII,"\t",		"\177\t",	"\t" },		/* 15 tab */
	{ ASCII,"q",		"Q",		"\021" },	/* 16 q */
	{ ASCII,"w",		"W",		"\027" },	/* 17 w */
	{ ASCII,"e",		"E",		"\005" },	/* 18 e */
	{ ASCII,"r",		"R",		"\022" },	/* 19 r */
	{ ASCII,"t",		"T",		"\024" },	/* 20 t */
	{ ASCII,"y",		"Y",		"\031" },	/* 21 y */
	{ ASCII,"u",		"U",		"\025" },	/* 22 u */
	{ ASCII,"i",		"I",		"\011" },	/* 23 i */
	{ ASCII,"o",		"O",		"\017" },	/* 24 o */
	{ ASCII,"p",		"P",		"\020" },	/* 25 p */
	{ ASCII,"[",		"{",		"\033" },	/* 26 [ */
	{ ASCII,"]",		"}",		"\035" },	/* 27 ] */
	{ ASCII,"\r",		"\r",		"\n" },		/* 28 return */
#if CAPS_IS_CONTROL == 1 && CAPS_ADD_CONTROL == 0
	{ CAPS,	"",		"",		"" },		/* 29 caps */
#else
	{ CTL,	"",		"",		"" },		/* 29 control */
#endif
	{ ASCII,"a",		"A",		"\001" },	/* 30 a */
	{ ASCII,"s",		"S",		"\023" },	/* 31 s */
	{ ASCII,"d",		"D",		"\004" },	/* 32 d */
	{ ASCII,"f",		"F",		"\006" },	/* 33 f */
	{ ASCII,"g",		"G",		"\007" },	/* 34 g */
	{ ASCII,"h",		"H",		"\010" },	/* 35 h */
	{ ASCII,"j",		"J",		"\n" },		/* 36 j */
	{ ASCII,"k",		"K",		"\013" },	/* 37 k */
	{ ASCII,"l",		"L",		"\014" },	/* 38 l */
	{ ASCII,";",		":",		";" },		/* 39 ; */
	{ ASCII,"'",		"\"",		"'" },		/* 40 ' */
	{ ASCII,"`",		"~",		"`" },		/* 41 ` */
	{ SHIFT,"",		"",		"" },		/* 42 shift */
	{ ASCII,"\\",		"|",		"\034" },	/* 43 \ */
	{ ASCII,"z",		"Z",		"\032" },	/* 44 z */
	{ ASCII,"x",		"X",		"\030" },	/* 45 x */
	{ ASCII,"c",		"C",		"\003" },	/* 46 c */
	{ ASCII,"v",		"V",		"\026" },	/* 47 v */
	{ ASCII,"b",		"B",		"\002" },	/* 48 b */
	{ ASCII,"n",		"N",		"\016" },	/* 49 n */
	{ ASCII,"m",		"M",		"\r" },		/* 50 m */
	{ ASCII,",",		"<",		"<" },		/* 51 , */
	{ ASCII,".",		">",		">" },		/* 52 . */
	{ ASCII,"/",		"?",		"\037" },	/* 53 / */
	{ SHIFT,"",		"",		"" },		/* 54 shift */
	{ KP,	"*",		"*",		"*" },		/* 55 kp * */
	{ ALT,	"",		"",		"" },		/* 56 alt */
	{ ASCII," ",		" ",		"\000" },	/* 57 space */
#if CAPS_IS_CONTROL == 1 || CAPS_ADD_CONTROL == 1
	{ CTL,	"",		"",		"" },		/* 58 control */
#else
	{ CAPS,	"",		"",		"" },		/* 58 caps */
#endif
	{ FUNC,	"\033[M",	"\033[Y",	"\033[k" },	/* 59 f1 */
	{ FUNC,	"\033[N",	"\033[Z",	"\033[l" },	/* 60 f2 */
	{ FUNC,	"\033[O",	"\033[a",	"\033[m" },	/* 61 f3 */
	{ FUNC,	"\033[P",	"\033[b",	"\033[n" },	/* 62 f4 */
	{ FUNC,	"\033[Q",	"\033[c",	"\033[o" },	/* 63 f5 */
	{ FUNC,	"\033[R",	"\033[d",	"\033[p" },	/* 64 f6 */
	{ FUNC,	"\033[S",	"\033[e",	"\033[q" },	/* 65 f7 */
	{ FUNC,	"\033[T",	"\033[f",	"\033[r" },	/* 66 f8 */
	{ FUNC,	"\033[U",	"\033[g",	"\033[s" },	/* 67 f9 */
	{ FUNC,	"\033[V",	"\033[h",	"\033[t" },	/* 68 f10 */
	{ NUM,	"",		"",		"" },		/* 69 num lock */
	{ SCROLL,"",		"",		"" },		/* 70 scroll lock */
	{ KP,	"7",		"\033[H",	"7" },		/* 71 kp 7 */
	{ KP,	"8",		"\033[A",	"8" },		/* 72 kp 8 */
	{ KP,	"9",		"\033[I",	"9" },		/* 73 kp 9 */
	{ KP,	"-",		"-",		"-" },		/* 74 kp - */
	{ KP,	"4",		"\033[D",	"4" },		/* 75 kp 4 */
	{ KP,	"5",		"\033[E",	"5" },		/* 76 kp 5 */
	{ KP,	"6",		"\033[C",	"6" },		/* 77 kp 6 */
	{ KP,	"+",		"+",		"+" },		/* 78 kp + */
	{ KP,	"1",		"\033[F",	"1" },		/* 79 kp 1 */
	{ KP,	"2",		"\033[B",	"2" },		/* 80 kp 2 */
	{ KP,	"3",		"\033[G",	"3" },		/* 81 kp 3 */
	{ KP,	"0",		"\033[L",	"0" },		/* 82 kp 0 */
	{ KP,	".",		"\177",		"." },		/* 83 kp . */
	{ NONE,	"",		"",		"" },		/* 84 0 */
	{ NONE,	"100",		"",		"" },		/* 85 0 */
	{ NONE,	"101",		"",		"" },		/* 86 0 */
	{ FUNC,	"\033[W",	"\033[i",	"\033[u" },	/* 87 f11 */
	{ FUNC,	"\033[X",	"\033[j",	"\033[v" },	/* 88 f12 */
	{ NONE,	"102",		"",		"" },		/* 89 0 */
	{ NONE,	"103",		"",		"" },		/* 90 0 */
	{ NONE,	"",		"",		"" },		/* 91 0 */
	{ ASCII,"\177",		"\177",		"\010" },	/* 92 del */
	{ NONE,	"",		"",		"" },		/* 93 0 */
	{ NONE,	"",		"",		"" },		/* 94 0 */
	{ NONE,	"",		"",		"" },		/* 95 0 */
	{ NONE,	"",		"",		"" },		/* 96 0 */
	{ NONE,	"",		"",		"" },		/* 97 0 */
	{ NONE,	"",		"",		"" },		/* 98 0 */
	{ NONE,	"",		"",		"" },		/* 99 0 */
	{ NONE,	"",		"",		"" },		/* 100 */
	{ NONE,	"",		"",		"" },		/* 101 */
	{ NONE,	"",		"",		"" },		/* 102 */
	{ NONE,	"",		"",		"" },		/* 103 */
	{ NONE,	"",		"",		"" },		/* 104 */
	{ NONE,	"",		"",		"" },		/* 105 */
	{ NONE,	"",		"",		"" },		/* 106 */
	{ NONE,	"",		"",		"" },		/* 107 */
	{ NONE,	"",		"",		"" },		/* 108 */
	{ NONE,	"",		"",		"" },		/* 109 */
	{ NONE,	"",		"",		"" },		/* 110 */
	{ NONE,	"",		"",		"" },		/* 111 */
	{ NONE,	"",		"",		"" },		/* 112 */
	{ NONE,	"",		"",		"" },		/* 113 */
	{ NONE,	"",		"",		"" },		/* 114 */
	{ NONE,	"",		"",		"" },		/* 115 */
	{ NONE,	"",		"",		"" },		/* 116 */
	{ NONE,	"",		"",		"" },		/* 117 */
	{ NONE,	"",		"",		"" },		/* 118 */
	{ NONE,	"",		"",		"" },		/* 119 */
	{ NONE,	"",		"",		"" },		/* 120 */
	{ NONE,	"",		"",		"" },		/* 121 */
	{ NONE,	"",		"",		"" },		/* 122 */
	{ NONE,	"",		"",		"" },		/* 123 */
	{ NONE,	"",		"",		"" },		/* 124 */
	{ NONE,	"",		"",		"" },		/* 125 */
	{ NONE,	"",		"",		"" },		/* 126 */
	{ NONE,	"",		"",		"" },		/* 127 */
};

/* XXX: Make these queue obsolute, or move these into vrkiu_softc */
static int
vrkiumatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;		/* XXX */
}

static inline void
vrkiu_write(sc, port, val)
	struct vrkiu_softc *sc;
	int port;
	unsigned short val;
{
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, port, val);
}

static inline unsigned short
vrkiu_read(sc, port)
	struct vrkiu_softc *sc;
	int port;
{
	return bus_space_read_2(sc->sc_iot, sc->sc_ioh, port);
}

static void
vrkiuattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct vrkiu_softc *sc = (struct vrkiu_softc *)self;
	struct vrip_attach_args *va = aux;

	bus_space_tag_t iot = va->va_iot;
	bus_space_handle_t ioh;

	if (bus_space_map(iot, va->va_addr, 1, 0, &ioh)) {
		printf(": can't map bus space\n");
		return;
	}

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;

	/* set KIU */
	vrkiu_write(sc, KIURST, 1);   /* reset */
	vrkiu_write(sc, KIUSCANLINE, 0); /* 96keys */
	vrkiu_write(sc, KIUWKS, 0x18a4); /* XXX: scan timing! */
	vrkiu_write(sc, KIUWKI, 450);
	vrkiu_write(sc, KIUSCANREP, 0x8023);
				/* KEYEN | STPREP = 2 | ATSTP | ATSCAN */

	the_vrkiu = sc;

	if (!(sc->sc_handler = 
	      vrip_intr_establish(va->va_vc, va->va_intr, IPL_TTY,
				  vrkiu_intr, sc))) {
		printf (": can't map interrupt line.\n");
		return;
	}
	/* Level2 register setting */
	vrip_intr_setmask2(va->va_vc, sc->sc_handler, KIUINT_KDATRDY, 1);

	printf("\n");
}

int
vrkiu_intr(arg)
	void *arg;
{
        struct vrkiu_softc *sc = arg;
	/* When key scan finisshed, this entry is called. */
	detect_key(sc);
	DPRINTF(("%d", vrkiu_read(sc, KIUINT) & 7));

	vrkiu_write(sc, KIUINT, 0x7); /* Clear all interrupt */

	return 0;
}

static void
detect_key(sc)
	struct vrkiu_softc *sc;
{
	int i, k;

	DPRINTF(("[detect_key():begin read]"));

	for (i = 0; i < KIU_NSCANLINE / 2; i++) {
		k = vrkiu_read(sc, KIUDATP + i * 2) ^ sc->keystat[i];
		sc->keystat[i] = vrkiu_read(sc, KIUDATP + i * 2);
		while (k) {
			int n, m;
			n = ffs(k) - 1;
			if (n < 0) {
				break;
			}
			k ^= 1 << n;
			m = n + i * 16;
			if (keytrans[m] < 0) {
                                printf("vrkiu: Unkown scan code 0x%02x\n", m);
                                continue;
			}
			/* XXX: scanbuf may overflow! */
			process_key(sc, keytrans[m],
				    !((sc->keystat[i] & (1 << n))));
		}
		/* XXX: The order of keys can be a problem.
		   If CTRL and normal key are pushed simultaneously,
		   normal key can be entered in queue first. 
		   Same problem would occur in key break. */
	}
	/* TODO: Enter scancode into the queue as long as the key pressed */

}

static void
process_key(sc, k, brk) 
	struct vrkiu_softc *sc;
	int k, brk;
{
	char *p;
	extern struct tty biconsdev_tty[];

	switch (scan_codes[k].type) {
	case ALT:
		sc->k_alt = brk ? 0 : 1;
		break;
	case SHIFT:
		sc->k_sft = brk ? 0 : 1;
		break;
	case CTL:
		sc->k_ctrl = brk ? 0 : 1;
		break;
	case ASCII:
		if (!brk) {
			if (sc->k_ctrl) {
				p = scan_codes[k].ctl;
			} else if (sc->k_sft) {
				p = scan_codes[k].shift;
			} else {
				p = scan_codes[k].unshift;
			}
			sc->keybuf[sc->keybufhead++] = 
				sc->k_alt ? (*p | 0x80) : *p;
				/* XXX: multi byte key! */
			if (sc->keybufhead >= NKEYBUF) {
				sc->keybufhead = 0;
			}
			(*linesw[0].l_rint)(sc->k_alt ? (*p | 0x80) : *p, 
					    &biconsdev_tty [0]);
		}
		break;
	default:
		/* Ignored */
	}
}

/* called from biconsdev.c */
int
vrkiu_getc()
{
	int ret;

	if (the_vrkiu == NULL) {
		return 0;	/* XXX */
	}

	while (the_vrkiu->keybuftail == the_vrkiu->keybufhead) {
		detect_key(the_vrkiu);
	}
	ret = the_vrkiu->keybuf[the_vrkiu->keybuftail++];
	if (the_vrkiu->keybuftail >= NKEYBUF) {
		the_vrkiu->keybuftail = 0;
	}
	return ret;
}

