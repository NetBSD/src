/*	$NetBSD: h_match.c,v 1.1 2026/11/16 17:33:45 christos Exp $	*/

/*
 * From PR/61017
 *
 * usage: h_match pattern hexstring rm_so icase|case [edge]
 * Compiles pattern with REG_EXTENDED|REG_GNU (and REG_ICASE for "icase")
 * under LC_CTYPE=C.UTF-8 and runs regexec(REG_STARTEND) on the string
 * from rm_so to its end.  With "edge" the string ends at an unmapped page.
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: h_match.c,v 1.1 2026/11/16 17:33:45 christos Exp $");

#include <sys/mman.h>
#include <locale.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	char tmp[256], *s;
	size_t n, i, ps = (size_t)sysconf(_SC_PAGESIZE);
	regex_t re;
	regmatch_t m;
	int rc, cfl = REG_EXTENDED | REG_GNU;
	unsigned v;

	if (argc < 5 || setlocale(LC_CTYPE, "C.UTF-8") == NULL)
		return 2;
	if (strcmp(argv[4], "icase") == 0)
		cfl |= REG_ICASE;
	for (n = 0, i = 0; argv[2][i] && argv[2][i + 1]; i += 2, n++) {
		sscanf(argv[2] + i, "%2x", &v);
		tmp[n] = (char)v;
	}
	s = mmap(NULL, 2 * ps, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE,
	    -1, 0);
	if (argc > 5 && strcmp(argv[5], "edge") == 0)
		mprotect(s + ps, ps, PROT_NONE);	/* s[n] is unmapped */
	else
		s[ps] = '\0';
	s += ps - n;
	memcpy(s, tmp, n);
	if (regcomp(&re, argv[1], cfl) != 0)
		return 3;
	m.rm_so = atoi(argv[3]);
	m.rm_eo = (regoff_t)n;
	rc = regexec(&re, s, 1, &m, REG_STARTEND);
	printf("rc=%d rm_so=%jd rm_eo=%jd (string is %zu bytes)\n", rc,
	    (intmax_t)m.rm_so, (intmax_t)m.rm_eo, n);
	return 0;
}
