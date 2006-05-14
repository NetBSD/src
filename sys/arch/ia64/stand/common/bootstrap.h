/*	$NetBSD: bootstrap.h,v 1.3 2006/05/14 21:55:38 elad Exp $	*/

/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/boot/common/bootstrap.h,v 1.38.6.1 2004/09/03 19:25:40 iedowse Exp $
 */

#ifndef _BOOTSTRAP_H_
#define _BOOTSTRAP_H_

#include <sys/types.h>
#include <sys/queue.h>

/*
 * Generic device specifier; architecture-dependant 
 * versions may be larger, but should be allowed to
 * overlap.
 */

struct devdesc 
{
    struct devsw	*d_dev;
    int			d_type;
#define DEVT_NONE	0
#define DEVT_DISK	1
#define DEVT_NET	2
#define	DEVT_CD		3
};

/* Commands and return values; nonzero return sets command_errmsg != NULL */
typedef int	(bootblk_cmd_t)(int argc, char *argv[]);
extern char	*command_errmsg;	
extern char	command_errbuf[];	/* XXX blah, length */
#define CMD_OK		0
#define CMD_ERROR	1

/* interp.c */
void	interact(void);
int	include(const char *filename);

/* interp_backslash.c */
char	*backslash(char *str);

/* interp_parse.c */
int	parse(int *argc, char ***argv, char *str);

/* interp_forth.c */
void	bf_init(void);
int	bf_run(char *line);

/* boot.c */
int	autoboot(int timeout, char *prompt);
void	autoboot_maybe(void);
int	getrootmount(char *rootdev);

/* misc.c */
char	*unargv(int argc, char *argv[]);
void	hexdump(caddr_t region, size_t len);
size_t	strlenout(vaddr_t str);
char	*strdupout(vaddr_t str);
void	kern_bzero(vaddr_t dest, size_t len);
int	kern_pread(int fd, vaddr_t dest, size_t len, off_t off);
void	*alloc_pread(int fd, off_t off, size_t len);

/* bcache.c */
int	bcache_init(u_int nblks, size_t bsize);
void	bcache_flush(void);
int	bcache_strategy(void *devdata, int unit, int rw, daddr_t blk,
			size_t size, char *buf, size_t *rsize);


/* strdup.c */
char	*strdup(const char*);

/*
 * Disk block cache
 */
struct bcache_devdata
{
    int         (*dv_strategy)(void *devdata, int rw, daddr_t blk, size_t size, char *buf, size_t *rsize);
    void	*dv_devdata;
};

/*
 * Modular console support.
 */
struct console 
{
    const char	*c_name;
    const char	*c_desc;
    int		c_flags;
#define C_PRESENTIN	(1<<0)
#define C_PRESENTOUT	(1<<1)
#define C_ACTIVEIN	(1<<2)
#define C_ACTIVEOUT	(1<<3)
    void	(* c_probe)(struct console *cp);	/* set c_flags to match hardware */
    int		(* c_init)(int arg);			/* reinit XXX may need more args */
    void	(* c_out)(int c);			/* emit c */
    int		(* c_in)(void);				/* wait for and return input */
    int		(* c_ready)(void);			/* return nonzer if input waiting */
};
extern struct console	*consoles[];
void		cons_probe(void);

/*
 * Plug-and-play enumerator/configurator interface.
 */
struct pnphandler 
{
    const char	*pp_name;		/* handler/bus name */
    void	(* pp_enumerate)(void);	/* enumerate PnP devices, add to chain */
};

struct pnpident
{
    char			*id_ident;	/* ASCII identifier, actual format varies with bus/handler */
    STAILQ_ENTRY(pnpident)	id_link;
};

struct pnpinfo
{
    char			*pi_desc;	/* ASCII description, optional */
    int				pi_revision;	/* optional revision (or -1) if not supported */
    char			*pi_module;	/* module/args nominated to handle device */
    int				pi_argc;	/* module arguments */
    char			**pi_argv;
    struct pnphandler		*pi_handler;	/* handler which detected this device */
    STAILQ_HEAD(,pnpident)	pi_ident;	/* list of identifiers */
    STAILQ_ENTRY(pnpinfo)	pi_link;
};

STAILQ_HEAD(pnpinfo_stql, pnpinfo);

extern struct pnpinfo_stql pnp_devices;

extern struct pnphandler	*pnphandlers[];		/* provided by MD code */

