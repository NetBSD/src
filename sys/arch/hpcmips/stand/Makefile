# $NetBSD: Makefile,v 1.2 2000/01/12 19:19:16 cgd Exp $
#
# Makefile to generate windows VC++ project and workspace files from
# config files and templates

PROJECT_DIRS=libsa pbsdboot

winfiles: 
	for dir in $(PROJECT_DIRS); do \
	    $$dir/$$dir.config; \
	done
	sh dspgen/gen_workspace.sh hpcmips_stand.dsw $(PROJECT_DIRS)

# remote transient build-related files globally and for each project:
#	* global 'class view' information (.ncb)
#	* Debug and Release compile dirs,
#	* .plg files (build logs), and
#	* .[0-9][0-9][0-9] files (old versions of auto-converted .dsp files).

clean:
	rm -f hpcmips_stand.ncb
	for dir in $(PROJECT_DIRS); do \
	    (cd $$dir && rm -rf WMIPSDbg WMIPSRel); \
	    (cd $$dir && rm -f $$dir.plg $$dir.[0-9][0-9][0-9]); \
	done

# remove all files which windows is likely to have created, in addition
# to transient build-related files:
#	* .opt workspace option file
#	* (probably automatic) updates to the .dsp and .dsw files,
#	  which should be identical to the auto-generated versions when
#	  clean.

distclean cleandir: clean winfiles
	rm -f hpcmips_stand.opt
