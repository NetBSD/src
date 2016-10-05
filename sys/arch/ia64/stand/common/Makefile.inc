#	$NetBSD: Makefile.inc,v 1.2.128.1 2016/10/05 20:55:29 skrll Exp $

SRCS+=	commands.c console.c devopen.c interp.c boot.c dev_net.c
SRCS+=	interp_backslash.c interp_parse.c ls.c misc.c 
SRCS+=	panic.c calloc.c readdir.c pager.c environment.c fileload.c 
SRCS+=  getopt.c gets.c strdup.c strtol.c strspn.c

.if ${MACHINE_ARCH} == "ia64" 
SRCS+=	load_elf64.c 
.endif

# XXX might need this for different loaders later
#.if defined(LOADER_NET_SUPPORT)
#SRCS+=	dev_net.c
#.endif

