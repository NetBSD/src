/*
 * Copyright (c) 2016, 2018, 2023 Kristaps Dzonsons <kristaps@bsd.lv>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHORS DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "config.h"

#if HAVE_SYS_QUEUE
# include <sys/queue.h>
#endif

#include <assert.h>
#include <ctype.h>
#if HAVE_ERR
# include <err.h>
#endif
#include <getopt.h>
#if HAVE_SANDBOX_INIT
# include <sandbox.h>
#endif
#include <search.h>
#include <stdint.h> /* uintptr_t */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Phase of parsing input file.
 */
enum	phase {
	PHASE_INIT = 0, /* waiting to encounter definition */
	PHASE_KEYS, /* have definition, now keywords */
	PHASE_DESC, /* have keywords, now description */
	PHASE_SEEALSO,
	PHASE_DECL /* have description, now declarations */
};

/*
 * What kind of declaration (preliminary analysis).
 */
enum	decltype {
	DECLTYPE_CPP, /* pre-processor */
	DECLTYPE_C, /* semicolon-closed non-preprocessor */
	DECLTYPE_NEITHER /* non-preprocessor, no semicolon */
};

/*
 * In variables and function declarations, we toss these.
 */
enum	preproc {
	PREPROC_SQLITE_API,
	PREPROC_SQLITE_DEPRECATED,
	PREPROC_SQLITE_EXPERIMENTAL,
	PREPROC_SQLITE_EXTERN,
	PREPROC_SQLITE_STDCALL,
	PREPROC__MAX
};

/*
 * HTML tags that we recognise.
 */
enum	tag {
	TAG_A_CLOSE,
	TAG_A_OPEN_ATTRS,
	TAG_B_CLOSE,
	TAG_B_OPEN,
	TAG_BLOCK_CLOSE,
	TAG_BLOCK_OPEN,
	TAG_BR_OPEN,
	TAG_DD_CLOSE,
	TAG_DD_OPEN,
	TAG_DL_CLOSE,
	TAG_DL_OPEN,
	TAG_DT_CLOSE,
	TAG_DT_OPEN,
	TAG_EM_CLOSE,
	TAG_EM_OPEN,
	TAG_H3_CLOSE,
	TAG_H3_OPEN,
	TAG_I_CLOSE,
	TAG_I_OPEN,
	TAG_LI_CLOSE,
	TAG_LI_OPEN,
	TAG_LI_OPEN_ATTRS,
	TAG_OL_CLOSE,
	TAG_OL_OPEN,
	TAG_P_OPEN,
	TAG_PRE_CLOSE,
	TAG_PRE_OPEN,
	TAG_SPAN_CLOSE,
	TAG_SPAN_OPEN_ATTRS,
	TAG_TABLE_CLOSE,
	TAG_TABLE_OPEN,
	TAG_TABLE_OPEN_ATTRS,
	TAG_TD_CLOSE,
	TAG_TD_OPEN,
	TAG_TD_OPEN_ATTRS,
	TAG_TH_CLOSE,
	TAG_TH_OPEN,
	TAG_TH_OPEN_ATTRS,
	TAG_TR_CLOSE,
	TAG_TR_OPEN,
	TAG_U_CLOSE,
	TAG_U_OPEN,
	TAG_UL_CLOSE,
	TAG_UL_OPEN,
	TAG__MAX
};

TAILQ_HEAD(defnq, defn);
TAILQ_HEAD(declq, decl);

/*
 * A declaration of type DECLTYPE_CPP or DECLTYPE_C.
 * These need not be unique (if ifdef'd).
 */
struct	decl {
	enum decltype	 type; /* type of declaration */
	char		*text; /* text */
	size_t		 textsz; /* strlen(text) */
	TAILQ_ENTRY(decl) entries;
};

/*
 * A definition is basically the manpage contents.
 */
struct	defn {
	char		 *name; /* really Nd */
	TAILQ_ENTRY(defn) entries;
	char		 *desc; /* long description */
	size_t		  descsz; /* strlen(desc) */
	char		 *fulldesc; /* description w/newlns */
	size_t		  fulldescsz; /* strlen(fulldesc) */
	struct declq	  dcqhead; /* declarations */
	int		  multiline; /* used when parsing */
	int		  instruct; /* used when parsing */
	const char	 *fn; /* parsed from file */
	size_t		  ln; /* parsed at line */
	int		  postprocessed; /* good for emission? */
	char		 *dt; /* manpage title */
	char		**nms; /* manpage names */
	size_t		  nmsz; /* number of names */
	char		 *fname; /* manpage filename */
	char		 *keybuf; /* raw keywords */
	size_t		  keybufsz; /* length of "keysbuf" */
	char		 *seealso; /* see also tags */
	size_t		  seealsosz; /* length of seealso */
	char		**xrs; /* parsed "see also" references */
	size_t		  xrsz; /* number of references */
	char		**keys; /* parsed keywords */
	size_t		  keysz; /* number of keywords */
};

/*
 * Entire parse routine.
 */
struct	parse {
	enum phase	 phase; /* phase of parse */
	size_t		 ln; /* line number */
	const char	*fn; /* open file */
	struct defnq	 dqhead; /* definitions */
};

/*
 * How to handle HTML tags we find in the text.
 */
struct	taginfo {
	const char	*html; /* HTML to key on */
	const char	*mdoc; /* generate mdoc(7) */
	unsigned int	 flags;
#define	TAGINFO_NOBR	 0x01 /* follow w/space, not newline */
#define	TAGINFO_NOOP	 0x02 /* just strip out */
#define	TAGINFO_NOSP	 0x04 /* follow w/o space or newline */
#define	TAGINFO_INLINE	 0x08 /* inline block (notused) */
#define TAGINFO_ATTRS	 0x10 /* ignore attributes */
};

