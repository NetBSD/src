# $FreeBSD$

SRCS+=	commands.c console.c devopen.c interp.c boot.c bcache.c#XXX: Fixme: Do we need the bcache ? bcache.c 
SRCS+=	interp_backslash.c interp_parse.c ls.c misc.c 
SRCS+=	panic.c calloc.c readdir.c pager.c environment.c fileload.c 
SRCS+=  getopt.c gets.c strdup.c strtol.c strspn.c

.if ${MACHINE_ARCH} == "ia64" 
SRCS+=	load_elf64.c 
.endif

.if defined(LOADER_NET_SUPPORT)
#SRCS+=	dev_net.c
.endif

