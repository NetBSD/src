##  Notes:
##    This set of tests creates a dummy directory tree in /tmp and
##    populates it with several files.  The test requires around 1100
##    inodes and 6.8MB. The test system needs to run a kernel with
##    FFS Endian Independent support (options FFS_EI) to be able to
##    perform this test.
##
##    This test verifies that newfs can create filesystems in both bytes
##    orders. For each byte order it is checked that:
##      - the kernel understands both filesystems created
##      - fsck_ffs can convert them from one byte order to the other
##      - dump handles properly the 'nodump' flag, an that restore can
##        properly restore the filesystem.
##
##    This is derived from work done by Brian Grayson, submitted in PR 6706.

TMPL=/tmp/ffstemplate
TMPMP=/tmp/ffsregresstest_mount
TMPIM=/var/tmp/ffsregresstest.im
TMPREST=/tmp/restoreregress
EN?= be
DIGITS=0 1 2 3 4 5 6 7 8 9

VND?= vnd0
BVND= /dev/${VND}
CVND= /dev/r${VND}
MPART?= a
.if (${MACHINE_ARCH} == "i386")
RPART?= d
.else
RPART?= c
.endif

# As make is called recusively, and we may cd to ${OBJDIR}, we need to call make
# with some variables propagated.
SRCDIR?= ${.CURDIR}
MAKECMD= ${MAKE} -f ${SRCDIR}/Makefile SRCDIR=${SRCDIR} EN=${EN} VND=${VND} MPART=${MPART} RPART=${RPART}

regress: template
	${MAKECMD} EN=be makeregress
	${MAKECMD} clean-tmpfs
	${MAKECMD} EN=le makeregress
	${MAKECMD} clean

makeregress: tmpfs
	${MAKECMD} dump
	${MAKECMD} fsck
	${MAKECMD} clean-dump;

clean: clean-tmpfs clean-template clean-dump
clean-dump:
	@echo "***  Cleaning up ${TMPREST}."
	-rm -rf ${TMPREST}
clean-tmpfs:
	@echo "***  Cleaning up ${TMPMP}."
	-umount ${TMPMP}
	-vnconfig -u ${BVND}${RPART}
	-rm -rf ${TMPMP} ${TMPIM}
clean-template:
	@echo "***  Cleaning up ${TMPL}."
	-rm -rf ${TMPL}

cmp:
	diff -r -q ${TMPL} ${TMPMP}

dump:
	@echo "***  Testing dump nodump flag support."
	@#  First of all, estimate the size of a dump of just the tmpfs.
	sh ${SRCDIR}/estimatecompare 2572 -S -h9 -0 ${CVND}${MPART}

	@#  Now estimate the size, after honoring the nodump flag.
	sh ${SRCDIR}/estimatecompare 347  -S -h0 -0 ${CVND}${MPART}
	@echo "***  Testing dump/restore support"
	mkdir ${TMPREST}
	dump -0 -h9 -B10000 -f - ${CVND}${MPART} | \
	    (cd ${TMPREST}; restore -rf -)
	@#restore should have created a restoresymtable file
	rm ${TMPREST}/restoresymtable
	diff -r -q ${TMPL} ${TMPREST}

fsck:
	@echo "*** checking fsck_ffs endian conversion."
	umount ${BVND}${MPART}
.if (${EN} == le)
	fsck_ffs -B be -y ${CVND}${MPART}
	mount -o async ${BVND}${MPART} ${TMPMP}
	${MAKECMD} cmp
	umount ${BVND}${MPART}
	fsck_ffs -B le -y ${CVND}${MPART}
	mount -o async ${BVND}${MPART} ${TMPMP}
	${MAKECMD} cmp
.else
	fsck_ffs -B le -y ${CVND}${MPART}
	mount -o async ${BVND}${MPART} ${TMPMP}
	${MAKECMD} cmp
	umount ${BVND}${MPART}
	fsck_ffs -B be -y ${CVND}${MPART}
	mount -o async ${BVND}${MPART} ${TMPMP}
	${MAKECMD} cmp
