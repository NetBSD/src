/*	$NetBSD: sysctlutil.c,v 1.6 2004/03/24 16:34:34 atatat Exp $ */

#include <sys/param.h>
#define __USE_NEW_SYSCTL
#include <sys/sysctl.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef _REENTRANT
#include "reentrant.h"
#endif /* _REENTRANT */

/*
 * the place where we attach stuff we learn on the fly, not
 * necessarily used.
 */
static struct sysctlnode sysctl_mibroot = {
#if defined(lint)
	/*
	 * lint doesn't like my initializers
	 */
	0
#else /* !lint */
	.sysctl_flags = SYSCTL_VERSION|CTLFLAG_ROOT|CTLTYPE_NODE,
	.sysctl_size = sizeof(struct sysctlnode),
	.sysctl_name = "(root)",
#endif /* !lint */
};

/*
 * routines to handle learning and cleanup
 */
static int compar(const void *, const void *);
int learn_tree(int *, u_int, struct sysctlnode *);
static void free_children(struct sysctlnode *);
static void relearnhead(void);
int sysctlnametomib(const char *, int *, size_t *);
int sysctlbyname(const char *, void *, size_t *, void *, size_t);
int sysctlgetmibinfo(const char *, int *, u_int *,
		     char *, size_t *, struct sysctlnode **, int);

/*
 * for ordering nodes -- a query may or may not be given them in
 * numeric order
 */
static int
compar(const void *a, const void *b)
{

	return (((const struct sysctlnode *)a)->sysctl_num -
		((const struct sysctlnode *)b)->sysctl_num);
}

/*
 * sucks in the children at a given level and attaches it to the tree.
 */
int
learn_tree(int *name, u_int namelen, struct sysctlnode *pnode)
{
	struct sysctlnode qnode;
	int rc;
	size_t sz;

	if (pnode == NULL)
		pnode = &sysctl_mibroot;
	if (SYSCTL_TYPE(pnode->sysctl_flags) != CTLTYPE_NODE) {
		errno = EINVAL;
		return (-1);
	}
	if (pnode->sysctl_child != NULL)
		return (0);

	if (pnode->sysctl_clen == 0)
		sz = SYSCTL_DEFSIZE * sizeof(struct sysctlnode);
	else
		sz = pnode->sysctl_clen * sizeof(struct sysctlnode);
	pnode->sysctl_child = malloc(sz);
	if (pnode->sysctl_child == NULL)
		return (-1);

	name[namelen] = CTL_QUERY;
	pnode->sysctl_clen = 0;
	pnode->sysctl_csize = 0;
	memset(&qnode, 0, sizeof(qnode));
	qnode.sysctl_flags = SYSCTL_VERSION;
	rc = sysctl(name, namelen + 1, pnode->sysctl_child, &sz,
		    &qnode, sizeof(qnode));
	if (sz == 0) {
		free(pnode->sysctl_child);
		pnode->sysctl_child = NULL;
		return (rc);
	}
	if (rc) {
		free(pnode->sysctl_child);
		pnode->sysctl_child = NULL;
		if ((sz % sizeof(struct sysctlnode)) != 0)
			errno = EINVAL;
		if (errno != ENOMEM)
			return (rc);
	}

	if (pnode->sysctl_child == NULL) {
		pnode->sysctl_child = malloc(sz);
		if (pnode->sysctl_child == NULL)
			return (-1);

		rc = sysctl(name, namelen + 1, pnode->sysctl_child, &sz,
			    &qnode, sizeof(qnode));
		if (rc) {
			free(pnode->sysctl_child);
			pnode->sysctl_child = NULL;
			return (rc);
		}
	}

	/*
	 * how many did we get?
	 */
	pnode->sysctl_clen = sz / sizeof(struct sysctlnode);
	pnode->sysctl_csize = sz / sizeof(struct sysctlnode);
	if (pnode->sysctl_clen * sizeof(struct sysctlnode) != sz) {
		free(pnode->sysctl_child);
		pnode->sysctl_child = NULL;
		errno = EINVAL;
		return (-1);
	}

	/*
	 * you know, the kernel doesn't really keep them in any
	 * particular order...just like entries in a directory
	 */
	qsort(pnode->sysctl_child, pnode->sysctl_clen, 
	    sizeof(struct sysctlnode), compar);

	/*
	 * rearrange parent<->child linkage
	 */
	for (rc = 0; rc < pnode->sysctl_clen; rc++) {
		pnode->sysctl_child[rc].sysctl_parent = pnode;
		if (SYSCTL_TYPE(pnode->sysctl_child[rc].sysctl_flags) ==
		    CTLTYPE_NODE) {
			/*
			 * these nodes may have children, but we
			 * haven't discovered that yet.
			 */
			pnode->sysctl_child[rc].sysctl_child = NULL;
		}
	}

	return (0);
}

