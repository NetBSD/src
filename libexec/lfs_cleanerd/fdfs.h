struct fd_buf {
	char *buf;     /* The buffer itself */
	daddr_t start; /* Start of this buffer */
	daddr_t end;   /* End of this buffer */
};

struct fdfs {
	int	       fd_fd;	     /* The file descriptor */
	int	       fd_bufc;	     /* Number of segment buffers */
	int	       fd_bufi;	     /* Index to next segment buffer */
	struct fd_buf *fd_bufp;	     /* The buffers */
	off_t	       fd_bsize;     /* block size */
	off_t	       fd_ssize;     /* segment size */
};

struct uvnode * fd_vget(int, int, int, int);
int fd_preload(struct uvnode *, daddr_t);
int fd_vop_strategy(struct ubuf *);
int fd_vop_bwrite(struct ubuf *);
int fd_vop_bmap(struct uvnode *, daddr_t, daddr_t *);
char *fd_ptrget(struct uvnode *, daddr_t);
void fd_reclaim(struct uvnode *);
void fd_release(struct uvnode *);
void fd_release_all(struct uvnode *);