void			pnp_addident(struct pnpinfo *pi, char *ident);
struct pnpinfo		*pnp_allocinfo(void);
void			pnp_freeinfo(struct pnpinfo *pi);
void			pnp_addinfo(struct pnpinfo *pi);
char			*pnp_eisaformat(u_int8_t *data);

/*
 *  < 0	- No ISA in system
 * == 0	- Maybe ISA, search for read data port
 *  > 0	- ISA in system, value is read data port address
 */
extern int			isapnp_readport;

struct preloaded_file;

/*
 * Preloaded file information. Depending on type, file can contain
 * additional units called 'modules'.
 *
 * At least one file (the kernel) must be loaded in order to boot.
 * The kernel is always loaded first.
 *
 * String fields (m_name, m_type) should be dynamically allocated.
 */
struct preloaded_file
{
    char			*f_name;	/* file name */
    char			*f_type;	/* verbose file type, eg 'ELF kernel', 'pnptable', etc. */
    char			*f_args;	/* arguments for the file */
    int				f_loader;	/* index of the loader that read the file */
    vaddr_t			f_addr;		/* load address */
    size_t			f_size;		/* file size */
    struct preloaded_file	*f_next;	/* next file */
    u_long                      *marks;         /* filled by loadfile() */
};

struct file_format
{
    /* Load function must return EFTYPE if it can't handle the module supplied */
    int		(* l_load)(char *filename, u_int64_t dest, struct preloaded_file **result);
    /* Only a loader that will load a kernel (first module) should have an exec handler */
    int		(* l_exec)(struct preloaded_file *mp);
};

extern struct file_format	*file_formats[];	/* supplied by consumer */
extern struct preloaded_file	*preloaded_files;

int			mod_load(char *name, int argc, char *argv[]);
int			mod_loadkld(const char *name, int argc, char *argv[]);

struct preloaded_file *file_alloc(void);
struct preloaded_file *file_findfile(char *name, char *type);

void file_discard(struct preloaded_file *fp);

int	elf64_loadfile(char *filename, u_int64_t dest, struct preloaded_file **result);

/*
 * Support for commands 
 */
struct bootblk_command 
{
    const char		*c_name;
    const char		*c_desc;
    bootblk_cmd_t	*c_fn;
};

/* Prototypes for the command handlers within stand/common/ */

/* command.c */

int command_help(int argc, char *argv[]) ;
int command_commandlist(int argc, char *argv[]);
int command_show(int argc, char *argv[]);
int command_set(int argc, char *argv[]);
int command_unset(int argc, char *argv[]);
int command_echo(int argc, char *argv[]);
int command_read(int argc, char *argv[]);
int command_more(int argc, char *argv[]);
int command_lsdev(int argc, char *argv[]);

/*	bcache.c	XXX: Fixme: Do we need the bcache ?*/
/* int command_bcache(int argc, char *argv[]); */
/*	boot.c		*/
int command_boot(int argc, char *argv[]);
int command_autoboot(int argc, char *argv[]);
/*	fileload.c	*/
int command_load(int argc, char *argv[]);
int command_unload(int argc, char *argv[]);
int command_lskern(int argc, char *argv[]);
/*	interp.c	*/
int command_include(int argc, char *argv[]);
/*	ls.c		*/
int command_ls(int argc, char *argv[]);

#define COMMAND_SET(a, b, c, d) /* nothing */

#define COMMON_COMMANDS							\
	/*	common.c	*/					\
	{ "help",	"detailed help",	command_help },		\
	{ "?",		"list commands",	command_commandlist },	\
	{ "show", "show variable(s)", command_show },			\
	{ "set", "set a variable", command_set },			\
	{ "unset", "unset a variable", command_unset },			\
	{ "echo", "echo arguments", command_echo },			\
	{ "read", "read input from the terminal", command_read },	\
	{ "more", "show contents of a file", command_more },		\
	{ "lsdev", "list all devices", command_lsdev },			\
									\
	/*	bcache.c	XXX: Fixme: Do we need the bcache ? */	\
									\
/*	{ "bcachestat", "get disk block cache stats", command_bcache }, */\
									\
	/*	boot.c	*/						\
									\
	{ "boot", "boot a file or loaded kernel", command_boot },	\
	{ "autoboot", "boot automatically after a delay", command_autoboot }, \
									\
	/*	fileload.c	*/					\
									\
	{ "load", "load a kernel", command_load },			\
	{ "unload", "unload all modules", command_unload },		\
	{ "lskern", "list loaded kernel", command_lskern },		\
									\
	/*	interp.c	*/					\
									\
	{ "include", "read commands from a file", command_include },	\
									\
	/*	ls.c	*/						\
									\
	{ "ls", "list files", command_ls }

