TRADCPP_OBJDIR!=	${MAKE} -C ${.CURDIR}/.. -V .OBJDIR
TRADCPP=	${TRADCPP_OBJDIR}/tradcpp

TESTS=\
	t01 t02 t03 t04 t05 t06 t07 t08 t09 t10 t11 t12 t13 t14 t15 t16 \
	t17 t18 t19 t20 t21 t22 t23 t24 t25 t26 t27 t28 t29 t30 t31 t32 \
	t33 t34 t35 t36 t37 t38 t39 t40

all: run-tests .WAIT show-diffs

.for T in $(TESTS)
run-tests: $(T).diff

$(T).diff: $(T).run $(T).good $(TRADCPP)
	-diff -u $(T).good $(T).run > $(T).diff

$(T).run: $(TRADCPP) $(T).c
	$(TRADCPP) $(T).c > $(T).run 2>&1 || echo FAILED >> $(T).run
.endfor

show-diffs:
	@echo '*** Test diffs ***'
.for T in $(TESTS)
	@cat $(T).diff
.endfor

clean:
.for T in $(TESTS)
	rm -f $(T).run $(T).diff
.endfor

good:
.for T in $(TESTS)
	cp $(T).run $(T).good
.endfor

.PHONY: all run-tests show-diffs clean good

############################################################

.if defined(ALLOW_BROKEN_ATF_POLLUTION)
TESTDIR=	${TESTSBASE}/usr.bin/tradcpp
TESTS_SH+=	tradcpp
.include <bsd.test.mk>
.endif
