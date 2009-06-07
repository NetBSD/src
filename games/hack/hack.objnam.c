/*	$NetBSD: hack.objnam.c,v 1.9 2009/06/07 20:13:18 dholland Exp $	*/

/*
 * Copyright (c) 1985, Stichting Centrum voor Wiskunde en Informatica,
 * Amsterdam
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Stichting Centrum voor Wiskunde en
 * Informatica, nor the names of its contributors may be used to endorse or
 * promote products derived from this software without specific prior
 * written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982 Jay Fenlason <hack@gnu.org>
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: hack.objnam.c,v 1.9 2009/06/07 20:13:18 dholland Exp $");
#endif				/* not lint */

#include <stdlib.h>
#include "hack.h"
#include "extern.h"
#define Snprintf (void) snprintf
#define Strcat  (void) strcat
#define	Strcpy	(void) strcpy
#define	PREFIX	15

char           *
strprepend(char *s, char *pref)
{
	int             i = strlen(pref);
	if (i > PREFIX) {
		pline("WARNING: prefix too short.");
		return (s);
	}
	s -= i;
	(void) strncpy(s, pref, i);	/* do not copy trailing 0 */
	return (s);
}

char           *
sitoa(int a)
{
	static char     buf[13];
	Snprintf(buf, sizeof(buf), (a < 0) ? "%d" : "+%d", a);
	return (buf);
}

char           *
typename(int otyp)
{
	static char     buf[BUFSZ];
	size_t bufpos;
	struct objclass *ocl = &objects[otyp];
	const char     *an = ocl->oc_name;
	const char     *dn = ocl->oc_descr;
	char           *un = ocl->oc_uname;
	int             nn = ocl->oc_name_known;
	switch (ocl->oc_olet) {
	case POTION_SYM:
		Strcpy(buf, "potion");
		break;
	case SCROLL_SYM:
		Strcpy(buf, "scroll");
		break;
	case WAND_SYM:
		Strcpy(buf, "wand");
		break;
	case RING_SYM:
		Strcpy(buf, "ring");
		break;
	default:
		if (nn) {
			Strcpy(buf, an);
			if (otyp >= TURQUOISE && otyp <= JADE)
				Strcat(buf, " stone");
			if (un) {
				bufpos = strlen(buf);
				Snprintf(buf+bufpos, sizeof(buf)-bufpos,
					" called %s", un);
			}
			if (dn) {
				bufpos = strlen(buf);
				Snprintf(buf+bufpos, sizeof(buf)-bufpos,
					" (%s)", dn);
			}
		} else {
			strlcpy(buf, dn ? dn : an, sizeof(buf));
			if (ocl->oc_olet == GEM_SYM) {
				strlcat(buf, " gem", sizeof(buf));
			}
			if (un) {
				bufpos = strlen(buf);
				Snprintf(buf+bufpos, sizeof(buf)-bufpos,
					" called %s", un);
			}
		}
		return (buf);
	}
	/* here for ring/scroll/potion/wand */
	if (nn) {
		bufpos = strlen(buf);
		Snprintf(buf+bufpos, sizeof(buf)-bufpos, " of %s", an);
	}
	if (un) {
		bufpos = strlen(buf);
		Snprintf(buf+bufpos, sizeof(buf)-bufpos, " called %s", un);
	}
	if (dn) {
		bufpos = strlen(buf);
		Snprintf(buf+bufpos, sizeof(buf)-bufpos, " (%s)", dn);
	}
	return (buf);
}

