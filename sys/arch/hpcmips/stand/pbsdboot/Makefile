#	$NetBSD: Makefile,v 1.5.16.2 2004/09/18 14:34:58 skrll Exp $

UUDECODE_FILES=	pbsdboot.exe pbsdboot1.exe

uu::
	if [ X"`strings WMIPSDbg/pbsdboot.exe | grep 'Compiled for WinCE'`" = \
           X'Compiled for WinCE 1.01' ]; then \
          echo This binary is compiled for WinCE 1.01; \
          uuencode WMIPSDbg/pbsdboot.exe pbsdboot1.exe > pbsdboot1.uu; \
        else \
          echo This binary is compiled for WinCE 2.00; \
          uuencode WMIPSDbg/pbsdboot.exe pbsdboot.exe > pbsdboot.uu; \
        fi

.include <bsd.files.mk>
