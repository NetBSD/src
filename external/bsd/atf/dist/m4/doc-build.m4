dnl
dnl Automated Testing Framework (atf)
dnl
dnl Copyright (c) 2007, 2008, 2009 The NetBSD Foundation, Inc.
dnl All rights reserved.
dnl
dnl Redistribution and use in source and binary forms, with or without
dnl modification, are permitted provided that the following conditions
dnl are met:
dnl 1. Redistributions of source code must retain the above copyright
dnl    notice, this list of conditions and the following disclaimer.
dnl 2. Redistributions in binary form must reproduce the above copyright
dnl    notice, this list of conditions and the following disclaimer in the
dnl    documentation and/or other materials provided with the distribution.
dnl
dnl THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
dnl CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
dnl INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
dnl MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
dnl IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
dnl DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
dnl DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
dnl GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
dnl INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
dnl IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
dnl OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
dnl IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
dnl

dnl -----------------------------------------------------------------------
dnl Set up the developer mode
dnl -----------------------------------------------------------------------

dnl
dnl ATF_DOC_BUILD
dnl
dnl Checks for optional requirements to build the documentation files.
dnl
AC_DEFUN([ATF_DOC_BUILD], [
    _ATF_DOC_FLAG

    if test ${enable_doc_build} = yes; then
        dnl Required tools.
        _ATF_DOC_CHECK_TOOL([links], [LINKS])
        _ATF_DOC_CHECK_TOOL([tidy], [TIDY])
        _ATF_DOC_CHECK_TOOL([xmlcatalog], [XMLCATALOG])
        _ATF_DOC_CHECK_TOOL([xmllint], [XMLLINT])
        _ATF_DOC_CHECK_TOOL([xsltproc], [XSLTPROC])

        dnl DTD check.
        _ATF_DOC_DTD([-//OASIS//DTD Simplified DocBook XML V1.1//EN])
    fi

    AC_MSG_NOTICE([Building documentation: ${enable_doc_build}])
    if test ${enable_doc_build} = no; then
        AC_MSG_WARN(['make dist' won't work if documentation is edited])
    fi

    AC_SUBST([DOC_BUILD], [${enable_doc_build}])
    AM_CONDITIONAL([DOC_BUILD], [test ${enable_doc_build} = yes])
])

dnl
dnl _ATF_DOC_FLAG
dnl
dnl Adds a --enable-doc-build flag to build the documentation.  Defaults
dnl to 'no' because distfiles ship with pre-built documents.
dnl
AC_DEFUN([_ATF_DOC_FLAG], [
    AC_ARG_ENABLE(doc-build,
                  AS_HELP_STRING(--enable-doc-build,
                                 [enable building of documentation]),,
                  enable_doc_build=no)

    case "${enable_doc_build}" in
        yes|no)
            ;;
        *)
            AC_MSG_ERROR([Invalid argument for --enable-doc-build])
            ;;
    esac
])

dnl
dnl _ATF_DOC_CHECK_TOOL(lowercase-name, uppercase-name)
dnl
dnl Checks for the 'lowercase-name' tool and sets the 'uppercase-name'
dnl variable to it if found.  Aborts execution if not found.
dnl
AC_DEFUN([_ATF_DOC_CHECK_TOOL], [
    AC_ARG_VAR([$2], [Absolute path to the $1 tool; build-time only])
    AC_PATH_PROG([$2], [$1])
    if test "${$2:-empty}" = empty; then
        AC_MSG_ERROR([$1 could not be found])
    fi
])

dnl
dnl _ATF_DOC_DTD(entity)
dnl
dnl Checks for the presence of a DTD based on its entity name.
dnl
AC_DEFUN([_ATF_DOC_DTD], [
    AC_ARG_VAR([XML_CATALOG_FILE],
               [XML catalog to use (default: /etc/xml/catalog)])
    : ${XML_CATALOG_FILE:=/etc/xml/catalog}

    AC_MSG_CHECKING([for DTD $1])
    XML_CATALOG_FILES= ${XMLCATALOG} ${XML_CATALOG_FILE} '$1' \
        >conftest.out 2>&1
    if test ${?} -ne 0; then
        AC_MSG_RESULT([no])
        echo "XML catalog: ${XML_CATALOG_FILE}"
        echo "xmlcatalog output:"
        cat conftest.out
        AC_MSG_ERROR([Could not find the '$1' DTD])
    else
        AC_MSG_RESULT([yes])
    fi
])
