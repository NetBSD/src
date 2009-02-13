# $NetBSD: d_export_all.mk,v 1.1 2009/02/13 05:19:52 jmmv Exp $

UT_OK=good
UT_F=fine

.export

.include "d_export.mk"

UT_TEST=export-all
UT_ALL=even this gets exported