/*
 * recursively nukes a branch or an entire tree from the given node
 */
static void
free_children(struct sysctlnode *rnode) 
{
	struct sysctlnode *node;

	if (rnode == NULL ||
	    SYSCTL_TYPE(rnode->sysctl_flags) != CTLTYPE_NODE ||
	    rnode->sysctl_child == NULL)
		return;

	for (node = rnode->sysctl_child;
	     node < &rnode->sysctl_child[rnode->sysctl_clen];
	     node++) {
		free_children(node);
	}
	free(rnode->sysctl_child);
	rnode->sysctl_child = NULL;
}

/*
 * verifies that the head of the tree in the kernel is the same as the
 * head of the tree we already got, integrating new stuff and removing
 * old stuff, if it's not.
 */
static void
relearnhead(void)
{
	struct sysctlnode *h, *i, *o, qnode;
	size_t si, so;
	int rc, name, nlen, olen, ni, oi, t;

	/*
	 * if there's nothing there, there's no need to expend any
	 * effort
	 */
	if (sysctl_mibroot.sysctl_child == NULL)
		return;

	/*
	 * attempt to pull out the head of the tree, starting with the
	 * size we have now, and looping if we need more (or less)
	 * space
	 */
	si = 0;
	so = sysctl_mibroot.sysctl_clen * sizeof(struct sysctlnode);
	name = CTL_QUERY;
	memset(&qnode, 0, sizeof(qnode));
	qnode.sysctl_flags = SYSCTL_VERSION;
	do {
		si = so;
		h = malloc(si);
		rc = sysctl(&name, 1, h, &so, &qnode, sizeof(qnode));
		if (rc == -1 && errno != ENOMEM)
			return;
		if (si < so)
			free(h);
	} while (si < so);

	/*
	 * order the new copy of the head
	 */
	nlen = so / sizeof(struct sysctlnode);
	qsort(h, (size_t)nlen, sizeof(struct sysctlnode), compar);

	/*
	 * verify that everything is the same.  if it is, we don't
	 * need to do any more work here.
	 */
	olen = sysctl_mibroot.sysctl_clen;
	rc = (nlen == olen) ? 0 : 1;
	o = sysctl_mibroot.sysctl_child;
	for (ni = 0; rc == 0 && ni < nlen; ni++) {
		if (h[ni].sysctl_num != o[ni].sysctl_num ||
		    h[ni].sysctl_ver != o[ni].sysctl_ver)
			rc = 1;
	}
	if (rc == 0) {
		free(h);
		return;
	}

	/*
	 * something changed.  h will become the new head, and we need
	 * pull over any subtrees we already have if they're the same
	 * version.
	 */
	i = h;
	ni = oi = 0;
	while (ni < nlen && oi < olen) {
		/*
		 * something was inserted or deleted
		 */
		if (SYSCTL_TYPE(i[ni].sysctl_flags) == CTLTYPE_NODE)
			i[ni].sysctl_child = NULL;
		if (i[ni].sysctl_num != o[oi].sysctl_num) {
			if (i[ni].sysctl_num < o[oi].sysctl_num) {
				ni++;
			}
			else {
				free_children(&o[oi]);
				oi++;
			}
			continue;
		}

		/*
		 * same number, but different version, so throw away
		 * any accumulated children
		 */
		if (i[ni].sysctl_ver != o[oi].sysctl_ver)
			free_children(&o[oi]);

		/*
		 * this node is the same, but we only need to
		 * move subtrees.
		 */
		else if (SYSCTL_TYPE(i[ni].sysctl_flags) == CTLTYPE_NODE) {	
			/*
			 * move subtree to new parent
			 */
			i[ni].sysctl_clen = o[oi].sysctl_clen;
			i[ni].sysctl_csize = o[oi].sysctl_csize;
			i[ni].sysctl_child = o[oi].sysctl_child;
			/*
			 * reparent inherited subtree
			 */
			for (t = 0;
			     i[ni].sysctl_child != NULL &&
				     t < i[ni].sysctl_clen;
			     t++)
				i[ni].sysctl_child[t].sysctl_parent = &i[ni];
		}
		ni++;
		oi++;
	}

	/*
	 * left over new nodes need to have empty subtrees cleared
	 */
	while (ni < nlen) {
		if (SYSCTL_TYPE(i[ni].sysctl_flags) == CTLTYPE_NODE)
			i[ni].sysctl_child = NULL;
		ni++;
	}

	/*
	 * left over old nodes need to be cleaned out
	 */
	while (oi < olen) {
		free_children(&o[oi]);
		oi++;
	}

	/*
	 * pop new head in
	 */
	sysctl_mibroot.sysctl_clen = nlen;
	sysctl_mibroot.sysctl_csize = nlen;
	sysctl_mibroot.sysctl_child = h;
	free(o);
}