static	const struct taginfo tags[TAG__MAX] = {
	{ "</a>", "", TAGINFO_INLINE }, /* TAG_A_CLOSE */
	{ "<a ", "", TAGINFO_INLINE | TAGINFO_ATTRS }, /* TAG_A_OPEN_ATTRS */
	{ "</b>", "\\fP", TAGINFO_INLINE }, /* TAG_B_CLOSE */
	{ "<b>", "\\fB", TAGINFO_INLINE }, /* TAG_B_OPEN */
	{ "<br>", " ", TAGINFO_INLINE }, /* TAG_BR_OPEN */
	{ "</blockquote>", ".Ed\n.Pp", 0 }, /* TAG_BLOCK_CLOSE */
	{ "<blockquote>", ".Bd -ragged", 0 }, /* TAG_BLOCK_OPEN */
	{ "</dd>", "", TAGINFO_NOOP }, /* TAG_DD_CLOSE */
	{ "<dd>", "", TAGINFO_NOBR | TAGINFO_NOSP }, /* TAG_DD_OPEN */
	{ "</dl>", ".El\n.Pp", 0 }, /* TAG_DL_CLOSE */
	{ "<dl>", ".Bl -tag -width Ds", 0 }, /* TAG_DL_OPEN */
	{ "</dt>", "", TAGINFO_NOBR | TAGINFO_NOSP}, /* TAG_DT_CLOSE */
	{ "<dt>", ".It", TAGINFO_NOBR }, /* TAG_DT_OPEN */
	{ "</em>", "\\fP", TAGINFO_INLINE }, /* TAG_EM_CLOSE */
	{ "<em>", "\\fB", TAGINFO_INLINE }, /* TAG_EM_OPEN */
	{ "</h3>", "", TAGINFO_NOBR | TAGINFO_NOSP}, /* TAG_H3_CLOSE */
	{ "<h3>", ".Ss", TAGINFO_NOBR }, /* TAG_H3_OPEN */
	{ "</i>", "\\fP", TAGINFO_INLINE }, /* TAG_I_CLOSE */
	{ "<i>", "\\fI", TAGINFO_INLINE }, /* TAG_I_OPEN */
	{ "</li>", "", TAGINFO_NOOP }, /* TAG_LI_CLOSE */
	{ "<li>", ".It", 0 }, /* TAG_LI_OPEN */
	{ "<li ", ".It", TAGINFO_ATTRS }, /* TAG_LI_OPEN_ATTRS */
	{ "</ol>", ".El\n.Pp", 0 }, /* TAG_OL_CLOSE */
	{ "<ol>", ".Bl -enum", 0 }, /* TAG_OL_OPEN */
	{ "<p>", ".Pp", 0 }, /* TAG_P_OPEN */
	{ "</pre>", ".Ed\n.Pp", 0 }, /* TAG_PRE_CLOSE */
	{ "<pre>", ".Bd -literal", 0 }, /* TAG_PRE_OPEN */
	{ "</span>", "", TAGINFO_INLINE }, /* TAG_SPAN_CLOSE */
	{ "<span ", "", TAGINFO_INLINE | TAGINFO_ATTRS }, /* TAG_SPAN_OPEN_ATTRS */
	{ "</table>", ".Pp", 0 }, /* TAG_TABLE_CLOSE */
	{ "<table>", ".Pp", 0 }, /* TAG_TABLE_OPEN */
	{ "<table ", ".Pp", TAGINFO_ATTRS }, /* TAG_TABLE_OPEN_ATTRS */
	{ "</td>", "", TAGINFO_NOOP }, /* TAG_TD_CLOSE */
	{ "<td>", " ", TAGINFO_INLINE }, /* TAG_TD_OPEN */
	{ "<td ", " ", TAGINFO_INLINE | TAGINFO_ATTRS}, /* TAG_TD_OPEN_ATTRS */
	{ "</th>", "", TAGINFO_NOOP }, /* TAG_TH_CLOSE */
	{ "<th>", " ", TAGINFO_INLINE }, /* TAG_TH_OPEN */
	{ "<th ", " ", TAGINFO_INLINE | TAGINFO_ATTRS}, /* TAG_TH_OPEN_ATTRS */
	{ "</tr>", "", TAGINFO_NOOP}, /* TAG_TR_CLOSE */
	{ "<tr>", "", TAGINFO_NOBR }, /* TAG_TR_OPEN */
	{ "</u>", "\\fP", TAGINFO_INLINE }, /* TAG_U_CLOSE */
	{ "<u>", "\\fI", TAGINFO_INLINE }, /* TAG_U_OPEN */
	{ "</ul>", ".El\n.Pp", 0 }, /* TAG_UL_CLOSE */
	{ "<ul>", ".Bl -bullet", 0 }, /* TAG_UL_OPEN */
};

static	const char *const preprocs[TAG__MAX] = {
	"SQLITE_API", /* PREPROC_SQLITE_API */
	"SQLITE_DEPRECATED", /* PREPROC_SQLITE_DEPRECATED */
	"SQLITE_EXPERIMENTAL", /* PREPROC_SQLITE_EXPERIMENTAL */
	"SQLITE_EXTERN", /* PREPROC_SQLITE_EXTERN */
	"SQLITE_STDCALL", /* PREPROC_SQLITE_STDCALL */
};

/* Verbose reporting. */
static	int verbose;

/* Don't output any files: use stdout. */
static	int nofile;

/* Print out only filename. */
static	int filename;

static void
decl_function_add(struct parse *p, char **etext,
	size_t *etextsz, const char *cp, size_t len)
{

	if ((*etext)[*etextsz - 1] != ' ') {
		*etext = realloc(*etext, *etextsz + 2);
		if (*etext == NULL)
			err(1, NULL);
		(*etextsz)++;
		strlcat(*etext, " ", *etextsz + 1);
	}
	*etext = realloc(*etext, *etextsz + len + 1);
	if (*etext == NULL)
		err(1, NULL);
	memcpy(*etext + *etextsz, cp, len);
	*etextsz += len;
	(*etext)[*etextsz] = '\0';
}

static void
decl_function_copy(struct parse *p, char **etext,
	size_t *etextsz, const char *cp, size_t len)
{

	*etext = malloc(len + 1);
	if (*etext == NULL)
		err(1, NULL);
	memcpy(*etext, cp, len);
	*etextsz = len;
	(*etext)[*etextsz] = '\0';
}

/*
 * A C function (or variable, or whatever).
 * This is more specifically any non-preprocessor text.
 */
static int
decl_function(struct parse *p, const char *cp, size_t len)
{
	char		*ep, *lcp, *rcp;
	const char	*ncp;
	size_t		 nlen;
	struct defn	*d;
	struct decl	*e;

	/* Fetch current interface definition. */
	d = TAILQ_LAST(&p->dqhead, defnq);
	assert(NULL != d);

	/*
	 * Since C tokens are semicolon-separated, we may be invoked any
	 * number of times per a single line.
	 */
again:
	while (isspace((unsigned char)*cp)) {
		cp++;
		len--;
	}
	if (*cp == '\0')
		return(1);

	/* Whether we're a continuation clause. */
	if (d->multiline) {
		/* This might be NULL if we're not a continuation. */
		e = TAILQ_LAST(&d->dcqhead, declq);
		assert(DECLTYPE_C == e->type);
		assert(NULL != e);
		assert(NULL != e->text);
		assert(e->textsz);
	} else {
		assert(d->instruct == 0);
		e = calloc(1, sizeof(struct decl));
		if (e == NULL)
			err(1, NULL);
		e->type = DECLTYPE_C;
		TAILQ_INSERT_TAIL(&d->dcqhead, e, entries);
	}

	/*
	 * We begin by seeing if there's a semicolon on this line.
	 * If there is, we'll need to do some special handling.
	 */
	ep = strchr(cp, ';');
	lcp = strchr(cp, '{');
	rcp = strchr(cp, '}');

	/* We're only a partial statement (i.e., no closure). */
	if (ep == NULL && d->multiline) {
		assert(e->text != NULL);
		assert(e->textsz > 0);
		/* Is a struct starting or ending here? */
		if (d->instruct && NULL != rcp)
			d->instruct--;
		else if (NULL != lcp)
			d->instruct++;
		decl_function_add(p, &e->text, &e->textsz, cp, len);
		return(1);
	} else if (ep == NULL && !d->multiline) {
		d->multiline = 1;
		/* Is a structure starting in this line? */
		if (NULL != lcp &&
		    (rcp == NULL || rcp < lcp))
			d->instruct++;
		decl_function_copy(p, &e->text, &e->textsz, cp, len);
		return(1);
	}

	/* Position ourselves after the semicolon. */
	assert(NULL != ep);
	ncp = cp;
	nlen = (ep - cp) + 1;
	cp = ep + 1;
	len -= nlen;

	if (d->multiline) {
		assert(NULL != e->text);
		/* Don't stop the multi-line if we're in a struct. */
		if (d->instruct == 0) {
			if (lcp == NULL || lcp > cp)
				d->multiline = 0;
		} else if (NULL != rcp && rcp < cp)
			if (--d->instruct == 0)
				d->multiline = 0;
		decl_function_add(p, &e->text, &e->textsz, ncp, nlen);
	} else {
		assert(e->text == NULL);
		if (NULL != lcp && lcp < cp) {
			d->multiline = 1;
			d->instruct++;
		}
		decl_function_copy(p, &e->text, &e->textsz, ncp, nlen);
	}

	goto again;
}

/*
 * A definition is just #define followed by space followed by the name,
 * then the value of that name.
 * We ignore the latter.
 * FIXME: this does not understand multi-line CPP, but I don't think
 * there are any instances of that in sqlite3.h.
 */
