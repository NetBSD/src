#	$NetBSD: Makefile,v 1.2 2001/03/13 18:13:24 itojun Exp $

LIB=	util

DIST=	${.CURDIR}/../../../dist/postfix/src/${LIB}
.PATH:	${DIST}

SRCS=	argv.c argv_split.c attr.c basename.c binhash.c chroot_uid.c \
	close_on_exec.c concatenate.c dict.c dict_db.c dict_dbm.c \
	dict_env.c dict_ht.c dict_ldap.c dict_mysql.c dict_ni.c dict_nis.c \
	dict_nisplus.c dict_open.c dir_forest.c doze.c environ.c \
	events.c exec_command.c fifo_listen.c fifo_trigger.c file_limit.c \
	find_inet.c fsspace.c fullname.c get_domainname.c get_hostname.c \
	htable.c inet_addr_host.c inet_addr_list.c inet_addr_local.c \
	inet_connect.c inet_listen.c inet_trigger.c inet_util.c \
	line_wrap.c lowercase.c lstat_as.c mac_parse.c make_dirs.c \
	match_list.c match_ops.c msg.c msg_output.c msg_syslog.c \
	msg_vstream.c mvect.c myflock.c mymalloc.c mystrtok.c name_mask.c \
	non_blocking.c open_as.c open_limit.c open_lock.c peekfd.c \
	percentm.c posix_signals.c printable.c read_wait.c readable.c \
	readlline.c ring.c safe_getenv.c safe_open.c sane_accept.c \
	scan_dir.c set_eugid.c set_ugid.c sigdelay.c skipblanks.c \
	split_at.c stat_as.c sys_compat.c timed_connect.c timed_wait.c \
	translit.c trimblanks.c unix_connect.c unix_listen.c unix_trigger.c \
	unsafe.c username.c valid_hostname.c vbuf.c vbuf_print.c \
	vstream.c vstream_popen.c vstring.c vstring_vstream.c writable.c \
	write_buf.c write_wait.c dict_unix.c dict_pcre.c stream_listen.c \
	stream_connect.c stream_trigger.c dict_regexp.c mac_expand.c \
	clean_env.c watchdog.c spawn_command.c duplex_pipe.c sane_rename.c \
	sane_link.c unescape.c timed_read.c timed_write.c dict_tcp.c \
	hex_quote.c dict_alloc.c rand_sleep.c sane_time.c dict_debug.c \
	sane_socketpair.c

MKLINT=		no
MKPIC=		no
MKPROFILE=	no

# only needed during build
libinstall::

.include <bsd.lib.mk>