/*
 * freebsd compatible sysctlnametomib() function, implemented as an
 * extremely thin wrapper around sysctlgetmibinfo().  i think the use
 * of size_t as the third argument is erroneous, but what can we do
 * about that?
 */
int
sysctlnametomib(const char *gname, int *iname, size_t *namelenp)
{
	u_int unamelen;
	int rc;

	unamelen = *namelenp;
	rc = sysctlgetmibinfo(gname, iname, &unamelen, NULL, NULL, NULL,
			      SYSCTL_VERSION);
	*namelenp = unamelen;

	return (rc);
}

/*
 * trivial sysctlbyname() function for the "lazy".
 */
int
sysctlbyname(const char *gname, void *oldp, size_t *oldlenp, void *newp,
	     size_t newlen)
{
	int name[CTL_MAXNAME], rc;
	u_int namelen;

	rc = sysctlgetmibinfo(gname, &name[0], &namelen, NULL, NULL, NULL,
			      SYSCTL_VERSION);
	if (rc == 0)
		rc = sysctl(&name[0], namelen, oldp, oldlenp, newp, newlen);
	return (rc);
}

/*
 * that's "given name" as a string, the integer form of the name fit
 * to be passed to sysctl(), "canonicalized name" (optional), and a
 * pointer to the length of the integer form.  oh, and then a pointer
 * to the node, in case you (the caller) care.  you can leave them all
 * NULL except for gname, though that might be rather pointless,
 * unless all you wanna do is verify that a given name is acceptable.
 *
 * returns either 0 (everything was fine) or -1 and sets errno
 * accordingly.  if errno is set to EAGAIN, we detected a change to
 * the mib while parsing, and you should try again.  in the case of an
 * invalid node name, cname will be set to contain the offending name.
 */
#ifdef _REENTRANT
static mutex_t sysctl_mutex = MUTEX_INITIALIZER;
static int sysctlgetmibinfo_unlocked(const char *, int *, u_int *, char *,
				     size_t *, struct sysctlnode **, int);
#endif /* __REENTRANT */

int
sysctlgetmibinfo(const char *gname, int *iname, u_int *namelenp,
		 char *cname, size_t *csz, struct sysctlnode **rnode, int v)
#ifdef _REENTRANT
{
	int rc;

	mutex_lock(&sysctl_mutex);
	rc = sysctlgetmibinfo_unlocked(gname, iname, namelenp, cname, csz,
				       rnode, v);
	mutex_unlock(&sysctl_mutex);

	return (rc);
}

static int
sysctlgetmibinfo_unlocked(const char *gname, int *iname, u_int *namelenp,
			  char *cname, size_t *csz, struct sysctlnode **rnode,
			  int v)
