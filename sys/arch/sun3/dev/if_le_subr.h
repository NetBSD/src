/* access LANCE registers */

#define	LERDWR(cntl, src, dst) (dst) = (src)

#define LANCE_ADDR(lance) ((unsigned int) lance & 0x00FFFFFF)

#define le_machdep_intrcheck(le, unit) 