.endif

tmpfs:
	@echo "***  Creating a dummy directory tree at ${TMPMP} monted on" \
	    "${TMPIM}, ${EN} byte order."
	dd if=/dev/zero of=${TMPIM} count=5860
	vnconfig -v ${BVND}${RPART} ${TMPIM}
	disklabel -f ${SRCDIR}/disktab -rw ${VND} floppy288
	newfs -B ${EN} -i 500 -b 8192 -f 1024 ${CVND}${MPART}
	mkdir ${TMPMP}
	mount -o async ${BVND}${MPART} ${TMPMP}
	# Arg, cp will give an error if the symlink is copied before its target
	-cp -Rp ${TMPL}/* ${TMPMP}
	${MAKECMD} cmp

template:
	mkdir ${TMPL}
	@# Create a directory with a 10K file, with the file marked nodump.
	mkdir ${TMPL}/nodumpfile
	jot -r -c -s '' -n 10240 > ${TMPL}/nodumpfile/10k 
	chflags nodump ${TMPL}/nodumpfile/10k
	@# And some ordinary 10k files.
	mkdir ${TMPL}/dumpfile
	TMPFS_DUMP=${TMPL}/dumpfile/dumpfile.10k; \
	for f in ${DIGITS}; do \
	  jot -r -c -s '' -n 10240 > $$TMPFS_DUMP.$$f; done;
	@# A subdir with a short and a long symbolic link in it
	mkdir ${TMPL}/dumpfile/subdir
	ln -s ../dumpfile.10k.0 ${TMPL}/dumpfile/subdir/link;
	ln -s ../dumpfile.10k.1 ${TMPL}/dumpfile/subdir/really_really_really_really_really_really_long_name_to_use_up_space.link;
	@# And now the same files, but in a dir marked nodump:
	mkdir ${TMPL}/nodumpdir
	chflags nodump ${TMPL}/nodumpdir
	TMPFS_1=${TMPL}/nodumpdir/10k; \
	  for f in ${DIGITS}; do  \
	      jot -r -c -s '' -n 10240 > $$TMPFS_1.$$f; \
	  done
	
	@# Also create a large directory that uses more than one direct block
	@# (so it has to be larger than 8K).
	@# Make sure one entry is for a deleted file, also.
	mkdir ${TMPL}/2blkdir
	TMPFS_1=${TMPL}/2blkdir; \
	  jot -r -c -s '' -n 10 > $$TMPFS_1/10b; \
	  for f in ${DIGITS}; do for g in ${DIGITS}; do \
	    jot -r -c -s '' -n 10 > \
	      $$TMPFS_1/really_really_really_really_really_really_long_name_to_use_up_space_$$f$$g ;\
	    done; done; \
	  rm $$TMPFS_1/10b

	@# Now create a directory with at least one indirect block.
	@# On a FS with 8K blocks, we need at least 1 + 12*8192 bytes, or
	@#   98305 bytes, in the directory.  1000 files does the trick,
	@#   with the long filename below.
	mkdir ${TMPL}/indirblk
	chflags nodump ${TMPL}/indirblk
	TMPFS_1=${TMPL}/indirblk; \
	  jot -r -c -s '' -n 10 >  $$TMPFS_1/10b; \
	  for e in 0; do\
	  for f in ${DIGITS}; do for g in ${DIGITS}; do for h in ${DIGITS}; do \
	    jot -r -c -s '' -n 10 >  \
	      $$TMPFS_1/really_really_really_really_really_really_long_name_to_use_up_dir_entry_space$$e$$f$$g$$h ;\
	    done; done; done; done; \
	  rm $$TMPFS_1/10b
	@# ^---  As before, make sure at least one direntry is a deleted file.