#endif /* _REENTRANT */
{
	struct sysctlnode *pnode, *node;
	int name[CTL_MAXNAME], ni, n, haven;
	u_int nl;
	quad_t q;
	char sep[2], token[SYSCTL_NAMELEN],
		pname[SYSCTL_NAMELEN * CTL_MAXNAME + CTL_MAXNAME];
	const char *piece, *dot;
	char *t;
	size_t l;

	if (rnode != NULL) {
		if (*rnode == NULL && v != SYSCTL_VERSION)
			/* XXX later deal with dealing back a sub version */
			return (EINVAL);
		if (SYSCTL_VERS((*rnode)->sysctl_flags) != SYSCTL_VERSION)
			/* XXX later deal with other people's trees */
			return (EINVAL);
	}
	if (rnode == NULL || *rnode == NULL)
		pnode = &sysctl_mibroot;
	else
		pnode = *rnode;

	if (sysctl_rootof(pnode) == &sysctl_mibroot)
		relearnhead();

	nl = ni = 0;
	token[0] = '\0';
	pname[0] = '\0';
	node = NULL;

	/*
	 * default to using '.' as the separator, but allow '/' as
	 * well, and then allow a leading separator
	 */
	if ((dot = strpbrk(gname, "./")) == NULL)
		sep[0] = '.';
	else
		sep[0] = dot[0];
	sep[1] = '\0';
	if (gname[0] == sep[0]) {
		strlcat(pname, sep, sizeof(pname));
		gname++;
	}

#define COPY_OUT_DATA(t, c, cs, nlp, l) do {			\
		if ((c) != NULL && (cs) != NULL)		\
			*(cs) = strlcpy((c), (t), *(cs));	\
		else if ((cs) != NULL)				\
			*(cs) = strlen(t) + 1;			\
		if ((nlp) != NULL)				\
			*(nlp) = (l);				\
	} while (/*CONSTCOND*/0)

	piece = gname;
	while (piece != NULL && *piece != '\0') {
		/*
		 * what was i looking for?
		 */
		dot = strchr(piece, sep[0]);
		if (dot == NULL) {
			l = strlcpy(token, piece, sizeof(token));
			if (l > sizeof(token)) {
				COPY_OUT_DATA(piece, cname, csz, namelenp, nl);
				errno = ENAMETOOLONG;
				return (-1);
			}
		}
		else if (dot - piece > sizeof(token) - 1) {
			COPY_OUT_DATA(token, cname, csz, namelenp, nl);
			errno = ENAMETOOLONG;
			return (-1);
		}
		else {
			strncpy(token, piece, (size_t)(dot - piece));
			token[dot - piece] = '\0';
		}

		/*
		 * i wonder if this "token" is an integer?
		 */
		errno = 0;
		q = strtoq(token, &t, 0);
		n = (int)q;
		if (errno != 0 || *t != '\0')
			haven = 0;
		else if (q < INT_MIN || q > UINT_MAX)
			haven = 0;
		else
			haven = 1;

		/*
		 * make sure i have something to look at
		 */
		if (SYSCTL_TYPE(pnode->sysctl_flags) != CTLTYPE_NODE) {
			if (haven && nl > 0) {
				strlcat(pname, sep, sizeof(pname));
				goto just_numbers;
			}
			COPY_OUT_DATA(token, cname, csz, namelenp, nl);
			errno = ENOTDIR;
			return (-1);
		}
		if (pnode->sysctl_child == NULL) {
			if (learn_tree(name, nl, pnode) == -1) {
				COPY_OUT_DATA(token, cname, csz, namelenp, nl);
				return (-1);
			}
		}
		node = pnode->sysctl_child;
		if (node == NULL) {
			COPY_OUT_DATA(token, cname, csz, namelenp, nl);
			errno = ENOENT;
			return (-1);
		}

		/*
		 * now...is it there?
		 */
		for (ni = 0; ni < pnode->sysctl_clen; ni++)
			if ((haven && ((n == node[ni].sysctl_num) ||
			    (node[ni].sysctl_flags & CTLFLAG_ANYNUMBER))) ||
			    strcmp(token, node[ni].sysctl_name) == 0)
				break;
		if (ni >= pnode->sysctl_clen) {
			COPY_OUT_DATA(token, cname, csz, namelenp, nl);
			errno = ENOENT;
			return (-1);
		}

		/*
		 * ah...it is.
		 */
		pnode = &node[ni];
		if (nl > 0)
			strlcat(pname, sep, sizeof(pname));
		if (haven && n != pnode->sysctl_num) {
 just_numbers:
			strlcat(pname, token, sizeof(pname));
			name[nl] = n;
		}
		else {
			strlcat(pname, pnode->sysctl_name, sizeof(pname));
			name[nl] = pnode->sysctl_num;
		}
		piece = (dot != NULL) ? dot + 1 : NULL;
		nl++;
		if (nl == CTL_MAXNAME) {
			COPY_OUT_DATA(token, cname, csz, namelenp, nl);
			errno = ERANGE;
			return (-1);
		}
	}

	if (nl == 0) {
		if (namelenp != NULL)
			*namelenp = 0;
		errno = EINVAL;
		return (-1);
	}

	COPY_OUT_DATA(pname, cname, csz, namelenp, nl);
	if (iname != NULL && namelenp != NULL)
		memcpy(iname, &name[0], MIN(nl, *namelenp) * sizeof(int));
	if (namelenp != NULL)
		*namelenp = nl;
	if (rnode != NULL) {
		if (*rnode != NULL)
			/*
			 * they gave us a private tree to work in, so
			 * we give back a pointer into that private
			 * tree
			 */
			*rnode = pnode;
		else {
			/*
			 * they gave us a place to put the node data,
			 * so give them a copy
			 */
			*rnode = malloc(sizeof(struct sysctlnode));
			if (*rnode != NULL) {
				**rnode = *pnode;
				(*rnode)->sysctl_child = NULL;
				(*rnode)->sysctl_parent = NULL;
			}
		}
	}

	return (0);
}
