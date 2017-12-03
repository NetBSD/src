/* $NetBSD: blkdev.h,v 1.1.1.1.6.2 2017/12/03 11:36:11 jdolecek Exp $ */

#if 0	/* folded into devopen */
int	blkdevopen(struct open_file *, ...);
#endif

int	blkdevstrategy(void *, int, daddr_t, size_t, void *, size_t *);
#if !defined(LIBSA_NO_FS_CLOSE)
int	blkdevclose(struct open_file *);
#endif /* defined(LIBSA_NO_FS_CLOSE) */
