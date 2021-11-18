/* $NetBSD: lsym_preprocessing.c,v 1.1 2021/11/18 21:19:19 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the token lsym_preprocessing, which represents a '#' that starts
 * a preprocessing line.
 *
 * The whole preprocessing line is processed separately from the main source
 * code, without much tokenizing or parsing.
 */

// TODO: test '#' in the middle of a non-preprocessing line
// TODO: test stringify '#'
// TODO: test token paste '##'

#indent input
// TODO: add input
#indent end

#indent run-equals-input
