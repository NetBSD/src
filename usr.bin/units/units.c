/*	$NetBSD: units.c,v 1.15.30.1 2009/05/13 19:20:09 jym Exp $	*/

/*
 * units.c   Copyright (c) 1993 by Adrian Mariano (adrian@cam.cornell.edu)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * Disclaimer:  This software is provided by the author "as is".  The author
 * shall not be liable for any damages caused in any way by this software.
 *
 * I would appreciate (though I do not require) receiving a copy of any
 * improvements you might make to this program.
 */

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "pathnames.h"

#define VERSION "1.0"

#ifndef UNITSFILE
#define UNITSFILE _PATH_UNITSLIB
#endif

#define MAXUNITS 1000
#define MAXPREFIXES 50

#define MAXSUBUNITS 500

#define PRIMITIVECHAR '!'

const char *powerstring = "^";

struct {
	const char *uname;
	const char *uval;
}      unittable[MAXUNITS];

struct unittype {
	const char *numerator[MAXSUBUNITS];
	const char *denominator[MAXSUBUNITS];
	double factor;
};

struct {
	const char *prefixname;
	const char *prefixval;
}      prefixtable[MAXPREFIXES];


const char *NULLUNIT = "";

int unitcount;
int prefixcount;


int	addsubunit __P((const char *[], const char *));
int	addunit __P((struct unittype *, const char *, int));
void	cancelunit __P((struct unittype *));
int	compare __P((const void *, const void *));
int	compareproducts __P((const char **, const char **));
int	compareunits __P((struct unittype *, struct unittype *));
int	compareunitsreciprocal __P((struct unittype *, struct unittype *));
int	completereduce __P((struct unittype *));
void	initializeunit __P((struct unittype *));
int	main __P((int, char **));
void	readerror __P((int));
void	readunits __P((const char *));
int	reduceproduct __P((struct unittype *, int));
int	reduceunit __P((struct unittype *));
void	showanswer __P((struct unittype *, struct unittype *));
void	showunit __P((struct unittype *));
void	sortunit __P((struct unittype *));
void	usage __P((void));
void	zeroerror __P((void));
char   *dupstr __P((const char *));
const char *lookupunit __P((const char *));


char *
dupstr(const char *str)
{
	char *ret;

	ret = strdup(str);
	if (!ret)
		err(3, "Memory allocation error");
	return (ret);
}


void 
readerror(int linenum)
{
	warnx("Error in units file '%s' line %d", UNITSFILE, linenum);
}


void 
readunits(const char *userfile)
{
	FILE *unitfile;
	char line[80], *lineptr;
	int len, linenum, i;

	unitcount = 0;
	linenum = 0;

	if (userfile) {
		unitfile = fopen(userfile, "rt");
		if (!unitfile)
			err(1, "Unable to open units file '%s'", userfile);
	}
	else {
		unitfile = fopen(UNITSFILE, "rt");
		if (!unitfile) {
			char *direc, *env;
			char filename[1000];
			char separator[2];

			env = getenv("PATH");
			if (env) {
				if (strchr(env, ';'))
					strlcpy(separator, ";",
					    sizeof(separator));
				else
					strlcpy(separator, ":",
					    sizeof(separator));
				direc = strtok(env, separator);
				while (direc) {
					strlcpy(filename, "", sizeof(filename));
					strlcat(filename, direc,
					    sizeof(filename));
					strlcat(filename, "/",
					    sizeof(filename));
					strlcat(filename, UNITSFILE,
					    sizeof(filename));
					unitfile = fopen(filename, "rt");
					if (unitfile)
						break;
					direc = strtok(NULL, separator);
				}
			}
			if (!unitfile)
				errx(1, "Can't find units file '%s'",
				    UNITSFILE);
		}
	}
	while (!feof(unitfile)) {
		if (!fgets(line, 79, unitfile))
			break;
		linenum++;
		lineptr = line;
		if (*lineptr == '/')
			continue;
		lineptr += strspn(lineptr, " \n\t");
		len = strcspn(lineptr, " \n\t");
		lineptr[len] = 0;
		if (!strlen(lineptr))
			continue;
		if (lineptr[strlen(lineptr) - 1] == '-') { /* it's a prefix */
			if (prefixcount == MAXPREFIXES) {
				warnx("Memory for prefixes exceeded in line %d",
				    linenum);
				continue;
			}
			lineptr[strlen(lineptr) - 1] = 0;
			prefixtable[prefixcount].prefixname = dupstr(lineptr);
			for (i = 0; i < prefixcount; i++)
				if (!strcmp(prefixtable[i].prefixname, lineptr)) {
					warnx(
			"Redefinition of prefix '%s' on line %d ignored",
					    lineptr, linenum);
					continue;
				}
			lineptr += len + 1;
			if (!strlen(lineptr)) {
				readerror(linenum);
				continue;
			}
			lineptr += strspn(lineptr, " \n\t");
			len = strcspn(lineptr, "\n\t");
			lineptr[len] = 0;
			prefixtable[prefixcount++].prefixval = dupstr(lineptr);
		}
		else {		/* it's not a prefix */
			if (unitcount == MAXUNITS) {
				warnx("Memory for units exceeded in line %d",
				    linenum);
				continue;
			}
			unittable[unitcount].uname = dupstr(lineptr);
			for (i = 0; i < unitcount; i++)
				if (!strcmp(unittable[i].uname, lineptr)) {
					warnx(
				"Redefinition of unit '%s' on line %d ignored",
					    lineptr, linenum);
					continue;
				}
			lineptr += len + 1;
			lineptr += strspn(lineptr, " \n\t");
			if (!strlen(lineptr)) {
				readerror(linenum);
				continue;
			}
			len = strcspn(lineptr, "\n\t");
			lineptr[len] = 0;
			unittable[unitcount++].uval = dupstr(lineptr);
		}
	}
	fclose(unitfile);
}

