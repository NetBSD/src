/*	$NetBSD: dvma.h,v 1.3 1998/01/05 07:03:26 perry Exp $	*/


void dvma_init();

char * dvma_mapin(char *pkt, int len);
void dvma_mapout(char *dmabuf, int len);

char * dvma_alloc(int len);
void dvma_free(char *dvma, int len);