static int
decl_define(struct parse *p, const char *cp, size_t len)
{
	struct defn	*d;
	struct decl	*e;
	size_t		 sz;

	while (isspace((unsigned char)*cp)) {
		cp++;
		len--;
	}
	if (len == 0) {
		warnx("%s:%zu: empty pre-processor "
			"constant", p->fn, p->ln);
		return(1);
	}

	d = TAILQ_LAST(&p->dqhead, defnq);
	assert(NULL != d);

	/*
	 * We're parsing a preprocessor definition, but we're still
	 * waiting on a semicolon from a function definition.
	 * It might be a comment or an error.
	 */
	if (d->multiline) {
		if (verbose)
			warnx("%s:%zu: multiline declaration "
				"still open", p->fn, p->ln);
		e = TAILQ_LAST(&d->dcqhead, declq);
		assert(NULL != e);
		e->type = DECLTYPE_NEITHER;
		d->multiline = d->instruct = 0;
	}

	sz = 0;
	while (cp[sz] != '\0' && !isspace((unsigned char)cp[sz]))
		sz++;

	e = calloc(1, sizeof(struct decl));
	if (e == NULL)
		err(1, NULL);
	e->type = DECLTYPE_CPP;
	e->text = calloc(1, sz + 1);
	if (e->text == NULL)
		err(1, NULL);
	strlcpy(e->text, cp, sz + 1);
	e->textsz = sz;
	TAILQ_INSERT_TAIL(&d->dcqhead, e, entries);
	return(1);
}

/*
 * A declaration is a function, variable, preprocessor definition, or
 * really anything else until we reach a blank line.
 */
static void
decl(struct parse *p, const char *cp, size_t len)
{
	struct defn	*d;
	struct decl	*e;
	const char	*oldcp;
	size_t		 oldlen;

	oldcp = cp;
	oldlen = len;

	while (isspace((unsigned char)*cp)) {
		cp++;
		len--;
	}

	d = TAILQ_LAST(&p->dqhead, defnq);
	assert(NULL != d);

	/* Check closure. */
	if (*cp == '\0') {
		p->phase = PHASE_INIT;
		/* Check multiline status. */
		if (d->multiline) {
			if (verbose)
				warnx("%s:%zu: multiline declaration "
					"still open", p->fn, p->ln);
			e = TAILQ_LAST(&d->dcqhead, declq);
			assert(NULL != e);
			e->type = DECLTYPE_NEITHER;
			d->multiline = d->instruct = 0;
		}
		return;
	}

	d->fulldesc = realloc(d->fulldesc,
		d->fulldescsz + oldlen + 2);
	if (d->fulldesc == NULL)
		err(1, NULL);
	if (d->fulldescsz == 0)
		d->fulldesc[0] = '\0';
	d->fulldescsz += oldlen + 2;
	strlcat(d->fulldesc, oldcp, d->fulldescsz);
	strlcat(d->fulldesc, "\n", d->fulldescsz);
	
	/*
	 * Catch preprocessor defines, but discard all other types of
	 * preprocessor statements.
	 * We might already be in the middle of a declaration (a
	 * function declaration), but that's ok.
	 */

	if (*cp == '#') {
		len--;
		cp++;
		while (isspace((unsigned char)*cp)) {
			len--;
			cp++;
		}
		if (strncmp(cp, "define", 6) == 0)
			decl_define(p, cp + 6, len - 6);
		return;
	}

	/* Skip one-liner comments. */

	if (len > 4 &&
	    cp[0] == '/' && cp[1] == '*' &&
	    cp[len - 2] == '*' && cp[len - 1] == '/')
		return;

	decl_function(p, cp, len);
}

/*
 * Whether to end an interface description phase with an asterisk-slash.
 * This is run within a phase already opened with slash-asterisk.  It
 * adjusts the parse state on ending a phase or syntax errors.  It has
 * various hacks around lacks syntax (e.g., starting single-asterisk
 * instead of double-asterisk) found in the wild.
 *
 * Returns zero if not ending the phase, non-zero if ending.
 */
static int
endphase(struct parse *p, const char *cp)
{

	if (*cp == '\0') {
		/*
		 * Error: empty line.
		 */
		warnx("%s:%zu: warn: unexpected empty line in "
			"interface description", p->fn, p->ln);
		p->phase = PHASE_INIT;
		return 1;
	} else if (strcmp(cp, "*/") == 0) {
		/*
		 * End of the interface description.
		 */
		p->phase = PHASE_DECL;
		return 1;
	} else if (!(cp[0] == '*' && cp[1] == '*')) {
		/*
		 * Error: bad syntax, not end or continuation.
		 */
		if (cp[0] == '*' && cp[1] == '\0') {
			if (verbose)
				warnx("%s:%zu: warn: ignoring "
					"standalone asterisk "
					"in interface description",
					p->fn, p->ln);
			return 0;
		} else if (cp[0] == '*' && cp[1] == ' ') {
			if (verbose)
				warnx("%s:%zu: warn: ignoring "
					"leading single asterisk "
					"in interface description",
					p->fn, p->ln);
			return 0;
		}
		warnx("%s:%zu: warn: ambiguous leading characters in "
			"interface description", p->fn, p->ln);
		p->phase = PHASE_INIT;
		return 1;
	}

	/* If here, at a continuation ('**'). */

	return 0;
}

/*
 * Parse a "SEE ALSO" phase, which can come at any point in the
 * interface description (unlike what they claim).
 */
static void
seealso(struct parse *p, const char *cp, size_t len)
{
	struct defn	*d;

	if (endphase(p, cp) || len < 2)
		return;

	cp += 2;
	len -= 2;

	while (isspace((unsigned char)*cp)) {
		cp++;
		len--;
	}

	/* Blank line: back to description part. */
	if (len == 0) {
		p->phase = PHASE_DESC;
		return;
	}

	/* Fetch current interface definition. */
	d = TAILQ_LAST(&p->dqhead, defnq);
	assert(NULL != d);

	d->seealso = realloc(d->seealso,
		d->seealsosz + len + 1);
	memcpy(d->seealso + d->seealsosz, cp, len);
	d->seealsosz += len;
	d->seealso[d->seealsosz] = '\0';
}

/*
 * A definition description is a block of text that we'll later format
 * in mdoc(7).
 * It extends from the name of the definition down to the declarations
 * themselves.
 */
static void
desc(struct parse *p, const char *cp, size_t len)
{
	struct defn	*d;
	size_t		 nsz;

	if (endphase(p, cp) || len < 2)
		return;

	cp += 2;
	len -= 2;

	while (isspace((unsigned char)*cp)) {
		cp++;
		len--;
	}

	/* Fetch current interface definition. */

	d = TAILQ_LAST(&p->dqhead, defnq);
	assert(NULL != d);

	/* Ignore leading blank lines. */

	if (len == 0 && d->desc == NULL)
		return;

	/* Collect SEE ALSO clauses. */

	if (strncasecmp(cp, "see also:", 9) == 0) {
		cp += 9;
		len -= 9;
		while (isspace((unsigned char)*cp)) {
			cp++;
			len--;
		}
		p->phase = PHASE_SEEALSO;
		d->seealso = realloc(d->seealso,
			d->seealsosz + len + 1);
		memcpy(d->seealso + d->seealsosz, cp, len);
		d->seealsosz += len;
		d->seealso[d->seealsosz] = '\0';
		return;
	}

	/* White-space padding between lines. */

	if (d->desc != NULL &&
	    d->descsz > 0 &&
	    d->desc[d->descsz - 1] != ' ' &&
	    d->desc[d->descsz - 1] != '\n') {
		d->desc = realloc(d->desc, d->descsz + 2);
		if (d->desc == NULL)
			err(1, NULL);
		d->descsz++;
		strlcat(d->desc, " ", d->descsz + 1);
	}

	/* Either append the line of a newline, if blank. */

	nsz = len == 0 ? 1 : len;
	if (d->desc == NULL) {
		assert(d->descsz == 0);
		d->desc = calloc(1, nsz + 1);
		if (d->desc == NULL)
			err(1, NULL);
	} else {
		d->desc = realloc(d->desc, d->descsz + nsz + 1);
		if (d->desc == NULL)
			err(1, NULL);
	}

	d->descsz += nsz;
	strlcat(d->desc, len == 0 ? "\n" : cp, d->descsz + 1);
}

