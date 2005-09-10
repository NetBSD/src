#	$NetBSD: Makefile,v 1.1 2005/09/10 19:20:51 jmmv Exp $

tests=	t_mount
tests+=	t_statvfs
tests+=	t_mkdir
tests+=	t_vnode_leak
tests+=	t_setattr
tests+=	t_rmdir
tests+=	t_id_gen
tests+=	t_trail_slash
tests+=	t_dots
tests+=	t_create
tests+=	t_remove
tests+=	t_link
tests+=	t_rename
tests+=	t_read_write
tests+=	t_exec
tests+=	t_truncate
tests+=	t_sizes
tests+=	t_times
tests+=	t_symlink
tests+=	t_mknod
tests+=	t_devices
tests+=	t_pipes
tests+=	t_sockets
tests+=	t_readdir

regress: ${tests}
.for t in	${tests}
	@./${t} ${TEST_ARGS}
.endfor

PROG=	h_tools
NOMAN=	# defined

t_sizes t_sockets t_statvfs: h_tools

.include <bsd.prog.mk>
