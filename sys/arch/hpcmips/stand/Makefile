# $NetBSD: Makefile,v 1.1 2000/01/12 02:33:56 cgd Exp $
#
# Makefile to generate windows VC++ project and workspace files from
# config files and templates

PROJECT_DIRS=libsa pbsdboot

winfiles: 
	for dir in $(PROJECT_DIRS); do \
	    $$dir/$$dir.config; \
	done
	sh dspgen/gen_workspace.sh hpcmips_stand.dsw $(PROJECT_DIRS)
