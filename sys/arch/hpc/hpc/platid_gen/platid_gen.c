/*	$NetBSD: platid_gen.c,v 1.3.4.1 2001/10/01 12:38:43 fvdl Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

#include "platid_gen.h"

/*
 * constants
 */
#define SEARCH_IGNORECASE	(1<<0)

#define NMODES	2
#define MAXNEST	4
#define MAXLEN	1024

enum { FORM_GENHDR, FORM_MASK_H, FORM_MASK_C, FORM_NAME_C, FORM_PARSE_ONLY };

/*
 * data type definitions
 */
struct genctx_t {
	int num;
	const char *alt;
	const char *node_name[2];
	char sym[MAXLEN];
	char name[MAXLEN];
} genctx[NMODES][MAXNEST];

/*
 * function prototypes
 */
void	gen_list(node_t *);
void	gen_output(void);
void	gen_header(void);
void	gen_mask_h(void);
void	gen_mask_c(void);
void	gen_name_c(void);
void	gen_comment(FILE *);
void	enter(void);
void	leave(void);

/*
 * global data
 */
node_t*	def_tree;
int nest;
int mode;
FILE *fp_out;
int form;
int count;

#define MODE_INVALID	-1
#define MODE_CPU	0
#define MODE_MACHINE	1
char* mode_names[] = {
	"CPU", "MACHINE", NULL
};
#define PREFIX	"PLATID"
char* prefix_names[] = {
	"CPU", "MACH",
};
char* shift_names[NMODES][MAXNEST] = {
	{
		"PLATID_CPU_ARCH_SHIFT",
		"PLATID_CPU_SERIES_SHIFT",
		"PLATID_CPU_MODEL_SHIFT",
		"PLATID_CPU_SUBMODEL_SHIFT",
	},
	{
		"PLATID_VENDOR_SHIFT",
		"PLATID_SERIES_SHIFT",
		"PLATID_MODEL_SHIFT",
		"PLATID_SUBMODEL_SHIFT",
	},
};

/*
 * program entry
 */
int
main(int argc, char *argv[])
{
	int i;

	form = FORM_GENHDR;
	fp_out = stdout;
	count = 0;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-header") == 0) {
			form = FORM_GENHDR;
		} else if (strcmp(argv[i], "-mask_h") == 0) {
			form = FORM_MASK_H;
		} else if (strcmp(argv[i], "-mask_c") == 0) {
			form = FORM_MASK_C;
		} else if (strcmp(argv[i], "-name_c") == 0) {
			form = FORM_NAME_C;
		} else if (strcmp(argv[i], "-parse_only") == 0) {
			form = FORM_PARSE_ONLY;
		} else {
		usage:
			fprintf(stderr, "usage platid_gen <option>\n");
			fprintf(stderr, "  option: -header\n");
			fprintf(stderr, "          -mask_h\n");
			fprintf(stderr, "          -mask_c\n");
			fprintf(stderr, "          -name_c\n");
			fprintf(stderr, "          -parse_only\n");
			exit(1);
		}
	}

	if (read_def()) {
		exit(1);
	}

	if (form == FORM_PARSE_ONLY) {
		dump_node("", def_tree);
		exit (0);
	}

	gen_comment(fp_out);
	switch (form) {
	case FORM_GENHDR:
		break;
	case FORM_MASK_H:
		break;
	case FORM_MASK_C:
		fprintf(fp_out, "#include <machine/platid.h>\n");
		fprintf(fp_out, "#include <machine/platid_mask.h>\n");
		break;
	case FORM_NAME_C:
		fprintf(fp_out, "#include <machine/platid.h>\n");
		fprintf(fp_out, "#include <machine/platid_mask.h>\n");
		fprintf(fp_out,"struct platid_name platid_name_table[] = {\n");
		break;
	}

	nest = -1;
	enter(); /* XXX */
	mode = MODE_INVALID;
	genctx[MODE_CPU][nest].alt = NULL;
	genctx[MODE_MACHINE][nest].alt = NULL;
	gen_list((node_t*)def_tree->ptr1);

	switch (form) {
	case FORM_GENHDR:
	case FORM_MASK_H:
	case FORM_MASK_C:
		break;
	case FORM_NAME_C:
		fprintf(fp_out, "};\n");
		fprintf(fp_out, "int platid_name_table_size = sizeof(platid_name_table)/sizeof(*platid_name_table);\n");
		break;
	}
	fclose(fp_out);

	exit(0);
}