/*
 * Copy all KEYWORDS into a buffer.
 */
static void
keys(struct parse *p, const char *cp, size_t len)
{
	struct defn	*d;

	if (endphase(p, cp) || len < 2)
		return;

	cp += 2;
	len -= 2;
	while (isspace((unsigned char)*cp)) {
		cp++;
		len--;
	}

	if (len == 0) {
		p->phase = PHASE_DESC;
		return;
	} else if (strncmp(cp, "KEYWORDS:", 9))
		return;

	cp += 9;
	len -= 9;

	d = TAILQ_LAST(&p->dqhead, defnq);
	assert(NULL != d);
	d->keybuf = realloc(d->keybuf, d->keybufsz + len + 1);
	if (d->keybuf == NULL)
		err(1, NULL);
	memcpy(d->keybuf + d->keybufsz, cp, len);
	d->keybufsz += len;
	d->keybuf[d->keybufsz] = '\0';
}

/*
 * Initial state is where we're scanning forward to find commented
 * instances of CAPI3REF.
 */
static void
init(struct parse *p, const char *cp)
{
	struct defn	*d;
	size_t		 i, sz;

	/* Look for comment hook. */

	if (cp[0] != '*' || cp[1] != '*')
		return;
	cp += 2;
	while (isspace((unsigned char)*cp))
		cp++;

	/* Look for beginning of definition. */

	if (strncmp(cp, "CAPI3REF:", 9))
		return;
	cp += 9;
	while (isspace((unsigned char)*cp))
		cp++;
	if (*cp == '\0') {
		warnx("%s:%zu: warn: unexpected end of "
			"interface definition", p->fn, p->ln);
		return;
	}

	/* Add definition to list of existing ones. */

	if ((d = calloc(1, sizeof(struct defn))) == NULL)
		err(1, NULL);
	if ((d->name = strdup(cp)) == NULL)
		err(1, NULL);

	/* Strip trailing spaces and periods. */

	for (sz = strlen(d->name); sz > 0; sz--)
		if (d->name[sz - 1] == '.' ||
		    d->name[sz - 1] == ' ')
			d->name[sz - 1] = '\0';
		else
			break;

	/*
	 * Un-title case.  Use a simple heuristic where all words
	 * starting with an upper case letter followed by a not
	 * uppercase letter are lowercased.
	 */

	for (i = 0; sz > 0 && i < sz - 1; i++)
		if ((i == 0 || d->name[i - 1] == ' ') &&
		    isupper((unsigned char)d->name[i]) &&
		    !isupper((unsigned char)d->name[i + 1]) &&
		    !ispunct((unsigned char)d->name[i + 1]))
			d->name[i] = tolower((unsigned char)d->name[i]);

	d->fn = p->fn;
	d->ln = p->ln;
	p->phase = PHASE_KEYS;
	TAILQ_INIT(&d->dcqhead);
	TAILQ_INSERT_TAIL(&p->dqhead, d, entries);
}

#define	BPOINT(_cp) \
	(';' == (_cp)[0] || \
	 '[' == (_cp)[0] || \
	 ('(' == (_cp)[0] && '*' != (_cp)[1]) || \
	 ')' == (_cp)[0] || \
	 '{' == (_cp)[0])

/*
 * Given a declaration (be it preprocessor or C), try to parse out a
 * reasonable "name" for the affair.
 * For a struct, for example, it'd be the struct name.
 * For a typedef, it'd be the type name.
 * For a function, it'd be the function name.
 */
static void
grok_name(const struct decl *e,
	const char **start, size_t *sz)
{
	const char	*cp;

	*start = NULL;
	*sz = 0;

	if (DECLTYPE_CPP != e->type) {
		if (e->text[e->textsz - 1] != ';')
			return;
		cp = e->text;
		do {
			while (isspace((unsigned char)*cp))
				cp++;
			if (BPOINT(cp))
				break;
			/* Function pointers... */
			if (*cp == '(')
				cp++;
			/* Pass over pointers. */
			while (*cp == '*')
				cp++;
			*start = cp;
			*sz = 0;
			while (!isspace((unsigned char)*cp)) {
				if (BPOINT(cp))
					break;
				cp++;
				(*sz)++;
			}
		} while (!BPOINT(cp));
	} else {
		*sz = e->textsz;
		*start = e->text;
	}
}

/*
 * Extract information from the interface definition.
 * Mark it as "postprocessed" on success.
 */