void 
initializeunit(struct unittype * theunit)
{
	theunit->factor = 1.0;
	theunit->numerator[0] = theunit->denominator[0] = NULL;
}


int 
addsubunit(const char *product[], const char *toadd)
{
	const char **ptr;

	for (ptr = product; *ptr && *ptr != NULLUNIT; ptr++);
	if (ptr >= product + MAXSUBUNITS) {
		warnx("Memory overflow in unit reduction");
		return 1;
	}
	if (!*ptr)
		*(ptr + 1) = 0;
	*ptr = dupstr(toadd);
	return 0;
}


void 
showunit(struct unittype * theunit)
{
	const char **ptr;
	int printedslash;
	int counter = 1;

	printf("\t%.8g", theunit->factor);
	for (ptr = theunit->numerator; *ptr; ptr++) {
		if (ptr > theunit->numerator && **ptr &&
		    !strcmp(*ptr, *(ptr - 1)))
			counter++;
		else {
			if (counter > 1)
				printf("%s%d", powerstring, counter);
			if (**ptr)
				printf(" %s", *ptr);
			counter = 1;
		}
	}
	if (counter > 1)
		printf("%s%d", powerstring, counter);
	counter = 1;
	printedslash = 0;
	for (ptr = theunit->denominator; *ptr; ptr++) {
		if (ptr > theunit->denominator && **ptr &&
		    !strcmp(*ptr, *(ptr - 1)))
			counter++;
		else {
			if (counter > 1)
				printf("%s%d", powerstring, counter);
			if (**ptr) {
				if (!printedslash)
					printf(" /");
				printedslash = 1;
				printf(" %s", *ptr);
			}
			counter = 1;
		}
	}
	if (counter > 1)
		printf("%s%d", powerstring, counter);
	printf("\n");
}


void 
zeroerror()
{
	warnx("Unit reduces to zero");
}

/*
   Adds the specified string to the unit.
   Flip is 0 for adding normally, 1 for adding reciprocal.

   Returns 0 for successful addition, nonzero on error.
*/

int 
addunit(struct unittype * theunit, const char *toadd, int flip)
{
	char *scratch, *savescr;
	char *item;
	char *divider, *slash;
	int doingtop;

	savescr = scratch = dupstr(toadd);
	for (slash = scratch + 1; *slash; slash++)
		if (*slash == '-' &&
		    (tolower((unsigned char)*(slash - 1)) != 'e' ||
		    !strchr(".0123456789", *(slash + 1))))
			*slash = ' ';
	slash = strchr(scratch, '/');
	if (slash)
		*slash = 0;
	doingtop = 1;
	do {
		item = strtok(scratch, " *\t\n/");
		while (item) {
			if (strchr("0123456789.", *item)) { /* item is a number */
				double num;

				divider = strchr(item, '|');
				if (divider) {
					*divider = 0;
					num = atof(item);
					if (!num) {
						zeroerror();
						return 1;
					}
					if (doingtop ^ flip)
						theunit->factor *= num;
					else
						theunit->factor /= num;
					num = atof(divider + 1);
					if (!num) {
						zeroerror();
						return 1;
					}
					if (doingtop ^ flip)
						theunit->factor /= num;
					else
						theunit->factor *= num;
				}
				else {
					num = atof(item);
					if (!num) {
						zeroerror();
						return 1;
					}
					if (doingtop ^ flip)
						theunit->factor *= num;
					else
						theunit->factor /= num;

				}
			}
			else {	/* item is not a number */
				int repeat = 1;

				if (strchr("23456789",
				    item[strlen(item) - 1])) {
					repeat = item[strlen(item) - 1] - '0';
					item[strlen(item) - 1] = 0;
				}
				for (; repeat; repeat--)
					if (addsubunit(doingtop ^ flip ? theunit->numerator : theunit->denominator, item))
						return 1;
			}
			item = strtok(NULL, " *\t/\n");
		}
		doingtop--;
		if (slash) {
			scratch = slash + 1;
		}
		else
			doingtop--;
	} while (doingtop >= 0);
	free(savescr);
	return 0;
}