extern struct bootblk_command commands[];


/* 
 * The intention of the architecture switch is to provide a convenient
 * encapsulation of the interface between the bootstrap MI and MD code.
 * MD code may selectively populate the switch at runtime based on the
 * actual configuration of the target system.
 */
struct arch_switch
{
    /* Automatically load modules as required by detected hardware */
    int		(*arch_autoload)(void);
    /* Locate the device for (name), return pointer to tail in (*path) */
    int		(*arch_getdev)(void **dev, const char *name, const char **path);
    /* Copy from local address space to module address space, similar to bcopy() */
    ssize_t	(*arch_copyin)(const void *src, vaddr_t dest,
			       const size_t len);
    /* Copy to local address space from module address space, similar to bcopy() */
    ssize_t	(*arch_copyout)(const vaddr_t src, void *dest,
				const size_t len);
    /* Read from file to module address space, same semantics as read() */
    ssize_t	(*arch_readin)(const int fd, vaddr_t dest,
			       const size_t len);
    /* Perform ISA byte port I/O (only for systems with ISA) */
    int		(*arch_isainb)(int port);
    void	(*arch_isaoutb)(int port, int value);
};
extern struct arch_switch archsw;

/* This must be provided by the MD code, but should it be in the archsw? */
void	delay(int delay);

void	dev_cleanup(void);

time_t	time(time_t *tloc);

/* calloc.c */
void    *calloc(unsigned int, unsigned int);

/* pager.c */
extern void	pager_open(void);
extern void	pager_close(void);
extern int	pager_output(const char *lines);
extern int	pager_file(const char *fname);

/* environment.c */
#define EV_DYNAMIC	(1<<0)		/* value was dynamically allocated, free if changed/unset */
#define EV_VOLATILE	(1<<1)		/* value is volatile, make a copy of it */
#define EV_NOHOOK	(1<<2)		/* don't call hook when setting */

struct env_var;
typedef char	*(ev_format_t)(struct env_var *ev);
typedef int	(ev_sethook_t)(struct env_var *ev, int flags,
		    const void *value);
typedef int	(ev_unsethook_t)(struct env_var *ev);

struct env_var
{
    char		*ev_name;
    int			ev_flags;
    void		*ev_value;
    ev_sethook_t	*ev_sethook;
    ev_unsethook_t	*ev_unsethook;
    struct env_var	*ev_next, *ev_prev;
};
extern struct env_var	*environ;

extern struct env_var	*env_getenv(const char *name);
extern int		env_setenv(const char *name, int flags,
				   const void *value, ev_sethook_t sethook,
				   ev_unsethook_t unsethook);
extern char		*getenv(const char *name);
extern int		setenv(const char *name, const char *value,
			       int overwrite);
extern int		putenv(const char *string);
extern int		unsetenv(const char *name);

extern ev_sethook_t	env_noset;		/* refuse set operation */
extern ev_unsethook_t	env_nounset;		/* refuse unset operation */



/* FreeBSD wrappers */


struct dirent *readdirfd(int);               /* XXX move to stand.h */

#define free(ptr) dealloc(ptr, 0) /* XXX UGLY HACK!!! This should work for just now though. See: libsa/alloc.c:free() */

/* XXX Hack Hack Hack!!! Need to update stand.h with fs_ops->fo_readdir */
#ifdef SKIFS /* defined via stand/ia64/ski/Makefile */
#define FS_READDIR(f, dirptr) skifs_readdir(f, dirptr)   
#else
#define FS_READDIR(f, dirptr) efifs_readdir(f, dirptr)
#endif

/* gets.c XXX move to stand.h */

extern int	fgetstr(char *buf, int size, int fd);
extern void	ngets(char *, int);

/* imports from stdlib, locally modified */

extern long	strtol(const char *, char **, int);
extern char	*optarg;			/* getopt(3) external variables */
extern int	optind, opterr, optopt, optreset;
extern int	getopt(int, char * const [], const char *);

extern long	strtol(const char *, char **, int);

/* XXX: From <fcntl.h>. Its not very _STANDALONE friendly */
/* open-only flags */
#define	O_RDONLY	0x00000000	/* open for reading only */
#define	O_WRONLY	0x00000001	/* open for writing only */
#define	O_RDWR		0x00000002	/* open for reading and writing */
#define	O_ACCMODE	0x00000003	/* mask for above modes */

#define ELF64_KERNELTYPE "elf kernel"

#endif /* _BOOTSTRAP_H_ */