static void
postprocess(const char *prefix, struct defn *d)
{
	struct decl	*first;
	const char	*start;
	size_t		 offs, sz, i;
	ENTRY		 ent;

	if (TAILQ_EMPTY(&d->dcqhead))
		return;

	/* Find the first #define or declaration. */

	TAILQ_FOREACH(first, &d->dcqhead, entries)
		if (DECLTYPE_CPP == first->type ||
		    DECLTYPE_C == first->type)
			break;

	if (first == NULL) {
		warnx("%s:%zu: no entry to document", d->fn, d->ln);
		return;
	}

	/*
	 * Now compute the document name (`Dt').
	 * We'll also use this for the filename.
	 */

	grok_name(first, &start, &sz);
	if (start == NULL) {
		warnx("%s:%zu: couldn't deduce "
			"entry name", d->fn, d->ln);
		return;
	}

	/* Document name needs all-caps. */

	if ((d->dt = strndup(start, sz)) == NULL)
		err(1, NULL);
	sz = strlen(d->dt);
	for (i = 0; i < sz; i++)
		d->dt[i] = toupper((unsigned char)d->dt[i]);

	/* Filename needs no special chars. */

	if (filename) {
		asprintf(&d->fname, "%.*s.3", (int)sz, start);
		offs = 0;
	} else {
		asprintf(&d->fname, "%s/%.*s.3",
			prefix, (int)sz, start);
		offs = strlen(prefix) + 1;
	}

	if (d->fname == NULL)
		err(1, NULL);

	for (i = 0; i < sz; i++) {
		if (isalnum((unsigned char)d->fname[offs + i]) ||
		    d->fname[offs + i] == '_' ||
		    d->fname[offs + i] == '-')
			continue;
		d->fname[offs + i] = '_';
	}

	/*
	 * First, extract all keywords.
	 */
	for (i = 0; i < d->keybufsz; ) {
		while (isspace((unsigned char)d->keybuf[i]))
			i++;
		if (i == d->keybufsz)
			break;
		sz = 0;
		start = &d->keybuf[i];
		if (d->keybuf[i] == '{') {
			start = &d->keybuf[++i];
			for ( ; i < d->keybufsz; i++, sz++)
				if (d->keybuf[i] == '}')
					break;
			if (d->keybuf[i] == '}')
				i++;
		} else
			for ( ; i < d->keybufsz; i++, sz++)
				if (isspace((unsigned char)d->keybuf[i]))
					break;
		if (sz == 0)
			continue;
		d->keys = reallocarray(d->keys,
			d->keysz + 1, sizeof(char *));
		if (d->keys == NULL)
			err(1, NULL);
		d->keys[d->keysz] = malloc(sz + 1);
		if (d->keys[d->keysz] == NULL)
			err(1, NULL);
		memcpy(d->keys[d->keysz], start, sz);
		d->keys[d->keysz][sz] = '\0';
		d->keysz++;
		
		/* Hash the keyword. */
		ent.key = d->keys[d->keysz - 1];
		ent.data = d;
		(void)hsearch(ent, ENTER);
	}

	/*
	 * Now extract all `Nm' values for this document.
	 * We only use CPP and C references, and hope for the best when
	 * doing so.
	 * Enter each one of these as a searchable keyword.
	 */
	TAILQ_FOREACH(first, &d->dcqhead, entries) {
		if (DECLTYPE_CPP != first->type &&
		    DECLTYPE_C != first->type)
			continue;
		grok_name(first, &start, &sz);
		if (start == NULL)
			continue;
		d->nms = reallocarray(d->nms,
			d->nmsz + 1, sizeof(char *));
		if (d->nms == NULL)
			err(1, NULL);
		d->nms[d->nmsz] = malloc(sz + 1);
		if (d->nms[d->nmsz] == NULL)
			err(1, NULL);
		memcpy(d->nms[d->nmsz], start, sz);
		d->nms[d->nmsz][sz] = '\0';
		d->nmsz++;

		/* Hash the name. */
		ent.key = d->nms[d->nmsz - 1];
		ent.data = d;
		(void)hsearch(ent, ENTER);
	}

	if (d->nmsz == 0) {
		warnx("%s:%zu: couldn't deduce "
			"any names", d->fn, d->ln);
		return;
	}

	/*
	 * Next, scan for all `Xr' values.
	 * We'll add more to this list later.
	 */
	for (i = 0; i < d->seealsosz; i++) {
		/*
		 * Find next value starting with `['.
		 * There's other stuff in there (whitespace or
		 * free text leading up to these) that we're ok
		 * to ignore.
		 */
		while (i < d->seealsosz && d->seealso[i] != '[')
			i++;
		if (i == d->seealsosz)
			break;

		/*
		 * Now scan for the matching `]'.
		 * We can also have a vertical bar if we're separating a
		 * keyword and its shown name.
		 */
		start = &d->seealso[++i];
		sz = 0;
		while (i < d->seealsosz &&
		      d->seealso[i] != ']' &&
		      d->seealso[i] != '|') {
			i++;
			sz++;
		}
		if (i == d->seealsosz)
			break;
		if (sz == 0)
			continue;

		/*
		 * Continue on to the end-of-reference, if we weren't
		 * there to begin with.
		 */
		if (d->seealso[i] != ']')
			while (i < d->seealsosz &&
			      d->seealso[i] != ']')
				i++;

		/* Strip trailing whitespace. */
		while (sz > 1 && start[sz - 1] == ' ')
			sz--;

		/* Strip trailing parenthesis. */
		if (sz > 2 &&
		    start[sz - 2] == '(' &&
	 	    start[sz - 1] == ')')
			sz -= 2;

		d->xrs = reallocarray(d->xrs,
			d->xrsz + 1, sizeof(char *));
		if (d->xrs == NULL)
			err(1, NULL);
		d->xrs[d->xrsz] = malloc(sz + 1);
		if (d->xrs[d->xrsz] == NULL)
			err(1, NULL);
		memcpy(d->xrs[d->xrsz], start, sz);
		d->xrs[d->xrsz][sz] = '\0';
		d->xrsz++;
	}

	/*
	 * Next, extract all references.
	 * We'll accumulate these into a list of SEE ALSO tags, after.
	 * See how these are parsed above for a description: this is
	 * basically the same thing.
	 */
	for (i = 0; i < d->descsz; i++) {
		if (d->desc[i] != '[')
			continue;
		i++;
		if (d->desc[i] == '[')
			continue;

		start = &d->desc[i];
		for (sz = 0; i < d->descsz; i++, sz++)
			if (d->desc[i] == ']' ||
			    d->desc[i] == '|')
				break;

		if (i == d->descsz)
			break;
		else if (sz == 0)
			continue;

		if (d->desc[i] != ']')
			while (i < d->descsz && d->desc[i] != ']')
				i++;

		while (sz > 1 && start[sz - 1] == ' ')
			sz--;

		if (sz > 2 &&
		    start[sz - 2] == '(' &&
		    start[sz - 1] == ')')
			sz -= 2;

		d->xrs = reallocarray(d->xrs,
			d->xrsz + 1, sizeof(char *));
		if (d->xrs == NULL)
			err(1, NULL);
		d->xrs[d->xrsz] = malloc(sz + 1);
		if (d->xrs[d->xrsz] == NULL)
			err(1, NULL);
		memcpy(d->xrs[d->xrsz], start, sz);
		d->xrs[d->xrsz][sz] = '\0';
		d->xrsz++;
	}

	d->postprocessed = 1;
}

/*
 * Convenience function to look up which manpage "hosts" a certain
 * keyword.  For example, SQLITE_OK(3) also handles SQLITE_TOOBIG and so
 * on, so a reference to SQLITE_TOOBIG should actually point to
 * SQLITE_OK.
 * Returns the keyword's file if found or NULL.
 */
static const char *
lookup(const char *key)
{
	ENTRY			 ent;
	ENTRY			*res;
	const struct defn	*d;

	ent.key = (char *)(uintptr_t)key;
	ent.data = NULL;

	if ((res = hsearch(ent, FIND)) == NULL)
		return NULL;

	d = (const struct defn *)res->data;
	if (d->nmsz == 0)
		return NULL;

	assert(d->nms[0] != NULL);
	return d->nms[0];
}

static int
xrcmp(const void *p1, const void *p2)
{
	/* Silence bogus warnings about un-consting. */

	const char	*s1 = lookup(*(const char **)(uintptr_t)p1),
			*s2 = lookup(*(const char **)(uintptr_t)p2);

	if (s1 == NULL)
		s1 = "";
	if (s2 == NULL)
		s2 = "";

	return strcasecmp(s1, s2);
}

/*
 * Return non-zero if "new sentence, new line" is in effect, zero
 * otherwise.
 * Accepts the start and finish offset of a buffer.
 */
static int
newsentence(size_t start, size_t finish, const char *buf)
{
	size_t	 span = finish - start;
	
	assert(finish >= start);

	/* Ignore "i.e." and "e.g.". */

	if ((span >= 4 &&
	     strncasecmp(&buf[finish - 4], "i.e.", 4) == 0) ||
	    (span >= 4 &&
	     strncasecmp(&buf[finish - 4], "e.g.", 4) == 0))
		return 0;

	return 1;
}

/*
 * Emit a valid mdoc(7) document within the given prefix.
 */