int
table_getnum(char **table, char *s, int def, int opt)
{
	int num;

	num = 0;
	while (table[num]) {
		int diff;
		if (opt & SEARCH_IGNORECASE) {
			diff = strcmp(table[num], s);
		} else {
			diff = strcasecmp(table[num], s);
		}
		if (diff == 0) {
			return (num);
		}
		num++;
	}
	return def;
}

#define GET_MODE(s) \
	table_getnum(mode_names, (s), MODE_INVALID, SEARCH_IGNORECASE)
#define GET_ALT(s) \
	table_getnum(mode_names, (s), MODE_INVALID, SEARCH_IGNORECASE)

void
enter()
{
	nest++;
	if (MAXNEST <= nest) {
		fprintf(stderr, "too much nest\n");
		exit(1);
	}
	genctx[mode][nest].num = 0;
	genctx[mode][nest].alt = NULL;
	genctx[mode][nest].node_name[0] = NULL;
	genctx[mode][nest].node_name[1] = NULL;
	if (0 < nest) {
		genctx[mode][nest].alt = genctx[mode][nest - 1].alt;
	}
}

void
leave()
{
	nest--;
	if (nest < 0) {
		fprintf(stderr, "internal error (nest=%d)\n", nest);
		exit(1);
	}
}

void
gen_comment(FILE *fp)
{
	fprintf(fp, "/*\n");
	fprintf(fp, " *  Do not edit.\n");
	fprintf(fp, " *  This file is automatically generated by platid.awk.\n");
	fprintf(fp, " */\n");
}

gen_name(char *buf, struct genctx_t ctx[], int nest, int name, char *punct,
    int ignr)
{
	int i;
	buf[0] = '\0';
	for (i = 0; i <= nest; i++) {
		if (!(ignr <= i && nest != i)) {
			if (i != 0) {
				strcat(buf, punct);
			}
			strcat(buf, ctx[i].node_name[name]);
		}
	}
}

void
gen_list(node_t* np)
{
	int i, t;

	for ( ; np; np = np->link) {
		switch (np->type) {
		case N_LABEL:
			if ((mode = GET_MODE(np->ptr1)) == MODE_INVALID) {
				fprintf(stderr, "invalid mode '%s'\n",
				    np->ptr1);
				exit(1);
			}
			break;
		case N_MODIFIER:
			t = GET_ALT(np->ptr1);
			if (t == MODE_INVALID) {
				fprintf(stderr, "unknown alternater '%s'\n",
				    np->ptr1);
				exit(1);
			}
			if (t == mode) {
				fprintf(stderr,
				    "invalid alternater '%s' (ignored)\n",
				    np->ptr1);
				exit(1);
			}
			genctx[mode][nest].alt = np->ptr2;
			break;
		case N_ENTRY:
			if (np->ptr2 == NULL) {
				char buf[MAXLEN];
				sprintf(buf, "%s%s",
				    nest == 0 ? "" : " ",
				    np->ptr1);
		  		np->ptr2 = strdup(buf);
				if (nest == 3)
					np->val = 1;
			}
			touppers((char*)np->ptr1);

			genctx[mode][nest].num++;
		  	genctx[mode][nest].node_name[0] = np->ptr1;
			genctx[mode][nest].node_name[1] = np->ptr2;
			gen_name(genctx[mode][nest].sym, genctx[mode],
			    nest, 0, "_", nest == 3 ? 2 : nest);
			gen_name(genctx[mode][nest].name, genctx[mode],
			    nest, 1, "", nest - np->val);
			gen_output();
			break;
		case N_LIST:
			enter();
			gen_list((node_t*)np->ptr1);
			leave();
			break;
		case N_DIRECTIVE:
			fprintf(fp_out, "%s", np->ptr1);
			break;
		default:
			fprintf(stderr, "internal error (type=%d)\n", np->type);
			exit(1);
			break;
		}
	}
}

void
gen_output()
{
	switch (form) {
	case FORM_GENHDR:
		gen_header();
		break;
	case FORM_MASK_H:
		gen_mask_h();
		break;
	case FORM_MASK_C:
		gen_mask_c();
		break;
	case FORM_NAME_C:
		gen_name_c();
		break;
	}
}

