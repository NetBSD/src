#	$NetBSD: Makefile,v 1.1 2000/10/26 23:32:14 jdolecek Exp $

MKMAN=  no

regress: test_one test_two

test_one:
	@good="1/2"; \
	out=`expr X"1/2/3" : 'X\(.*[^/]\)//*[^/][^/]*/*$$' \| . : '\(.\)'`; \
	if [ "$$good" != "$$out" ]; then	\
		echo "Test 1 (precedence) failed - got '$$out', should be '$$good'."; \
		exit 1; \
	else \
		echo "Test 1 (precedence) succeeded."; \
	fi
	
test_two:
	@good="2"; \
	out=`expr '1/2' : '.*/\(.*\)'`; \
	if [ "$$good" != "$$out" ]; then	\
		echo "Test 2 failed - got '$$out', should be '$$good'."; \
		exit 1; \
	else \
		echo "Test 2 succeeded."; \
	fi


.include <bsd.prog.mk>
