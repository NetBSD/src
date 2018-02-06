:: $OpenLDAP$
:: This work is part of OpenLDAP Software <http://www.openldap.org/>.
::
:: Copyright 1998-2017 The OpenLDAP Foundation.
:: All rights reserved.
::
:: Redistribution and use in source and binary forms, with or without
:: modification, are permitted only as authorized by the OpenLDAP
:: Public License.
::
:: A copy of this license is available in the file LICENSE in the
:: top-level directory of the distribution or, alternatively, at
:: <http://www.OpenLDAP.org/license.html>.

::
:: Create a version.c file from build/version.h
::

:: usage: mkvers.bat <path/version.h>, <version.c>, <appname>, <static>

copy %1 %2
(echo. ) >> %2
(echo #include "portable.h") >> %2
(echo. ) >> %2
(echo %4 const char __Version[] =) >> %2
(echo "@(#) $" OPENLDAP_PACKAGE ": %3 " OPENLDAP_VERSION) >> %2
(echo " (" __DATE__ " " __TIME__ ") $\n") >> %2
(echo "\t%USERNAME%@%COMPUTERNAME% %CD:\=/%\n";) >> %2
