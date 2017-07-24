/* $NetBSD: blkdev.h,v 1.1.1.1 2017/07/24 08:56:29 mrg Exp $ */

#if 0	/* folded into devopen */
int	blkdevopen(struct open_file *, ...);
#endif

int	blkdevstrategy(void *, int, daddr_t, size_t, void *, size_t *);
#if !defined(LIBSA_NO_FS_CLOSE)
int	blkdevclose(struct open_file *);
#endif /* defined(LIBSA_NO_FS_CLOSE) */
