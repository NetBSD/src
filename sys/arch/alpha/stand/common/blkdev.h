/* $NetBSD: blkdev.h,v 1.1 1999/04/02 03:17:29 cgd Exp $ */

#if 0	/* folded into devopen */
int	blkdevopen __P((struct open_file *, ...));
#endif

int	blkdevstrategy __P((void *, int, daddr_t, size_t, void *, size_t *));
#if defined(LIBSA_NO_FS_CLOSE)
int	blkdevclose __P((struct open_file *));
#endif /* defined(LIBSA_NO_FS_CLOSE) */
