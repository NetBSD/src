/*	$NetBSD: rijndael_local.h,v 1.3 2003/07/15 11:00:43 itojun Exp $	*/
/*	$KAME: rijndael_local.h,v 1.4 2003/07/15 10:47:16 itojun Exp $	*/

/* the file should not be used from outside */
typedef u_int8_t		u8;
typedef u_int16_t		u16;	
typedef u_int32_t		u32;

#define MAXKC RIJNDAEL_MAXKC	(256/32)
#define MAXKB RIJNDAEL_MAXKB	(256/8)
#define MAXNR RIJNDAEL_MAXNR	14
