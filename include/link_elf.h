/*	$NetBSD: link_elf.h,v 1.8 2009/11/04 19:28:03 pooka Exp $	*/

/*
 * This only exists for GDB.
 */

#ifndef _LINK_ELF_H_
#define	_LINK_ELF_H_

#include <sys/types.h>

#include <machine/elf_machdep.h>

typedef struct link_map {
	caddr_t		 l_addr;	/* Base Address of library */
#ifdef __mips__
	caddr_t		 l_offs;	/* Load Offset of library */
#endif
	const char	*l_name;	/* Absolute Path to Library */
	void		*l_ld;		/* Pointer to .dynamic in memory */
	struct link_map	*l_next;	/* linked list of of mapped libs */
	struct link_map *l_prev;
} Link_map;

struct r_debug {
	int r_version;			/* not used */
	struct link_map *r_map;		/* list of loaded images */
	void (*r_brk)(void);		/* pointer to break point */
	enum {
		RT_CONSISTENT,		/* things are stable */
		RT_ADD,			/* adding a shared library */
		RT_DELETE		/* removing a shared library */
	} r_state;
};

#endif	/* _LINK_ELF_H_ */
