/*	$NetBSD: initfini.c,v 1.1 2001/07/17 03:41:03 thorpej Exp $	*/

/*
 * This file placed in the public domain.
 * Jason R. Thorpe, July 16, 2001.
 */

#include <sys/types.h>
#include <unistd.h>

void	i_am_init(void);
void	i_am_fini(void);

int	main(int, char *[]);

#define	WRITE(str)							\
	write(STDOUT_FILENO, str, sizeof(str) - 1)

int
main(int argc, char *argv[])
{

	WRITE("I am main.\n");
	exit(0);
}

void
i_am_init(void)
{

	WRITE("I am init.\n");
}

void
i_am_fini(void)
{

	WRITE("I am fini.\n");
}
