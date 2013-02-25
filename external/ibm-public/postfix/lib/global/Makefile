#	$NetBSD: Makefile,v 1.3.2.1 2013/02/25 00:27:33 tls Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	global

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${LIB}
.PATH:	${DIST}

SRCS=	abounce.c anvil_clnt.c been_here.c bounce.c bounce_log.c \
	canon_addr.c cfg_parser.c cleanup_strerror.c cleanup_strflags.c \
	clnt_stream.c conv_time.c db_common.c debug_peer.c debug_process.c \
	defer.c deliver_completed.c deliver_flock.c deliver_pass.c \
	deliver_request.c dict_ldap.c dict_memcache.c dict_mysql.c \
	dict_pgsql.c dict_proxy.c dict_sqlite.c \
	domain_list.c dot_lockfile.c dot_lockfile_as.c \
	dsb_scan.c dsn.c dsn_buf.c dsn_mask.c dsn_print.c dsn_util.c \
	ehlo_mask.c ext_prop.c file_id.c flush_clnt.c header_opts.c \
	header_token.c input_transp.c int_filt.c is_header.c log_adhoc.c \
	mail_addr.c mail_addr_crunch.c mail_addr_find.c mail_addr_map.c \
	mail_command_client.c mail_command_server.c mail_conf.c \
	mail_conf_bool.c mail_conf_int.c mail_conf_long.c mail_conf_raw.c \
	mail_conf_str.c mail_conf_time.c mail_connect.c mail_copy.c \
	mail_date.c mail_dict.c mail_error.c mail_flush.c mail_open_ok.c \
	mail_params.c mail_pathname.c mail_queue.c mail_run.c \
	mail_scan_dir.c mail_stream.c mail_task.c mail_trigger.c maps.c \
	mark_corrupt.c match_parent_style.c mbox_conf.c mbox_open.c \
	mime_state.c mkmap_cdb.c mkmap_db.c mkmap_dbm.c mkmap_open.c \
	mkmap_sdbm.c msg_stats_print.c msg_stats_scan.c mynetworks.c \
	mypwd.c namadr_list.c off_cvt.c opened.c own_inet_addr.c \
	pipe_command.c post_mail.c quote_821_local.c quote_822_local.c \
	rcpt_buf.c rcpt_print.c rec_attr_map.c rec_streamlf.c rec_type.c \
	recipient_list.c record.c remove.c resolve_clnt.c resolve_local.c \
	rewrite_clnt.c scache_clnt.c scache_multi.c scache_single.c \
	sent.c smtp_stream.c split_addr.c string_list.c strip_addr.c \
	sys_exits.c timed_ipc.c tok822_find.c tok822_node.c tok822_parse.c \
	tok822_resolve.c tok822_rewrite.c tok822_tree.c trace.c \
	user_acl.c valid_mailhost_addr.c verify.c verify_clnt.c \
	verp_sender.c wildcard_inet_addr.c xtext.c data_redirect.c \
	delivered_hdr.c fold_addr.c mkmap_proxy.c header_body_checks.c \
	mail_conf_nint.c match_service.c match_service.h mail_conf_nbool.c \
	smtp_reply_footer.c memcache_proto.c mkmap_fail.c safe_ultostr.c \
	server_acl.c verify_sender_addr.c mail_version.c

.include <bsd.lib.mk>