int 
compare(const void *item1, const void *item2)
{
	return strcmp(*(const char * const *) item1,
		      *(const char * const *) item2);
}


void 
sortunit(struct unittype * theunit)
{
	const char **ptr;
	int count;

	for (count = 0, ptr = theunit->numerator; *ptr; ptr++, count++);
	qsort(theunit->numerator, count, sizeof(char *), compare);
	for (count = 0, ptr = theunit->denominator; *ptr; ptr++, count++);
	qsort(theunit->denominator, count, sizeof(char *), compare);
}


void 
cancelunit(struct unittype * theunit)
{
	const char **den, **num;
	int comp;

	den = theunit->denominator;
	num = theunit->numerator;

	while (*num && *den) {
		comp = strcmp(*den, *num);
		if (!comp) {
/*      if (*den!=NULLUNIT) free(*den);
      if (*num!=NULLUNIT) free(*num);*/
			*den++ = NULLUNIT;
			*num++ = NULLUNIT;
		}
		else if (comp < 0)
			den++;
		else
			num++;
	}
}




/*
   Looks up the definition for the specified unit.
   Returns a pointer to the definition or a null pointer
   if the specified unit does not appear in the units table.
*/

static char buffer[100];	/* buffer for lookupunit answers with
				   prefixes */

const char *
lookupunit(const char *unit)
{
	int i;
	char *copy;

	for (i = 0; i < unitcount; i++) {
		if (!strcmp(unittable[i].uname, unit))
			return unittable[i].uval;
	}

	if (unit[strlen(unit) - 1] == '^') {
		copy = dupstr(unit);
		copy[strlen(copy) - 1] = 0;
		for (i = 0; i < unitcount; i++) {
			if (!strcmp(unittable[i].uname, copy)) {
				strlcpy(buffer, copy, sizeof(buffer));
				free(copy);
				return buffer;
			}
		}
		free(copy);
	}
	if (unit[strlen(unit) - 1] == 's') {
		copy = dupstr(unit);
		copy[strlen(copy) - 1] = 0;
		for (i = 0; i < unitcount; i++) {
			if (!strcmp(unittable[i].uname, copy)) {
				strlcpy(buffer, copy, sizeof(buffer));
				free(copy);
				return buffer;
			}
		}
		if (copy[strlen(copy) - 1] == 'e') {
			copy[strlen(copy) - 1] = 0;
			for (i = 0; i < unitcount; i++) {
				if (!strcmp(unittable[i].uname, copy)) {
					strlcpy(buffer, copy, sizeof(buffer));
					free(copy);
					return buffer;
				}
			}
		}
		free(copy);
	}
	for (i = 0; i < prefixcount; i++) {
		if (!strncmp(prefixtable[i].prefixname, unit,
			strlen(prefixtable[i].prefixname))) {
			unit += strlen(prefixtable[i].prefixname);
			if (!strlen(unit) || lookupunit(unit)) {
				strlcpy(buffer, prefixtable[i].prefixval,
				    sizeof(buffer));
				strlcat(buffer, " ", sizeof(buffer));
				strlcat(buffer, unit, sizeof(buffer));
				return buffer;
			}
		}
	}
	return 0;
}



/*
   reduces a product of symbolic units to primitive units.
   The three low bits are used to return flags:

     bit 0 (1) set on if reductions were performed without error.
     bit 1 (2) set on if no reductions are performed.
     bit 2 (4) set on if an unknown unit is discovered.
*/


#define ERROR 4

int 
reduceproduct(struct unittype * theunit, int flip)
{

	const char *toadd;
	const char **product;
	int didsomething = 2;

	if (flip)
		product = theunit->denominator;
	else
		product = theunit->numerator;

	for (; *product; product++) {

		for (;;) {
			if (!strlen(*product))
				break;
			toadd = lookupunit(*product);
			if (!toadd) {
				printf("unknown unit '%s'\n", *product);
				return ERROR;
			}
			if (strchr(toadd, PRIMITIVECHAR))
				break;
			didsomething = 1;
			if (*product != NULLUNIT) {
				free(__UNCONST(*product));
				*product = NULLUNIT;
			}
			if (addunit(theunit, toadd, flip))
				return ERROR;
		}
	}
	return didsomething;
}


