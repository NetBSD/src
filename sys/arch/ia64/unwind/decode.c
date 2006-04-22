/*	$NetBSD: decode.c,v 1.1.6.2 2006/04/22 11:37:39 simonb Exp $	*/

/* Contributed to the NetBSD foundation by Cherry G. Mathew
 * This file contains routines to decode unwind descriptors into
 * easily an readable data structure ( unwind_desc )
 * This is the lowest layer of the unwind stack heirarchy.
 */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <ia64/unwind/decode.h>

/* Decode ULE128 string */

char *
unwind_decode_ule128(char *buf, unsigned long *val)
{
	int i = 0;

	val[0] = 0;
	do {
		val[0] += ((buf[i] & 0x7f) << (i * 7)); 

	}while((0x80 & buf[i++]) && (i < 9));

	if(i > 9) {
		printf("Warning: ULE128 won't fit in an unsigned long. decode aborted!!!\n");
		return 0;
	}

	buf+= i; 
	return buf;
}	


char *
unwind_decode_R1(char *buf, union unwind_desc *uwd)
{

	if(!IS_R1(buf[0])) return NULL;
	uwd->R1.r = ((buf[0] & 0x20) == 0x20); 
	uwd->R1.rlen = (buf[0] & 0x1f);
	buf++;
	return buf;
}

char *
unwind_decode_R2(char *buf, union unwind_desc *uwd)
{

	if(!IS_R2(buf[0])) return NULL;
	
	uwd->R2.mask = (((buf[0] & 0x07) << 1) | ( (buf[1] >> 7) & 0xff));
	uwd->R2.grsave = (buf[1] & 0x7f);

	buf += 2;
	buf = unwind_decode_ule128(buf, &uwd->R2.rlen);
	return buf;
}

char *
unwind_decode_R3(char *buf, union unwind_desc *uwd)
{

	if(!IS_R3(buf[0])) return NULL;

	uwd->R3.r = ((buf[0] & 0x03) == 0x01);

	buf++;
	buf = unwind_decode_ule128(buf, &uwd->R3.rlen);
	return buf;
}

char *
unwind_decode_P1(char *buf, union unwind_desc *uwd)
{


	if(!IS_P1(buf[0])) return NULL;

	uwd->P1.brmask = (buf[0] & 0x1f);
	buf++;
	return buf;
}
	
char *
unwind_decode_P2(char *buf, union unwind_desc *uwd)
{

	if(!IS_P2(buf[0])) return NULL;
	
	uwd->P2.brmask = (((buf[0] & 0x0f) << 1) | ( (buf[1] >> 7) & 0xff));
	uwd->P2.gr = (buf[1] & 0x7f);
	buf += 2;
	return buf;
}

char *
unwind_decode_P3(char *buf, union unwind_desc *uwd)
{

	if(!IS_P3(buf[0])) return NULL;
	
	uwd->P3.r = (((0x07 & buf[0]) << 1) | ((0x80 & buf[1]) >> 7));
	uwd->P3.grbr = (buf[1] & 0x7f);
	buf +=2;
	return buf;
}

char *
unwind_decode_P4(char *buf, union unwind_desc *uwd, vsize_t len)
{

	if(!IS_P4(buf[0])) return NULL;

	uwd->P4.imask = 0; /* XXX: Unimplemented */

	/* XXX: adjust buf for imask length on return!!! 
	 * don't know the length of imask here.
	 */
	buf += roundup(len << 1, 8);
	return buf;
}

char *
unwind_decode_P5(char *buf, union unwind_desc *uwd)
{

	if(!IS_P5(buf[0])) return NULL;

	uwd->P5.grmask = (buf[1] >> 4);
	uwd->P5.frmask = ((buf[1] & 0x0f << 16) | (buf[2] << 8) | buf[3]);
	buf += 4;
	return buf;
}

char *
unwind_decode_P6(char *buf, union unwind_desc *uwd)
{

	if(!IS_P6(buf[0])) return NULL;

	uwd->P6.r = ((buf[0] & 0x10) == 0x10);
	uwd->P6.rmask = (buf[0] & 0x0f);
	buf++;
	return buf;
}


char *
unwind_decode_P7(char *buf, union unwind_desc *uwd)
{
	
	if (!IS_P7(buf[0])) return NULL;

	uwd->P7.r = (buf[0] & 0x0f);

	buf++;

	buf = unwind_decode_ule128(buf, &uwd->P7.t);
	if (uwd->P7.r == 0) /* memstack_f */
		buf = unwind_decode_ule128(buf, &uwd->P7.size);
	return buf;
}

