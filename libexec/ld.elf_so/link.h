/*	$NetBSD: link.h,v 1.1.2.2 1998/10/30 18:26:25 cgd Exp $	*/

/*
 * This only exists for GDB.
 */
#ifndef _LINK_H
#define	_LINK_H

#include <sys/types.h>

#include <machine/elf_machdep.h>

struct link_map {
	caddr_t		l_addr;			/* Base Address of library */
#ifdef __mips__
	caddr_t		l_offs;			/* Load Offset of library */
#endif
	const char	*l_name;		/* Absolute Path to Library */
	void		*l_ld;			/* Pointer to .dynamic in memory */
	struct link_map	*l_next, *l_prev;	/* linked list of of mapped libs */
};

struct r_debug {
	int		r_version;		/* not used */
	struct link_map *r_map;			/* list of loaded images */
	void		(*r_brk)(void);		/* pointer to break point */
	enum {
	    RT_CONSISTENT,			/* things are stable */
	    RT_ADD,				/* adding a shared library */
	    RT_DELETE				/* removing a shared library */
	}		r_state;
};
#endif	/* _LINK_H */