static void
emit(struct defn *d)
{
	struct decl	*first;
	size_t		 sz, i, j, col, last, ns, fnsz, stripspace;
	FILE		*f;
	char		*cp;
	const char	*res, *lastres, *args, *str, *end, *fn;
	enum tag	 tag;
	enum preproc	 pre;

	if (!d->postprocessed) {
		warnx("%s:%zu: interface has errors, not "
			"producing manpage", d->fn, d->ln);
		return;
	}

	if (nofile == 0) {
		if ((f = fopen(d->fname, "w")) == NULL) {
			warn("%s: fopen", d->fname);
			return;
		}
	} else if (filename) {
		printf("%s\n", d->fname);
		return;
	} else
		f = stdout;

	/* Begin by outputting the mdoc(7) header. */

	fputs(".Dd $" "Mdocdate$\n", f);
	fprintf(f, ".Dt %s 3\n", d->dt);
	fputs(".Os\n", f);
	fputs(".Sh NAME\n", f);

	/* Now print the name bits of each declaration. */

	for (i = 0; i < d->nmsz; i++)
		fprintf(f, ".Nm %s%s\n", d->nms[i],
			i < d->nmsz - 1 ? " ," : "");

	fprintf(f, ".Nd %s\n", d->name);
	fputs(".Sh SYNOPSIS\n", f);
	fputs(".In sqlite3.h\n", f);

	TAILQ_FOREACH(first, &d->dcqhead, entries) {
		if (first->type != DECLTYPE_CPP &&
		    first->type != DECLTYPE_C)
			continue;

		/* Easy: just print the CPP name. */

		if (first->type == DECLTYPE_CPP) {
			fprintf(f, ".Fd #define %s\n",
				first->text);
			continue;
		}

		/* First, strip out the sqlite CPPs. */

		for (i = 0; i < first->textsz; ) {
			for (pre = 0; pre < PREPROC__MAX; pre++) {
				sz = strlen(preprocs[pre]);
				if (strncmp(preprocs[pre],
				    &first->text[i], sz))
					continue;
				i += sz;
				while (isspace((unsigned char)first->text[i]))
					i++;
				break;
			}
			if (pre == PREPROC__MAX)
				break;
		}

		/* If we're a typedef, immediately print Vt. */

		if (strncmp(&first->text[i], "typedef", 7) == 0) {
			fprintf(f, ".Vt %s\n", &first->text[i]);
			continue;
		}

		/* Are we a struct? */

		if (first->textsz > 2 &&
		    first->text[first->textsz - 2] == '}' &&
		    (cp = strchr(&first->text[i], '{')) != NULL) {
			*cp = '\0';
			fprintf(f, ".Vt %s;\n", &first->text[i]);
			/* Restore brace for later usage. */
			*cp = '{';
			continue;
		}

		/* Catch remaining non-functions. */

		if (first->textsz > 2 &&
		    first->text[first->textsz - 2] != ')') {
			fprintf(f, ".Vt %s\n", &first->text[i]);
			continue;
		}

		str = &first->text[i];
		if ((args = strchr(str, '(')) == NULL || args == str) {
			/* What is this? */
			fputs(".Bd -literal\n", f);
			fputs(&first->text[i], f);
			fputs("\n.Ed\n", f);
			continue;
		}

		/*
		 * Current state:
		 *  type_t *function      (args...)
		 *  ^str                  ^args
		 * Scroll back to end of function name.
		 */

		end = args - 1;
		while (end > str && isspace((unsigned char)*end))
			end--;

		/*
		 * Current state:
		 *  type_t *function      (args...)
		 *  ^str           ^end   ^args
		 * Scroll back to what comes before.
		 */

		for (fnsz = 0; end > str; end--, fnsz++)
			if (isspace((unsigned char)*end) || *end == '*')
				break;

		if (fnsz == 0)
			warnx("%s:%zu: zero-length "
				"function name", d->fn, d->ln);
		fn = end + 1;

		/*
		 * Current state:
		 *  type_t *function      (args...)
		 *  ^str   ^end           ^args
		 *  type_t  function      (args...)
		 *  ^str   ^end           ^args
		 * Strip away whitespace.
		 */

		while (end > str && isspace((unsigned char)*end))
			end--;

		/*
		 * type_t *function      (args...)
		 * ^str   ^end           ^args
		 * type_t  function      (args...)
		 * ^str ^end             ^args
		 */

		/*
		 * If we can't find what came before, then the function
		 * has no type, which is odd... let's just call it void.
		 */

		if (end > str) {
			fprintf(f, ".Ft %.*s\n",
				(int)(end - str + 1), str);
			fprintf(f, ".Fo %.*s\n", (int)fnsz, fn);
		} else {
			fputs(".Ft void\n", f);
			fprintf(f, ".Fo %.*s\n", (int)fnsz, fn);
		}

		/*
		 * Convert function arguments into `Fa' clauses.
		 * This also handles nested function pointers, which
		 * would otherwise throw off the delimeters.
		 */

		for (;;) {
			str = ++args;
			while (isspace((unsigned char)*str))
				str++;
			fputs(".Fa \"", f);
			ns = 0;
			while (*str != '\0' &&
			       (ns || *str != ',') &&
			       (ns || *str != ')')) {
				/*
				 * Handle comments in the declarations.
				 */
				if (str[0] == '/' && str[1] == '*') {
					str += 2;
					for ( ; str[0] != '\0'; str++)
						if (str[0] == '*' && str[1] == '/')
							break;
					if (*str == '\0')
						break;
					str += 2;
					while (isspace((unsigned char)*str))
						str++;
					if (*str == '\0' ||
					    (ns == 0 && *str == ',') ||
					    (ns == 0 && *str == ')'))
						break;
				}
				if (*str == '(')
					ns++;
				else if (*str == ')')
					ns--;

				/*
				 * Handle some instances of whitespace
				 * by compressing it down.
				 * However, if the whitespace ends at
				 * the end-of-definition, then don't
				 * print it at all.
				 */

				if (isspace((unsigned char)*str)) {
					while (isspace((unsigned char)*str))
						str++;
					/* Are we at a comment? */
					if (str[0] == '/' && str[1] == '*')
						continue;
					if (*str == '\0' ||
					    (ns == 0 && *str == ',') ||
					    (ns == 0 && *str == ')'))
						break;
					fputc(' ', f);
				} else {
					fputc(*str, f);
					str++;
				}
			}
			fputs("\"\n", f);
			if (*str == '\0' || *str == ')')
				break;
			args = str;
		}

		fputs(".Fc\n", f);
	}

	fputs(".Sh DESCRIPTION\n", f);

	/*
	 * Strip the crap out of the description.
	 * "Crap" consists of things I don't understand that mess up
	 * parsing of the HTML, for instance,
	 *   <dl>[[foo bar]]<dt>foo bar</dt>...</dl>
	 * These are not well-formed HTML.
	 * Note that d->desc[d->descz] is the NUL terminator, so we
	 * don't need to check d->descsz - 1.
	 */

	for (i = 0; i < d->descsz; ) {
		if (d->desc[i] == '^' &&
		    d->desc[i + 1] == '(') {
			memmove(&d->desc[i],
				&d->desc[i + 2],
				d->descsz - i - 1);
			d->descsz -= 2;
			continue;
		} else if (d->desc[i] == ')' &&
			   d->desc[i + 1] == '^') {
			memmove(&d->desc[i],
				&d->desc[i + 2],
				d->descsz - i - 1);
			d->descsz -= 2;
			continue;
		} else if (d->desc[i] == '^') {
			memmove(&d->desc[i],
				&d->desc[i + 1],
				d->descsz - i);
			d->descsz -= 1;
			continue;
		} else if (d->desc[i] != '[' ||
			   d->desc[i + 1] != '[') {
			i++;
			continue;
		}

		for (j = i; j < d->descsz; j++)
			if (d->desc[j] == ']' &&
			    d->desc[j + 1] == ']')
				break;

		/* Ignore if we don't have a terminator. */

		assert(j > i);
		j += 2;
		if (j > d->descsz) {
			i++;
			continue;
		}

		memmove(&d->desc[i], &d->desc[j], d->descsz - j + 1);
		d->descsz -= (j - i);
	}

	/*
	 * Here we go!
	 * Print out the description as best we can.
	 * Do on-the-fly processing of any HTML we encounter into
	 * mdoc(7) and try to break lines up.
	 */

	col = stripspace = 0;

	for (i = 0; i < d->descsz; ) {
		/*
		 * The "stripspace" variable is set to >=2 if we've
		 * stripped white-space off before an anticipated macro.
		 * Without it, if the macro ends up *not* being a macro,
		 * we wouldn't flush the line and thus end up losing a
		 * space.  This lets the code that flushes the line know
		 * that we've stripped spaces and adds them back in.
		 */

		if (stripspace > 0)
			stripspace--;

		/* Ignore NUL byte, just in case. */

		if (d->desc[i] == '\0') {
			i++;
			continue;
		}

		/*
		 * Newlines are paragraph breaks.
		 * If we have multiple newlines, then keep to a single
		 * `Pp' to keep it clean.
		 * Only do this if we're not before a block-level HTML,
		 * as this would mean, for instance, a `Pp'-`Bd' pair.
		 */

		if (d->desc[i] == '\n') {
			while (isspace((unsigned char)d->desc[i]))
				i++;
			for (tag = 0; tag < TAG__MAX; tag++) {
				sz = strlen(tags[tag].html);
				if (strncasecmp(&d->desc[i],
				    tags[tag].html, sz) == 0)
					break;
			}
			if (tag == TAG__MAX ||
			    (tags[tag].flags & TAGINFO_INLINE)) {
				if (col > 0)
					fputs("\n", f);
				fputs(".Pp\n", f);
				/* We're on a new line. */
				col = 0;
			}
			continue;
		}

		/*
		 * New sentence, new line.
		 * We guess whether this is the case by using the
		 * dumbest possible heuristic.
		 */

		if (d->desc[i] == ' ' &&
		    i > 0 && d->desc[i - 1] == '.') {
			for (j = i - 1; j > 0; j--)
				if (isspace((unsigned char)d->desc[j])) {
					j++;
					break;
				}
			if (newsentence(j, i, d->desc)) {
				while (d->desc[i] == ' ')
					i++;
				fputc('\n', f);
				col = 0;
				continue;
			}
		}

		/*
		 * After 65 characters, force a break when we encounter
		 * white-space to keep our lines more or less tidy.
		 */

		if (col > 65 && d->desc[i] == ' ') {
			while (d->desc[i] == ' ' )
				i++;
			fputc('\n', f);
			col = 0;
			continue;
		}

		/* Parse HTML tags and links. */

		if (d->desc[i] == '<') {
			for (tag = 0; tag < TAG__MAX; tag++) {
				sz = strlen(tags[tag].html);
				assert(sz > 0);
				if (strncmp(&d->desc[i],
				    tags[tag].html, sz))
					continue;

				i += sz;

				/* Blindly ignore attributes. */

				if (tags[tag].flags & TAGINFO_ATTRS) {
					while (d->desc[i] != '\0' &&
					       d->desc[i] != '>')
						i++;
					if (d->desc[i] == '\0')
						break;
					i++;
				}

				/*
				 * NOOP tags don't do anything, such as
				 * the case of `</dd>', which only
				 * serves to end an `It' block that will
				 * be closed out by a subsequent `It' or
				 * end of clause `El' anyway.
				 * Skip the trailing space.
				 */

				if (tags[tag].flags & TAGINFO_NOOP) {
					while (isspace((unsigned char)d->desc[i]))
						i++;
					break;
				} else if (tags[tag].flags & TAGINFO_INLINE) {
					while (stripspace > 0) {
						fputc(' ', f);
						col++;
						stripspace--;
					}
					fputs(tags[tag].mdoc, f);
					/*col += strlen(tags[tag].mdoc);*/
					break;
				}

				/*
				 * A breaking mdoc(7) statement.
				 * Break the current line, output the
				 * macro, and conditionally break
				 * following that (or we might do
				 * nothing at all).
				 */

				if (col > 0) {
					fputs("\n", f);
					col = 0;
				}

				fputs(tags[tag].mdoc, f);
				if (!(tags[tag].flags & TAGINFO_NOBR)) {
					fputs("\n", f);
					col = 0;
				} else if (!(tags[tag].flags & TAGINFO_NOSP)) {
					fputs(" ", f);
					col++;
				}
				while (isspace((unsigned char)d->desc[i]))
					i++;
				break;
			}
			if (tag < TAG__MAX) {
				stripspace = 0;
				continue;
			}
			while (stripspace > 0) {
				fputc(' ', f);
				col++;
				stripspace--;
			}
		} else if (d->desc[i] == '[' && d->desc[i + 1] != ']') {
			/* Do we start at the bracket or bar? */

			for (sz = i + 1; sz < d->descsz; sz++)
				if (d->desc[sz] == '|' ||
				    d->desc[sz] == ']')
					break;

			/* This is a degenerate case. */

			if (sz == d->descsz) {
				i++;
				stripspace = 0;
				continue;
			}

			/*
			 * Look for a trailing "()", using "j" as a
			 * sentinel in case it was found.  This lets us
			 * print out a "Fn xxxx" instead of having the
			 * function be ugly.  If we don't have a Fn and
			 * we'd stripped space before this, remember to
			 * add the space back in.
			 */

			j = 0;
			if (d->desc[sz] != '|') {
				i = i + 1;
				if (sz > 2 &&
				    d->desc[sz - 1] == ')' &&
				    d->desc[sz - 2] == '(') {
					if (col > 0)
						fputc('\n', f);
					fputs(".Fn ", f);
					j = sz - 2;
					assert(j > 0);
				} else if (stripspace) {
					fputc(' ', f);
					col++;
				}
			} else {
				if (stripspace) {
					fputc(' ', f);
					col++;
				}
				i = sz + 1;
			}

			while (isspace((unsigned char)d->desc[i]))
				i++;

			/*
			 * Now handle in-page references.  If we're a
			 * function reference (e.g., function()), then
			 * omit the trailing parentheses and put in a Fn
			 * block.  Otherwise print them out as-is: we've
			 * already accumulated them into our "SEE ALSO"
			 * values, which we'll use below.
			 */

			for ( ; i < d->descsz; i++, col++) {
				if (j > 0 && i == j) {
					i += 3;
					for ( ; i < d->descsz; i++)
						if (d->desc[i] == '.')
							fputs(" .", f);
						else if (d->desc[i] == ',')
							fputs(" ,", f);
						else if (d->desc[i] == ')')
							fputs(" )", f);
						else
							break;

					/* Trim trailing space. */

					while (i < d->descsz &&
					       isspace((unsigned char)d->desc[i]))
						i++;	

					fputc('\n', f);
					col = 0;
					break;
				} else if (d->desc[i] == ']') {
					i++;
					break;
				}
				fputc(d->desc[i], f);
				col++;
			}

			stripspace = 0;
			continue;
		}

		/* Strip leading spaces from output. */

		if (d->desc[i] == ' ' && col == 0) {
			while (d->desc[i] == ' ')
				i++;
			continue;
		}

		/*
		 * Strip trailing spaces from output.
		 * Set "stripspace" to be the number of white-space
		 * characters that we've skipped, plus one.
		 * This means that the next loop iteration while get the
		 * actual amount we've skipped (for '<' or '[') and we
		 * can act upon it there.
		 */
		
		if (d->desc[i] == ' ') {
			j = i;
			while (j < d->descsz && d->desc[j] == ' ')
				j++;
			if (j < d->descsz &&
			    (d->desc[j] == '\n' ||
			     d->desc[j] == '<' ||
			     d->desc[j] == '[')) {
				stripspace = d->desc[j] != '\n' ?
					(j - i + 1) : 0;
				i = j;
				continue;
			}
		}

		assert(d->desc[i] != '\n');

		/*
		 * Handle some oddities.
		 * The following HTML escapes exist in the output that I
		 * could find.
		 * There might be others...
		 */

		if (strncmp(&d->desc[i], "&rarr;", 6) == 0) {
			i += 6;
			fputs("\\(->", f);
		} else if (strncmp(&d->desc[i], "&larr;", 6) == 0) {
			i += 6;
			fputs("\\(<-", f);
		} else if (strncmp(&d->desc[i], "&nbsp;", 6) == 0) {
			i += 6;
			fputc(' ', f);
		} else if (strncmp(&d->desc[i], "&lt;", 4) == 0) {
			i += 4;
			fputc('<', f);
		} else if (strncmp(&d->desc[i], "&gt;", 4) == 0) {
			i += 4;
			fputc('>', f);
		} else if (strncmp(&d->desc[i], "&#91;", 5) == 0) {
			i += 5;
			fputc('[', f);
		} else {
			/* Make sure we don't trigger a macro. */
			if (col == 0 &&
			    (d->desc[i] == '.' || d->desc[i] == '\''))
				fputs("\\&", f);
			fputc(d->desc[i], f);
			i++;
		}

		col++;
	}

	if (col > 0)
		fputs("\n", f);

	fputs(".Sh IMPLEMENTATION NOTES\n", f);
	fprintf(f, "These declarations were extracted from the\n"
	      "interface documentation at line %zu.\n", d->ln);
	fputs(".Bd -literal\n", f);
	fputs(d->fulldesc, f);
	fputs(".Ed\n", f);

	/*
	 * Look up all of our keywords (which are in the xrs field) in
	 * the table of all known keywords.
	 * Don't print duplicates.
	 */

	if (d->xrsz > 0) {
		qsort(d->xrs, d->xrsz, sizeof(char *), xrcmp);
		lastres = NULL;
		for (last = 0, i = 0; i < d->xrsz; i++) {
			res = lookup(d->xrs[i]);

			/* Ignore self-reference. */

			if (res == d->nms[0] && verbose)
				warnx("%s:%zu: self-reference: %s",
					d->fn, d->ln, d->xrs[i]);
			if (res == d->nms[0])
				continue;
			if (res == NULL && verbose)
				warnx("%s:%zu: ref not found: %s",
					d->fn, d->ln, d->xrs[i]);
			if (res == NULL)
				continue;

			/* Ignore duplicates. */

			if (lastres == res)
				continue;

			if (last)
				fputs(" ,\n", f);
			else
				fputs(".Sh SEE ALSO\n", f);

			fprintf(f, ".Xr %s 3", res);
			last = 1;
			lastres = res;
		}
		if (last)
			fputs("\n", f);
	}

	if (nofile == 0)
		fclose(f);
}

