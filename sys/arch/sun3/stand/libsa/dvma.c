
/*
 * The easiest way to deal with the need for DVMA mappings is
 * to just map the entire third megabyte of RAM into DVMA space.
 * That way, dvma_mapin can just compute the DVMA alias address,
 * and dvma_mapout does nothing.  Note that this assumes all
 * standalone programs stay in the range SA_MIN_VA .. SA_MAX_VA
 */

#include <sys/param.h>

#define	DVMA_BASE 0xFFf00000
#define DVMA_MAPLEN  0xE0000	/* 1 MB - 128K (save MONSHORTSEG) */

#define SA_MIN_VA	0x200000
#define SA_MAX_VA	(SA_MIN_VA + DVMA_MAPLEN)

void
dvma_init()
{
	int segva, dmava, sme;

	segva = SA_MIN_VA;
	dmava = DVMA_BASE;

	while (segva < SA_MAX_VA) {
		sme = get_segmap(segva);
		set_segmap(dmava, sme);
		segva += NBSG;
		dmava += NBSG;
	}
}

/* Convert a local address to a DVMA address. */
char *
dvma_mapin(char *addr, int len)
{
	int va = (int)addr;

	/* Make sure the address is in the DVMA map. */
	if ((va < SA_MIN_VA) || (va >= SA_MAX_VA))
		panic("dvma_mapin");

	va -= SA_MIN_VA;
	va += DVMA_BASE;

	return ((char *) va);
}

/* Convert a DVMA address to a local address. */
char *
dvma_mapout(char *addr, int len)
{
	int va = (int)addr;

	/* Make sure the address is in the DVMA map. */
	if ((va < DVMA_BASE) || (va >= (DVMA_BASE + DVMA_MAPLEN)))
		panic("dvma_mapout");

	va -= DVMA_BASE;
	va += SA_MIN_VA;

	return ((char *) va);
}

extern char *alloc(int len);
char *
dvma_alloc(int len)
{
	char *mem;

	mem = alloc(len);
	if (!mem)
		return(mem);
	return(dvma_mapin(mem, len));
}

extern void free(void *ptr, int len);
void
dvma_free(char *dvma, int len)
{
	char *mem;

	mem = dvma_mapout(dvma, len);
	if (mem)
		free(mem, len);
}
