/*
 * Copyright (C) 2010 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * machine/setjmp.h: machine dependent setjmp-related information.
 */

/* _JBLEN is the size of a jmp_buf in longs.
 * Do not modify this value or you will break the ABI !
 *
 * This value comes from the original OpenBSD ARM-specific header
 * that was replaced by this one.
 */
#define _JBLEN  64

/* According to the ARM AAPCS document, we only need to save
 * the following registers:
 *
 *  Core   r4-r14
 *
 *  VFP    d8-d15  (see section 5.1.2.1)
 *
 *      Registers s16-s31 (d8-d15, q4-q7) must be preserved across subroutine
 *      calls; registers s0-s15 (d0-d7, q0-q3) do not need to be preserved
 *      (and can be used for passing arguments or returning results in standard
 *      procedure-call variants). Registers d16-d31 (q8-q15), if present, do
 *      not need to be preserved.
 *
 *  FPSCR  saved because GLibc does saves it too.
 *
 */

/* The internal structure of a jmp_buf is totally private.
 * Current layout (may change in the future):
 *
 * word   name         description
 * 0      magic        magic number
 * 1      sigmask      signal mask (not used with _setjmp / _longjmp)
 * 2      float_base   base of float registers (d8 to d15)
 * 18     float_state  floating-point status and control register
 * 19     core_base    base of core registers (r4 to r14)
 * 30     reserved     reserved entries (room to grow)
 * 64
 *
 * NOTE: float_base must be at an even word index, since the
 *       FP registers will be loaded/stored with instructions
 *       that expect 8-byte alignment.
 */

#define _JB_MAGIC       0
#define _JB_SIGMASK     (_JB_MAGIC+1)
#define _JB_FLOAT_BASE  (_JB_SIGMASK+1)
#define _JB_FLOAT_STATE (_JB_FLOAT_BASE + (15-8+1)*2)
#define _JB_CORE_BASE   (_JB_FLOAT_STATE+1)

#define _JB_MAGIC__SETJMP	0x4278f500
#define _JB_MAGIC_SETJMP	0x4278f501