char           *
xname(struct obj *obj)
{
	static char     bufr[BUFSZ];
	/* caution: doname() and aobjnam() below "know" these sizes */
	char           *buf = &(bufr[PREFIX]);	/* leave room for "17 -3 " */
	size_t          bufmax = sizeof(bufr) - PREFIX;
	int             nn = objects[obj->otyp].oc_name_known;
	const char     *an = objects[obj->otyp].oc_name;
	const char     *dn = objects[obj->otyp].oc_descr;
	char           *un = objects[obj->otyp].oc_uname;
	int             pl = (obj->quan != 1);

	if (!obj->dknown && !Blind)
		obj->dknown = 1;/* %% doesnt belong here */
	switch (obj->olet) {
	case AMULET_SYM:
		Strcpy(buf, (obj->spe < 0 && obj->known)
		       ? "cheap plastic imitation of the " : "");
		Strcat(buf, "Amulet of Yendor");
		break;
	case TOOL_SYM:
		if (!nn) {
			strlcpy(buf, dn, bufmax);
			break;
		}
		strlcpy(buf, an, bufmax);
		break;
	case FOOD_SYM:
		if (obj->otyp == DEAD_HOMUNCULUS && pl) {
			pl = 0;
			Strcpy(buf, "dead homunculi");
			break;
		}
		/* fungis ? */
		/* fall into next case */
	case WEAPON_SYM:
		if (obj->otyp == WORM_TOOTH && pl) {
			pl = 0;
			Strcpy(buf, "worm teeth");
			break;
		}
		if (obj->otyp == CRYSKNIFE && pl) {
			pl = 0;
			Strcpy(buf, "crysknives");
			break;
		}
		/* fall into next case */
	case ARMOR_SYM:
	case CHAIN_SYM:
	case ROCK_SYM:
		strlcpy(buf, an, bufmax);
		break;
	case BALL_SYM:
		Snprintf(buf, bufmax, "%sheavy iron ball",
		  (obj->owt > objects[obj->otyp].oc_weight) ? "very " : "");
		break;
	case POTION_SYM:
		if (nn || un || !obj->dknown) {
			Strcpy(buf, "potion");
			if (pl) {
				pl = 0;
				Strcat(buf, "s");
			}
			if (!obj->dknown)
				break;
			if (un) {
				Strcat(buf, " called ");
				strlcat(buf, un, bufmax);
			} else {
				Strcat(buf, " of ");
				strlcat(buf, an, bufmax);
			}
		} else {
			strlcpy(buf, dn, bufmax);
			strlcat(buf, " potion", bufmax);
		}
		break;
	case SCROLL_SYM:
		Strcpy(buf, "scroll");
		if (pl) {
			pl = 0;
			Strcat(buf, "s");
		}
		if (!obj->dknown)
			break;
		if (nn) {
			Strcat(buf, " of ");
			strlcat(buf, an, bufmax);
		} else if (un) {
			Strcat(buf, " called ");
			strlcat(buf, un, bufmax);
		} else {
			Strcat(buf, " labeled ");
			strlcat(buf, dn, bufmax);
		}
		break;
	case WAND_SYM:
		if (!obj->dknown)
			Snprintf(buf, bufmax, "wand");
		else if (nn)
			Snprintf(buf, bufmax, "wand of %s", an);
		else if (un)
			Snprintf(buf, bufmax, "wand called %s", un);
		else
			Snprintf(buf, bufmax, "%s wand", dn);
		break;
	case RING_SYM:
		if (!obj->dknown)
			Snprintf(buf, bufmax, "ring");
		else if (nn)
			Snprintf(buf, bufmax, "ring of %s", an);
		else if (un)
			Snprintf(buf, bufmax, "ring called %s", un);
		else
			Snprintf(buf, bufmax, "%s ring", dn);
		break;
	case GEM_SYM:
		if (!obj->dknown) {
			Strcpy(buf, "gem");
			break;
		}
		if (!nn) {
			Snprintf(buf, bufmax, "%s gem", dn);
			break;
		}
		strlcpy(buf, an, bufmax);
		if (obj->otyp >= TURQUOISE && obj->otyp <= JADE)
			strlcat(buf, " stone", bufmax);
		break;
	default:
		Snprintf(buf, bufmax, "glorkum %c (0%o) %u %d",
			obj->olet, obj->olet, obj->otyp, obj->spe);
	}
	if (pl) {
		char           *p;

		for (p = buf; *p; p++) {
			if (!strncmp(" of ", p, 4)) {
				/* pieces of, cloves of, lumps of */
				int             c1, c2 = 's';

				do {
					c1 = c2;
					c2 = *p;
					*p++ = c1;
				} while (c1);
				goto nopl;
			}
		}
		p = eos(buf) - 1;
		if (*p == 's' || *p == 'z' || *p == 'x' ||
		    (*p == 'h' && p[-1] == 's')) {
			/* boxes */
			strlcat(buf, "es", bufmax);
		} else if (*p == 'y' && !strchr(vowels, p[-1])) {
			/* rubies, zruties */
			*p = '\0';
			strlcat(buf, "ies", bufmax);
		} else {
			strlcat(buf, "s", bufmax);
		}
	}
nopl:
	if (obj->onamelth) {
		strlcat(buf, " named ", bufmax);
		strlcat(buf, ONAME(obj), bufmax);
	}
	return (buf);
}

