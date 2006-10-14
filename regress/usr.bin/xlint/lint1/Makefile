
TESTS != echo test*.c
LINT1 = ${DESTDIR}/usr/libexec/lint1

regress:
.	for i in ${TESTS}
	${LINT1} -g -S $i /dev/null
.	endfor