#if HAVE_PLEDGE
/*
 * We pledge(2) stdio if we're receiving from stdin and writing to
 * stdout, otherwise we need file-creation and writing.
 */
static void
sandbox_pledge(void)
{

	if (nofile) {
		if (pledge("stdio", NULL) == -1)
			err(1, NULL);
	} else {
		if (pledge("stdio wpath cpath", NULL) == -1)
			err(1, NULL);
	}
}
#endif

#if HAVE_SANDBOX_INIT
/*
 * Darwin's "seatbelt".
 * If we're writing to stdout, then use pure computation.
 * Otherwise we need file writing.
 */
static void
sandbox_apple(void)
{
	char	*ep;
	int	 rc;

	rc = sandbox_init
		(nofile ? kSBXProfilePureComputation :
		 kSBXProfileNoNetwork, SANDBOX_NAMED, &ep);
	if (rc == 0)
		return;
	perror(ep);
	sandbox_free_error(ep);
	exit(1);
}
#endif

/*
 * Check to see whether there are any filename duplicates.
 * This is just a warning, but will really screw things up, since the
 * last filename will overwrite the first.
 */
static void
check_dupes(struct parse *p)
{
	const struct defn	*d, *dd;

	TAILQ_FOREACH(d, &p->dqhead, entries)
		TAILQ_FOREACH_REVERSE(dd, &p->dqhead, defnq, entries) {
			if (dd == d)
				break;
			if (d->fname == NULL ||
			    dd->fname == NULL ||
			    strcmp(d->fname, dd->fname))
				continue;
			warnx("%s:%zu: duplicate filename: "
				"%s (from %s, line %zu)", d->fn,
				d->ln, d->fname, dd->nms[0], dd->ln);
		}
}