/*
   Reduces numerator and denominator of the specified unit.
   Returns 0 on success, or 1 on unknown unit error.
*/

int 
reduceunit(struct unittype * theunit)
{
	int ret;

	ret = 1;
	while (ret & 1) {
		ret = reduceproduct(theunit, 0) | reduceproduct(theunit, 1);
		if (ret & 4)
			return 1;
	}
	return 0;
}


int 
compareproducts(const char **one, const char **two)
{
	while (*one || *two) {
		if (!*one && *two != NULLUNIT)
			return 1;
		if (!*two && *one != NULLUNIT)
			return 1;
		if (*one == NULLUNIT)
			one++;
		else if (*two == NULLUNIT)
			two++;
		else if (*one && *two && strcmp(*one, *two))
			return 1;
		else
			one++, two++;
	}
	return 0;
}


/* Return zero if units are compatible, nonzero otherwise */

int 
compareunits(struct unittype * first, struct unittype * second)
{
	return
	compareproducts(first->numerator, second->numerator) ||
	compareproducts(first->denominator, second->denominator);
}

int 
compareunitsreciprocal(struct unittype * first, struct unittype * second)
{
	return
	compareproducts(first->numerator, second->denominator) ||
	compareproducts(first->denominator, second->numerator);
}


int 
completereduce(struct unittype * unit)
{
	if (reduceunit(unit))
		return 1;
	sortunit(unit);
	cancelunit(unit);
	return 0;
}


void 
showanswer(struct unittype * have, struct unittype * want)
{
	if (compareunits(have, want)) {
		if (compareunitsreciprocal(have, want)) {
			printf("conformability error\n");
			showunit(have);
			showunit(want);
		} else {
			printf("\treciprocal conversion\n");
			printf("\t* %.8g\n\t/ %.8g\n", 1 / (have->factor * want->factor),
			    want->factor * have->factor);
		}
	}
	else
		printf("\t* %.8g\n\t/ %.8g\n", have->factor / want->factor,
		    want->factor / have->factor);
}


void 
usage()
{
	fprintf(stderr,
	    "\nunits [-f unitsfile] [-q] [-v] [from-unit to-unit]\n");
	fprintf(stderr, "\n    -f specify units file\n");
	fprintf(stderr, "    -q suppress prompting (quiet)\n");
	fprintf(stderr, "    -v print version number\n");
	exit(3);
}


int
main(int argc, char **argv)
{

	struct unittype have, want;
	char havestr[81], wantstr[81];
	int optchar;
	const char *userfile = 0;
	int quiet = 0;

	while ((optchar = getopt(argc, argv, "vqf:")) != -1) {
		switch (optchar) {
		case 'f':
			userfile = optarg;
			break;
		case 'q':
			quiet = 1;
			break;
		case 'v':
			fprintf(stderr, "\n  units version %s  Copyright (c) 1993 by Adrian Mariano\n",
			    VERSION);
			fprintf(stderr, "                    This program may be freely distributed\n");
			usage();
		default:
			usage();
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 3 && argc != 2 && argc != 0)
		usage();

	readunits(userfile);

	if (argc == 3) {
		strlcpy(havestr, argv[0], sizeof(havestr));
		strlcat(havestr, " ", sizeof(havestr));
		strlcat(havestr, argv[1], sizeof(havestr));
		argc--;
		argv++;
		argv[0] = havestr;
	}

	if (argc == 2) {
		strlcpy(havestr, argv[0], sizeof(havestr));
		strlcpy(wantstr, argv[1], sizeof(wantstr));
		initializeunit(&have);
		addunit(&have, havestr, 0);
		completereduce(&have);
		initializeunit(&want);
		addunit(&want, wantstr, 0);
		completereduce(&want);
		showanswer(&have, &want);
	}
	else {
		if (!quiet)
			printf("%d units, %d prefixes\n\n", unitcount,
			    prefixcount);
		for (;;) {
			do {
				initializeunit(&have);
				if (!quiet)
					printf("You have: ");
				if (!fgets(havestr, 80, stdin)) {
					if (!quiet)
						putchar('\n');
					exit(0);
				}
			} while (addunit(&have, havestr, 0) ||
			    completereduce(&have));
			do {
				initializeunit(&want);
				if (!quiet)
					printf("You want: ");
				if (!fgets(wantstr, 80, stdin)) {
					if (!quiet)
						putchar('\n');
					exit(0);
				}
			} while (addunit(&want, wantstr, 0) ||
			    completereduce(&want));
			showanswer(&have, &want);
		}
	}
	return (0);
}
