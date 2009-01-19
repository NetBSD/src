/*	$NetBSD: gemini_ipmvar.h,v 1.1.6.2 2009/01/19 13:15:58 skrll Exp $	*/

#ifndef _GEMINI_IPMVAR_H_
#define _GEMINI_IPMVAR_H_

/*
 * message queue
 *
 * - the queue gets located in memory shared between cores
 * - is mapped non-cached so SW coherency is not required.
 * - be sure ipm_queue_t starts on 32 bit (min) boundary to align descriptors
 * - note that indicies are 8 bit and NIPMDESC < (1<<8)
 *   be sure to adjust typedef if size is increased
 * - current sizes, typedef, and padding make sizeof(ipm_queue_t) == 4096
 */
typedef uint32_t ipmqindex_t;
#define NIPMDESC	255
#define IPMQPADSZ	(4096 - ((sizeof(ipm_desc_t) * NIPMDESC) + (2 * sizeof(ipmqindex_t))))
typedef struct ipm_queue {
	ipm_desc_t ipm_desc[NIPMDESC];
	volatile ipmqindex_t ix_write; /* writer increments and inserts here  */
	volatile ipmqindex_t ix_read;  /* reader extracts here and increments */
	uint8_t pad[IPMQPADSZ];
} ipm_queue_t;

static inline ipmqindex_t
ipmqnext(ipmqindex_t ix)
{
	if (++ix >= NIPMDESC)
		ix = 0;
	return ix;
}

static inline bool
ipmqisempty(ipmqindex_t ixr, ipmqindex_t ixw)
{
	if (ixr == ixw)
		return TRUE;
	return FALSE;
}

static inline bool
ipmqisfull(ipmqindex_t ixr, ipmqindex_t ixw)
{
	if (ipmqnext(ixw) == ixr)
		return TRUE;
	return FALSE;
}

#endif	/* _GEMINI_IPMVAR_H_ */
