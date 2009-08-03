# $NetBSD: Makefile,v 1.5 2009/08/03 15:59:42 plunky Exp $

USE_FORT?= yes	# network protocol library

LIB=	bluetooth

SRCS=	bluetooth.c bt_dev.c \
	sdp_data.c sdp_get.c sdp_match.c sdp_put.c sdp_record.c \
	sdp_set.c sdp_uuid.c sdp_service.c sdp_session.c
MAN=	bluetooth.3 bt_dev.3 sdp.3 sdp_data.3

CPPFLAGS+=	-I${.CURDIR}

.if (${SDP_COMPAT:Uyes} != "no")
SRCS+=		sdp_compat.c
.endif

INCS=		bluetooth.h sdp.h
INCSDIR=	/usr/include

MLINKS+=	bluetooth.3 bt_gethostbyname.3
MLINKS+=	bluetooth.3 bt_gethostbyaddr.3
MLINKS+=	bluetooth.3 bt_gethostent.3
MLINKS+=	bluetooth.3 bt_sethostent.3
MLINKS+=	bluetooth.3 bt_endhostent.3

MLINKS+=	bluetooth.3 bt_getprotobyname.3
MLINKS+=	bluetooth.3 bt_getprotobynumber.3
MLINKS+=	bluetooth.3 bt_getprotoent.3
MLINKS+=	bluetooth.3 bt_setprotoent.3
MLINKS+=	bluetooth.3 bt_endprotoent.3

MLINKS+=	bluetooth.3 bt_ntoa.3
MLINKS+=	bluetooth.3 bt_aton.3

MLINKS+=	bt_dev.3 bt_devaddr.3
MLINKS+=	bt_dev.3 bt_devname.3
MLINKS+=	bt_dev.3 bt_devenum.3
MLINKS+=	bt_dev.3 bt_devinfo.3
MLINKS+=	bt_dev.3 bt_devopen.3
MLINKS+=	bt_dev.3 bt_devsend.3
MLINKS+=	bt_dev.3 bt_devrecv.3
MLINKS+=	bt_dev.3 bt_devreq.3
MLINKS+=	bt_dev.3 bt_devfilter.3
MLINKS+=	bt_dev.3 bt_devfilter_pkt_set.3
MLINKS+=	bt_dev.3 bt_devfilter_pkt_clr.3
MLINKS+=	bt_dev.3 bt_devfilter_pkt_tst.3
MLINKS+=	bt_dev.3 bt_devfilter_evt_set.3
MLINKS+=	bt_dev.3 bt_devfilter_evt_clr.3
MLINKS+=	bt_dev.3 bt_devfilter_evt_tst.3
MLINKS+=	bt_dev.3 bt_devinquiry.3

MLINKS+=	sdp.3 sdp_open.3
MLINKS+=	sdp.3 sdp_open_local.3
MLINKS+=	sdp.3 sdp_close.3
MLINKS+=	sdp.3 sdp_service_search.3
MLINKS+=	sdp.3 sdp_service_attribute.3
MLINKS+=	sdp.3 sdp_service_search_attribute.3
MLINKS+=	sdp.3 sdp_record_insert.3
MLINKS+=	sdp.3 sdp_record_update.3
MLINKS+=	sdp.3 sdp_record_remove.3

MLINKS+=	sdp_data.3 sdp_match_uuid16.3

MLINKS+=	sdp_data.3 sdp_data_print.3
MLINKS+=	sdp_data.3 sdp_data_size.3
MLINKS+=	sdp_data.3 sdp_data_type.3
MLINKS+=	sdp_data.3 sdp_data_valid.3

MLINKS+=	sdp_data.3 sdp_get_alt.3
MLINKS+=	sdp_data.3 sdp_get_attr.3
MLINKS+=	sdp_data.3 sdp_get_bool.3
MLINKS+=	sdp_data.3 sdp_get_data.3
MLINKS+=	sdp_data.3 sdp_get_int.3
MLINKS+=	sdp_data.3 sdp_get_seq.3
MLINKS+=	sdp_data.3 sdp_get_str.3
MLINKS+=	sdp_data.3 sdp_get_url.3
MLINKS+=	sdp_data.3 sdp_get_uint.3
MLINKS+=	sdp_data.3 sdp_get_uuid.3

MLINKS+=	sdp_data.3 sdp_put_alt.3
MLINKS+=	sdp_data.3 sdp_put_attr.3
MLINKS+=	sdp_data.3 sdp_put_bool.3
MLINKS+=	sdp_data.3 sdp_put_data.3
MLINKS+=	sdp_data.3 sdp_put_int.3
MLINKS+=	sdp_data.3 sdp_put_int8.3
MLINKS+=	sdp_data.3 sdp_put_int16.3
MLINKS+=	sdp_data.3 sdp_put_int32.3
MLINKS+=	sdp_data.3 sdp_put_int64.3
MLINKS+=	sdp_data.3 sdp_put_seq.3
MLINKS+=	sdp_data.3 sdp_put_str.3
MLINKS+=	sdp_data.3 sdp_put_uint.3
MLINKS+=	sdp_data.3 sdp_put_uint8.3
MLINKS+=	sdp_data.3 sdp_put_uint16.3
MLINKS+=	sdp_data.3 sdp_put_uint32.3
MLINKS+=	sdp_data.3 sdp_put_uint64.3
MLINKS+=	sdp_data.3 sdp_put_url.3
MLINKS+=	sdp_data.3 sdp_put_uuid.3
MLINKS+=	sdp_data.3 sdp_put_uuid16.3
MLINKS+=	sdp_data.3 sdp_put_uuid32.3
MLINKS+=	sdp_data.3 sdp_put_uuid128.3

MLINKS+=	sdp_data.3 sdp_set_alt.3
MLINKS+=	sdp_data.3 sdp_set_bool.3
MLINKS+=	sdp_data.3 sdp_set_seq.3
MLINKS+=	sdp_data.3 sdp_set_uint.3
MLINKS+=	sdp_data.3 sdp_set_int.3

.include <bsd.lib.mk>