char *
unwind_decode_P8(char *buf, union unwind_desc *uwd)
{

	if(!IS_P8(buf[0])) return NULL;

	uwd->P8.r = buf[1];

	buf +=2;
	buf = unwind_decode_ule128(buf, &uwd->P8.t);
	return buf;
}

char *
unwind_decode_P9(char *buf, union unwind_desc *uwd)
{


	if(!IS_P9(buf[0])) return NULL;

	uwd->P9.grmask = buf[1] & 0x0f;
	uwd->P9.gr = buf[2] & 0x7f;
	buf += 3;
	return buf;
}


char *
unwind_decode_P10(char *buf, union unwind_desc *uwd)
{


	if(!IS_P10(buf[0])) return NULL;

	uwd->P10.abi = buf[1];
	uwd->P10.context = buf[2];
	buf += 3;
	return buf;
}

char *
unwind_decode_B1(char *buf, union unwind_desc *uwd)
{


	if(!IS_B1(buf[0])) return NULL;

	uwd->B1.r = ((buf[0] & 0x20) == 0x20);
	uwd->B1.label = (buf[0] & 0x1f);

	buf++;
	return buf;
}

char *
unwind_decode_B2(char *buf, union unwind_desc *uwd)
{


	if(!IS_B2(buf[0])) return NULL;

	uwd->B2.ecount = (buf[0] & 0x1f);

	buf++;
	buf = unwind_decode_ule128(buf, &uwd->B2.t);
	return buf;
}

char *
unwind_decode_B3(char *buf, union unwind_desc *uwd)
{

	
	if(!IS_B3(buf[0])) return NULL;

	buf++;
	buf = unwind_decode_ule128(buf, &uwd->B3.t);
	buf = unwind_decode_ule128(buf, &uwd->B3.ecount);
	return buf;
}

char *
unwind_decode_B4(char *buf, union unwind_desc *uwd)
{


	if(!IS_B4(buf[0])) return NULL;

	uwd->B4.r = ((buf[0] & 0x08) == 0x08);

	buf++;
	buf = unwind_decode_ule128(buf, &uwd->B4.label);
	return buf;
}


char *
unwind_decode_X1(char *buf, union unwind_desc *uwd)
{


	if(!IS_X1(buf[0])) return NULL;

	uwd->X1.r = ((buf[1] & 0x80) == 0x80);
	uwd->X1.a = ((buf[1] & 0x40) == 0x40);
	uwd->X1.b = ((buf[1] & 0x20) == 0x20);
	uwd->X1.reg = (buf[1] & 0x1f);

	buf += 2;
	buf = unwind_decode_ule128(buf, &uwd->X1.t);
	buf = unwind_decode_ule128(buf, &uwd->X1.offset);
	return buf;
}


char *
unwind_decode_X2(char *buf, union unwind_desc *uwd)
{


	if(!IS_X2(buf[0])) return NULL;

	uwd->X2.x = ((buf[1] & 0x80) == 0x80);
	uwd->X2.a = ((buf[1] & 0x40) == 0x40);
	uwd->X2.b = ((buf[1] & 0x20) == 0x20);
	uwd->X2.reg = (buf[1] & 0x1f);
	uwd->X2.y = ((buf[2] & 0x80) == 0x80);
	uwd->X2.treg = (buf[2] & 0x7f);

	buf += 3;
	buf = unwind_decode_ule128(buf, &uwd->X2.t);
	return buf;
}

char *
unwind_decode_X3(char *buf, union unwind_desc *uwd)
{


	if(!IS_X3(buf[0])) return NULL;

	uwd->X3.r = ((buf[1] & 0x80) == 0x80);
	uwd->X3.qp = (buf[1] & 0x3f);
	uwd->X3.a = ((buf[1] & 0x40) == 0x40);
	uwd->X3.b = ((buf[1] & 0x20) == 0x20);
	uwd->X3.reg = (buf[1] & 0x1f);

	buf += 3;
	buf = unwind_decode_ule128(buf, &uwd->X3.t);
	buf = unwind_decode_ule128(buf, &uwd->X3.offset );
	return buf;
}

char *
unwind_decode_X4(char *buf, union unwind_desc *uwd)
{


	if(!IS_X4(buf[0])) return NULL;

	uwd->X4.qp = (buf[1] & 0x3f);
	uwd->X4.x = ((buf[2] & 0x80) == 0x80);
	uwd->X4.a = ((buf[2] & 0x40) == 0x40);
	uwd->X4.b = ((buf[2] & 0x20) == 0x20);
	uwd->X4.reg = (buf[2] & 0x1f);
	uwd->X4.y = ((buf[3] & 0x80) == 0x80);
	uwd->X4.treg = (buf[3] & 0x7f);

	buf +=4;
	buf = unwind_decode_ule128(buf, &uwd->X4.t);
	return buf;
}
	
