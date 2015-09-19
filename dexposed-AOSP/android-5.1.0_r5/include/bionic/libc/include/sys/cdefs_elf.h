/*	$NetBSD: cdefs_elf.h,v 1.22 2005/02/26 22:25:34 perry Exp $	*/

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#ifndef _SYS_CDEFS_ELF_H_
#define	_SYS_CDEFS_ELF_H_

#define __strong_alias(alias, sym) \
    __asm__(".global " #alias "\n" \
            #alias " = " #sym);

#define __weak_alias(alias,sym) \
    __asm__(".weak " #alias "\n" \
            #alias " = " #sym);

/* We use __warnattr instead of __warn_references.
 * TODO: remove this and put an empty definition in one of the upstream-* compatibility headers.
 */
#define	__warn_references(sym,msg)					\
    /*__asm__(".section .gnu.warning." #sym "\n\t.ascii \"" msg "\"\n\t.text");*/

#endif /* !_SYS_CDEFS_ELF_H_ */
