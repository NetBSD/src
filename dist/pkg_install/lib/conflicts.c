/*
 * XXX: Reading the +CONTENTS files of all installed packages is
 * rather slow. Since this check is necessary to avoid conflicting
 * packages, it should not be removed.
 *
 * TODO: Put all the information that is currently in the +CONTENTS
 * files into one large file or another database.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <nbcompat.h>

#if HAVE_ERR_H
#include <err.h>
#endif

#include "dewey.h"
#include "lib.h"

/**
 * Data structure to keep the intermediate result of the conflict
 * search. ''pkgname'' is the package in question. The first
 * installed package that conflicts is filled into
 * ''conflicting_pkgname''. The pattern that leads to the conflict is
 * also filled in to help the user in deciding what to do with the
 * conflict.
 */
struct package_conflict {
	const char *pkgname;
	char **conflicting_pkgname;
	char **conflicting_pattern;
};

static void *
nonnull(void *p)
{

	if (p == NULL) {
		err(EXIT_FAILURE, "NullPointerException");
		/* NOTREACHED */
	}
	return p;
}

static FILE *
fopen_contents(const char *pkgname, const char *mode)
{
	char fname[MaxPathSize];
	FILE *f;

	snprintf(fname, sizeof(fname), "%s/%s/%s", _pkgdb_getPKGDB_DIR(), pkgname, CONTENTS_FNAME);
	f = fopen(fname, mode);
	if (f == NULL) {
		err(EXIT_FAILURE, "%s", fname);
		/* NOTREACHED */
	}
	return f;
}


static int
check_package_conflict(const char *pkgname, void *v)
{
	struct package_conflict *conflict = v;
	package_t pkg;
	plist_t *p;
	FILE *f;
	int rv;

	rv = 0;
	pkg.head = NULL;
	pkg.tail = NULL;

	f = fopen_contents(pkgname, "r");
	read_plist(&pkg, f);
	(void)fclose(f);

	for (p = pkg.head; p; p = p->next) {
		if (p->type != PLIST_PKGCFL)
			continue;

		if (pkg_match(p->name, conflict->pkgname) == 1) {
			*(conflict->conflicting_pkgname) = nonnull(strdup(pkgname));
			*(conflict->conflicting_pattern) = nonnull(strdup(p->name));
			rv = 1 /* nonzero, stop iterating */;
			break;
		}
	}

	free_plist(&pkg);
	return rv;
}

/**
 * Checks if some installed package has a pkgcfl entry that matches
 * PkgName.  If such an entry is found, the package name is returned in
 * inst_pkgname, the matching pattern in inst_pattern, and the function
 * returns a non-zero value. Otherwise, zero is returned and the result
 * variables are set to NULL.
 */
int
some_installed_package_conflicts_with(const char *pkgname, char **inst_pkgname, char **inst_pattern)
{
	struct package_conflict cfl;
	int rv;

	cfl.pkgname = pkgname;
	*inst_pkgname = NULL;
	*inst_pattern = NULL;
	cfl.conflicting_pkgname = inst_pkgname;
	cfl.conflicting_pattern = inst_pattern;
	rv = iterate_pkg_db(check_package_conflict, &cfl);
	if (rv == -1) {
		errx(EXIT_FAILURE, "Couldn't read list of installed packages.");
		/* NOTREACHED */
	}
	return *inst_pkgname != NULL;
}

#if 0
int main(int argc, char **argv)
{
	char *pkg, *patt;

	if (some_installed_package_conflicts_with(argv[1], &pkg, &patt))
		printf("yes: package %s conflicts with %s, pattern %s\n", pkg, argv[1], patt);
	else
		printf("no\n");
	return 0;
}
void cleanup(int i) {}
#endif
