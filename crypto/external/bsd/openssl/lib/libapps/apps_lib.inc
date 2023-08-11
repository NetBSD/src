#	$NetBSD: apps_lib.inc,v 1.1.2.2 2023/08/11 13:41:54 martin Exp $

.PATH:	${OPENSSLSRC}/apps/lib

APPS_LIB_SRCS += \
app_libctx.c \
app_params.c \
app_provider.c \
app_rand.c \
app_x509.c \
apps.c \
apps_ui.c \
columns.c \
cmp_mock_srv.c \
engine.c \
engine_loader.c \
fmt.c \
http_server.c \
names.c \
opt.c \
s_cb.c \
s_socket.c \
tlssrp_depr.c \

SRCS += ${APPS_LIB_SRCS}

.for commonsrc in ${APPS_LIB_SRCS}
CPPFLAGS.${commonsrc} += -I${OPENSSLSRC}/apps/lib ${APPS_LIBCPPFLAGS}
.endfor
