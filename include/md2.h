/*	$NetBSD: md2.h,v 1.1 2001/03/19 04:18:53 atatat Exp $	*/

#ifndef _MD2_H_
#define _MD2_H_

#include <sys/cdefs.h>
#include <sys/types.h>

/* MD2 context. */
typedef struct MD2Context {
	u_int32_t i;
	unsigned char C[16];		/* checksum */
	unsigned char X[48];		/* input buffer */
} MD2_CTX;

__BEGIN_DECLS
void	MD2Init __P((MD2_CTX *));
void	MD2Update __P((MD2_CTX *, const unsigned char *, unsigned int));
void	MD2Final __P((unsigned char[16], MD2_CTX *));
char	*MD2End __P((MD2_CTX *, char *));
char	*MD2File __P((const char *, char *));
char	*MD2Data __P((const unsigned char *, unsigned int, char *));
__END_DECLS

#endif /* _MD2_H_ */