char           *
doname(struct obj *obj)
{
	char            prefix[PREFIX];
	char           *bp = xname(obj);
	size_t          bppos, bpmax;

	/* XXX do this better somehow w/o knowing internals of xname() */
	bpmax = BUFSZ - PREFIX;

	if (obj->quan != 1)
		Snprintf(prefix, sizeof(prefix), "%u ", obj->quan);
	else
		Strcpy(prefix, "a ");
	switch (obj->olet) {
	case AMULET_SYM:
		if (strncmp(bp, "cheap ", 6))
			Strcpy(prefix, "the ");
		break;
	case ARMOR_SYM:
		if (obj->owornmask & W_ARMOR)
			strlcat(bp, " (being worn)", bpmax);
		/* fall into next case */
	case WEAPON_SYM:
		if (obj->known) {
			strlcat(prefix, sitoa(obj->spe), sizeof(prefix));
			strlcat(prefix, " ", sizeof(prefix));
		}
		break;
	case WAND_SYM:
		if (obj->known) {
			bppos = strlen(bp);
			Snprintf(bp+bppos, bpmax-bppos, " (%d)", obj->spe);
		}
		break;
	case RING_SYM:
		if (obj->owornmask & W_RINGR)
			strlcat(bp, " (on right hand)", bpmax);
		if (obj->owornmask & W_RINGL)
			strlcat(bp, " (on left hand)", bpmax);
		if (obj->known && (objects[obj->otyp].bits & SPEC)) {
			strlcat(prefix, sitoa(obj->spe), sizeof(prefix));
			strlcat(prefix, " ", sizeof(prefix));
		}
		break;
	}
	if (obj->owornmask & W_WEP)
		strlcat(bp, " (weapon in hand)", bpmax);
	if (obj->unpaid)
		strlcat(bp, " (unpaid)", bpmax);
	if (!strcmp(prefix, "a ") && strchr(vowels, *bp))
		Strcpy(prefix, "an ");
	bp = strprepend(bp, prefix);
	return (bp);
}

/* used only in hack.fight.c (thitu) */
void
setan(const char *str, char *buf, size_t bufmax)
{
	if (strchr(vowels, *str))
		Snprintf(buf, bufmax, "an %s", str);
	else
		Snprintf(buf, bufmax, "a %s", str);
}

char           *
aobjnam(struct obj *otmp, const char *verb)
{
	char           *bp = xname(otmp);
	char            prefix[PREFIX];
	size_t          bpmax;

	/* XXX do this better somehow w/o knowing internals of xname() */
	bpmax = BUFSZ - PREFIX;

	if (otmp->quan != 1) {
		Snprintf(prefix, sizeof(prefix), "%u ", otmp->quan);
		bp = strprepend(bp, prefix);
	}
	if (verb) {
		/* verb is given in plural (i.e., without trailing s) */
		strlcat(bp, " ", bpmax);
		if (otmp->quan != 1)
			strlcat(bp, verb, bpmax);
		else if (!strcmp(verb, "are"))
			strlcat(bp, "is", bpmax);
		else {
			strlcat(bp, verb, bpmax);
			strlcat(bp, "s", bpmax);
		}
	}
	return (bp);
}

char           *
Doname(struct obj *obj)
{
	char           *s = doname(obj);

	if ('a' <= *s && *s <= 'z')
		*s -= ('a' - 'A');
	return (s);
}

const char *const wrp[] = {"wand", "ring", "potion", "scroll", "gem"};
const char wrpsym[] = {WAND_SYM, RING_SYM, POTION_SYM, SCROLL_SYM, GEM_SYM};