int
main(int argc, char *argv[])
{
	size_t		 i, bufsz;
	ssize_t		 len;
	FILE		*f = stdin;
	char		*cp = NULL;
	const char	*prefix = ".";
	struct parse	 p;
	int		 rc = 0, ch;
	struct defn	*d;
	struct decl	*e;

	memset(&p, 0, sizeof(struct parse));

	p.fn = "<stdin>";
	p.ln = 0;
	p.phase = PHASE_INIT;

	TAILQ_INIT(&p.dqhead);

	while ((ch = getopt(argc, argv, "nNp:v")) != -1)
		switch (ch) {
		case 'n':
			nofile = 1;
			break;
		case 'N':
			nofile = 1;
			filename = 1;
			break;
		case 'p':
			prefix = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			goto usage;
		}

	argc -= optind;
	argv += optind;

	if (argc > 1)
		goto usage;

	if (argc > 0) {
		if ((f = fopen(argv[0], "r")) == NULL)
			err(1, "%s", argv[0]);
		p.fn = argv[0];
	}

#if HAVE_SANDBOX_INIT
	sandbox_apple();
#elif HAVE_PLEDGE
	sandbox_pledge();
#endif
	/*
	 * Read in line-by-line and process in the phase dictated by our
	 * finite state automaton.
	 */
	
	while ((len = getline(&cp, &bufsz, f)) != -1) {
		assert(len > 0);
		p.ln++;
		if (cp[len - 1] != '\n') {
			warnx("%s:%zu: unterminated line", p.fn, p.ln);
			break;
		}

		/*
		 * Lines are now always NUL-terminated, and don't allow
		 * NUL characters in the line.
		 */

		cp[--len] = '\0';
		len = strlen(cp);

		switch (p.phase) {
		case PHASE_INIT:
			init(&p, cp);
			break;
		case PHASE_KEYS:
			keys(&p, cp, (size_t)len);
			break;
		case PHASE_DESC:
			desc(&p, cp, (size_t)len);
			break;
		case PHASE_SEEALSO:
			seealso(&p, cp, (size_t)len);
			break;
		case PHASE_DECL:
			decl(&p, cp, (size_t)len);
			break;
		}
	}

	/*
	 * If we hit the last line, then try to process.
	 * Otherwise, we failed along the way.
	 */

	if (feof(f)) {
		/*
		 * Allow us to be at the declarations or scanning for
		 * the next clause.
		 */
		if (p.phase == PHASE_INIT ||
		    p.phase == PHASE_DECL) {
			if (hcreate(5000) == 0)
				err(1, NULL);
			TAILQ_FOREACH(d, &p.dqhead, entries)
				postprocess(prefix, d);
			check_dupes(&p);
			TAILQ_FOREACH(d, &p.dqhead, entries)
				emit(d);
			rc = 1;
		} else if (p.phase != PHASE_DECL)
			warnx("%s:%zu: exit when not in "
				"initial state", p.fn, p.ln);
	}

	while ((d = TAILQ_FIRST(&p.dqhead)) != NULL) {
		TAILQ_REMOVE(&p.dqhead, d, entries);
		while ((e = TAILQ_FIRST(&d->dcqhead)) != NULL) {
			TAILQ_REMOVE(&d->dcqhead, e, entries);
			free(e->text);
			free(e);
		}
		free(d->name);
		free(d->desc);
		free(d->fulldesc);
		free(d->dt);
		for (i = 0; i < d->nmsz; i++)
			free(d->nms[i]);
		for (i = 0; i < d->xrsz; i++)
			free(d->xrs[i]);
		for (i = 0; i < d->keysz; i++)
			free(d->keys[i]);
		free(d->keys);
		free(d->nms);
		free(d->xrs);
		free(d->fname);
		free(d->seealso);
		free(d->keybuf);
		free(d);
	}

	return !rc;
usage:
	fprintf(stderr, "usage: %s [-Nnv] [-p prefix] [file]\n",
		getprogname());
	return 1;
}
