# $NetBSD: d_export_all.mk,v 1.1.2.2 2012/04/17 00:09:20 yamt Exp $

UT_OK=good
UT_F=fine

.export

.include "d_export.mk"

UT_TEST=export-all
UT_ALL=even this gets exported