/*
 * platid_generated.h
 *
 * #define PLATID_CPU_XXX_NUM	1
 * #define PLATID_CPU_XXX	\
 *   ((PLATID_CPU_XXX_NUM << PLATID_CPU_ARCH_SHIFT))
 * #define PLATID_CPU_XXX_YYY	\
 *   ((PLATID_CPU_XXX_YYY_NUM << PLATID_CPU_SERIES_SHIFT)| \
 *     PLATID_CPU_XXX)
 *
 * #ifndef SPEC_PLATFORM
 * #define SPEC_MACH_XXX
 * #endif 
 * #define PLATID_MACH_XXX_NUM	1
 * #define PLATID_MACH_XXX	\
 *   ((PLATID_MACH_XXX_NUM << PLATID_MACH_ARCH_SHIFT))
 * #define PLATID_MACH_XXX_YYY	\
 *   ((PLATID_MACH_XXX_YYY_NUM << PLATID_MACH_SERIES_SHIFT)| \
 *     PLATID_MACH_XXX)
 */
void
gen_header()
{
	char *prefix = prefix_names[mode];
	char *name = genctx[mode][nest].sym;

	if (mode == MODE_MACHINE) {
		fprintf(fp_out, "#ifndef SPEC_PLATFORM\n");
		fprintf(fp_out, "#define %s_%s_%s\n", "SPEC", prefix, name);
		fprintf(fp_out, "#endif /* !SPEC_PLATFORM */\n");
	}
	fprintf(fp_out, "#define %s_%s_%s_NUM\t%d\n", PREFIX, prefix, name,
	    genctx[mode][nest].num);
	fprintf(fp_out, "#define %s_%s_%s\t\\\n", PREFIX, prefix, name);
	fprintf(fp_out, "  ((%s_%s_%s_NUM << %s)", PREFIX, prefix, name,
	    shift_names[mode][nest]);
	if (0 < nest) {
		fprintf(fp_out, "| \\\n    %s_%s_%s",
		    PREFIX, prefix, genctx[mode][nest - 1].sym);
	}
	fprintf(fp_out, ")\n");
}

/*
 * platid_mask.h:
 *
 * extern platid_t platid_mask_CPU_MIPS;
 * #ifdef PLATID_DEFINE_MASK_NICKNAME
 * #  define GENERIC_MIPS ((int)&platid_mask_CPU_MIPS)
 * #endif
 */
void
gen_mask_h()
{
	char *name = genctx[mode][nest].sym;

	fprintf(fp_out, "extern platid_t platid_mask_%s_%s;\n",
	    prefix_names[mode], name);
	fprintf(fp_out, "#ifdef PLATID_DEFINE_MASK_NICKNAME\n");
	fprintf(fp_out, "#  define %s%s ((int)&platid_mask_%s_%s)\n",
	    (mode == MODE_CPU)?"GENERIC_":"",
	    name, prefix_names[mode], name);
	fprintf(fp_out, "#endif\n");
}

/*
 * platid_mask.c:
 *
 * platid_t platid_mask_CPU_MIPS = {{
 * 	PLATID_CPU_MIPS,
 *	PLATID_WILD
 * }};
 */
void
gen_mask_c()
{
	char *name = genctx[mode][nest].sym;

	fprintf(fp_out, "platid_t platid_mask_%s_%s = {{\n",
	    prefix_names[mode], name);
	switch (mode) {
	case MODE_CPU:
		fprintf(fp_out, "\t%s_CPU_%s,\n", PREFIX, name);
		if (genctx[mode][nest].alt == NULL)
			fprintf(fp_out, "\t%s_WILD\n", PREFIX);
		else
			fprintf(fp_out, "\t%s_MACH_%s,\n", PREFIX,
			    genctx[mode][nest].alt);
		break;
	case MODE_MACHINE:
		if (genctx[mode][nest].alt == NULL)
			fprintf(fp_out, "\t%s_WILD,\n", PREFIX);
		else
			fprintf(fp_out, "\t%s_CPU_%s,\n", PREFIX,
			    genctx[mode][nest].alt);
		fprintf(fp_out, "\t%s_MACH_%s\n", PREFIX, name);
		break;
	}
	fprintf(fp_out, "}};\n");
}

/*
 * platid_name.c:
 */
void
gen_name_c()
{
	fprintf(fp_out, "\t{ &platid_mask_%s_%s,\n",
	    prefix_names[mode], genctx[mode][nest].sym);
	fprintf(fp_out, "\t TEXT(\"%s\") },\n", genctx[mode][nest].name);
	count++;
}
