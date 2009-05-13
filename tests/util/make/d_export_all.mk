# $NetBSD: d_export_all.mk,v 1.1.2.2 2009/05/13 19:19:39 jym Exp $

UT_OK=good
UT_F=fine

.export

.include "d_export.mk"

UT_TEST=export-all
UT_ALL=even this gets exported
