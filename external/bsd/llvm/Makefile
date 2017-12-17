#	$NetBSD: Makefile,v 1.10 2017/12/17 21:16:57 joerg Exp $

.include <bsd.init.mk>

SUBDIR=	include \
	.WAIT \
	lib \
	.WAIT \
	bin

.if ${MKCLANGRT:Uno} == "yes"
SUBDIR+=	librt
.endif

.include "Makefile.inc"

checkout:
	svn co -r ${LLD_REVISION} ${SVN_ROOT}/lld/${SVN_BRANCH} ${LLD_SRCDIR}
	svn co -r ${LLDB_REVISION} ${SVN_ROOT}/lldb/${SVN_BRANCH} ${LLDB_SRCDIR}

checkout-imported:
	svn co -r ${CLANG_REVISION} ${SVN_ROOT}/cfe/${SVN_BRANCH} ${CLANG_SRCDIR}
	svn co -r ${LLVM_REVISION} ${SVN_ROOT}/llvm/${SVN_BRANCH} ${LLVM_SRCDIR}

checkout-mclinker:
	if [ -d ${MCLINKER_SRCDIR}/.git ]; then \
		cd ${MCLINKER_SRCDIR}; git pull ${MCLINKER_ROOT} ; \
	else \
		git clone ${MCLINKER_ROOT} ${MCLINKER_SRCDIR}; \
	fi
	cd ${MCLINKER_SRCDIR} && git checkout ${MCLINKER_REVISION}

import-clang:
	cd ${CLANG_SRCDIR} && cvs -d cvs.netbsd.org:/cvsroot import -m "Import clang ${CLANG_REVISION} from ${SVN_BRANCH}" src/external/bsd/llvm/dist/clang LLVM clang-${CLANG_REVISION:C,^r,,}
import-llvm:
	cd ${LLVM_SRCDIR} && cvs -d cvs.netbsd.org:/cvsroot import -m "Import LLVM ${LLVM_REVISION} from ${SVN_BRANCH}" src/external/bsd/llvm/dist/llvm LLVM llvm-${LLVM_REVISION:C,^r,,}

.include <bsd.subdir.mk>
