#	$NetBSD: Makefile,v 1.10 2003/03/08 19:12:16 perry Exp $

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

LIB=	global

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${LIB}
.PATH:	${DIST}

SRCS=	been_here.c bounce.c canon_addr.c cleanup_strerror.c clnt_stream.c \
	debug_peer.c debug_process.c defer.c deliver_completed.c \
	deliver_flock.c deliver_pass.c deliver_request.c domain_list.c \
	dot_lockfile.c dot_lockfile_as.c ext_prop.c file_id.c \
	header_opts.c is_header.c mail_addr.c mail_addr_crunch.c \
	mail_addr_find.c mail_addr_map.c mail_command_server.c \
	mail_command_client.c mail_conf.c mail_conf_bool.c mail_conf_int.c \
	mail_conf_raw.c mail_conf_str.c mail_connect.c mail_copy.c \
	mail_date.c mail_error.c mail_flush.c mail_open_ok.c mail_params.c \
	mail_pathname.c mail_queue.c mail_run.c \
	mail_scan_dir.c mail_stream.c mail_task.c mail_trigger.c maps.c \
	mark_corrupt.c mkmap_db.c mkmap_dbm.c mkmap_open.c mynetworks.c \
	mypwd.c namadr_list.c off_cvt.c opened.c own_inet_addr.c \
	pipe_command.c post_mail.c quote_821_local.c \
	quote_822_local.c rec_streamlf.c rec_type.c recipient_list.c \
	record.c remove.c resolve_clnt.c resolve_local.c rewrite_clnt.c \
	sent.c smtp_stream.c split_addr.c string_list.c sys_exits.c \
	timed_ipc.c tok822_find.c tok822_node.c tok822_parse.c \
	tok822_resolve.c tok822_rewrite.c tok822_tree.c xtext.c bounce_log.c \
	flush_clnt.c mail_conf_time.c mbox_conf.c mbox_open.c abounce.c \
	verp_sender.c match_parent_style.c mime_state.c header_token.c \
	strip_addr.c virtual8_maps.c hold_message.c dict_proxy.c mail_dict.c

# IPv6
#SRCS+=	wildcard_inet_addr.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