struct obj     *
readobjnam(char *bp)
{
	char           *p;
	unsigned        ii;
	int		i;
	int             cnt, spe, spesgn, typ, heavy;
	char            let;
	char           *un, *dn, *an;
	/* int the = 0; char *oname = 0; */
	cnt = spe = spesgn = typ = heavy = 0;
	let = 0;
	an = dn = un = 0;
	for (p = bp; *p; p++)
		if ('A' <= *p && *p <= 'Z')
			*p += 'a' - 'A';
	if (!strncmp(bp, "the ", 4)) {
		/* the = 1; */
		bp += 4;
	} else if (!strncmp(bp, "an ", 3)) {
		cnt = 1;
		bp += 3;
	} else if (!strncmp(bp, "a ", 2)) {
		cnt = 1;
		bp += 2;
	}
	if (!cnt && digit(*bp)) {
		cnt = atoi(bp);
		while (digit(*bp))
			bp++;
		while (*bp == ' ')
			bp++;
	}
	if (!cnt)
		cnt = 1;	/* %% what with "gems" etc. ? */

	if (*bp == '+' || *bp == '-') {
		spesgn = (*bp++ == '+') ? 1 : -1;
		spe = atoi(bp);
		while (digit(*bp))
			bp++;
		while (*bp == ' ')
			bp++;
	} else {
		p = strrchr(bp, '(');
		if (p) {
			if (p > bp && p[-1] == ' ')
				p[-1] = 0;
			else
				*p = 0;
			p++;
			spe = atoi(p);
			while (digit(*p))
				p++;
			if (strcmp(p, ")"))
				spe = 0;
			else
				spesgn = 1;
		}
	}
	/*
	 * now we have the actual name, as delivered by xname, say green
	 * potions called whisky scrolls labeled "QWERTY" egg dead zruties
	 * fortune cookies very heavy iron ball named hoei wand of wishing
	 * elven cloak
	 */
	for (p = bp; *p; p++)
		if (!strncmp(p, " named ", 7)) {
			*p = 0;
			/* oname = p+7; */
		}
	for (p = bp; *p; p++)
		if (!strncmp(p, " called ", 8)) {
			*p = 0;
			un = p + 8;
		}
	for (p = bp; *p; p++)
		if (!strncmp(p, " labeled ", 9)) {
			*p = 0;
			dn = p + 9;
		}
	/* first change to singular if necessary */
	if (cnt != 1) {
		/* find "cloves of garlic", "worthless pieces of blue glass" */
		for (p = bp; *p; p++)
			if (!strncmp(p, "s of ", 5)) {
				while ((*p = p[1]) != '\0')
					p++;
				goto sing;
			}
		/* remove -s or -es (boxes) or -ies (rubies, zruties) */
		p = eos(bp);
		if (p[-1] == 's') {
			if (p[-2] == 'e') {
				if (p[-3] == 'i') {
					if (!strcmp(p - 7, "cookies"))
						goto mins;
					Strcpy(p - 3, "y");
					goto sing;
				}
				/* note: cloves / knives from clove / knife */
				if (!strcmp(p - 6, "knives")) {
					Strcpy(p - 3, "fe");
					goto sing;
				}
				/* note: nurses, axes but boxes */
				if (!strcmp(p - 5, "boxes")) {
					p[-2] = 0;
					goto sing;
				}
			}
	mins:
			p[-1] = 0;
		} else {
			if (!strcmp(p - 9, "homunculi")) {
				Strcpy(p - 1, "us");	/* !! makes string
							 * longer */
				goto sing;
			}
			if (!strcmp(p - 5, "teeth")) {
				Strcpy(p - 5, "tooth");
				goto sing;
			}
			/* here we cannot find the plural suffix */
		}
	}
sing:
	if (!strcmp(bp, "amulet of yendor")) {
		typ = AMULET_OF_YENDOR;
		goto typfnd;
	}
	p = eos(bp);
	if (!strcmp(p - 5, " mail")) {	/* Note: ring mail is not a ring ! */
		let = ARMOR_SYM;
		an = bp;
		goto srch;
	}
	for (ii = 0; ii < sizeof(wrpsym); ii++) {
		int             j = strlen(wrp[ii]);
		if (!strncmp(bp, wrp[ii], j)) {
			let = wrpsym[ii];
			bp += j;
			if (!strncmp(bp, " of ", 4))
				an = bp + 4;
			/* else if(*bp) ?? */
			goto srch;
		}
		if (!strcmp(p - j, wrp[ii])) {
			let = wrpsym[ii];
			p -= j;
			*p = 0;
			if (p[-1] == ' ')
				p[-1] = 0;
			dn = bp;
			goto srch;
		}
	}
	if (!strcmp(p - 6, " stone")) {
		p[-6] = 0;
		let = GEM_SYM;
		an = bp;
		goto srch;
	}
	if (!strcmp(bp, "very heavy iron ball")) {
		heavy = 1;
		typ = HEAVY_IRON_BALL;
		goto typfnd;
	}
	an = bp;
srch:
	if (!an && !dn && !un)
		goto any;
	i = 1;
	if (let)
		i = bases[letindex(let)];
	while (i <= NROFOBJECTS && (!let || objects[i].oc_olet == let)) {
		const char           *zn = objects[i].oc_name;

		if (!zn)
			goto nxti;
		if (an && strcmp(an, zn))
			goto nxti;
		if (dn && (!(zn = objects[i].oc_descr) || strcmp(dn, zn)))
			goto nxti;
		if (un && (!(zn = objects[i].oc_uname) || strcmp(un, zn)))
			goto nxti;
		typ = i;
		goto typfnd;
nxti:
		i++;
	}
any:
	if (!let)
		let = wrpsym[rn2(sizeof(wrpsym))];
	typ = probtype(let);
typfnd:
	{
		struct obj     *otmp;
		let = objects[typ].oc_olet;
		otmp = mksobj(typ);
		if (heavy)
			otmp->owt += 15;
		if (cnt > 0 && strchr("%?!*)", let) &&
		(cnt < 4 || (let == WEAPON_SYM && typ <= ROCK && cnt < 20)))
			otmp->quan = cnt;

		if (spe > 3 && spe > otmp->spe)
			spe = 0;
		else if (let == WAND_SYM)
			spe = otmp->spe;
		if (spe == 3 && u.uluck < 0)
			spesgn = -1;
		if (let != WAND_SYM && spesgn == -1)
			spe = -spe;
		if (let == BALL_SYM)
			spe = 0;
		else if (let == AMULET_SYM)
			spe = -1;
		else if (typ == WAN_WISHING && rn2(10))
			spe = (rn2(10) ? -1 : 0);
		otmp->spe = spe;

		if (spesgn == -1)
			otmp->cursed = 1;

		return (otmp);
	}
}
