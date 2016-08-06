#	$NetBSD: Makefile.inc,v 1.2.132.1 2016/08/06 00:19:05 pgoyette Exp $

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

