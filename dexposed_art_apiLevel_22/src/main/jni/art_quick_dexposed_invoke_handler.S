
#ifdef __arm__
#include "arch/arm/asm_support_arm.S"

.macro SETUP_REF_AND_ARGS_CALLEE_SAVE_FRAME
    push {r1-r3, r5-r8, r10-r11, lr}  @ 10 words of callee saves
    .save {r1-r3, r5-r8, r10-r11, lr}
    .cfi_rel_offset r1, 0
    .cfi_rel_offset r2, 4
    .cfi_rel_offset r3, 8
    .cfi_rel_offset r5, 12
    .cfi_rel_offset r6, 16
    .cfi_rel_offset r7, 20
    .cfi_rel_offset r8, 24
    .cfi_rel_offset r10, 28
    .cfi_rel_offset r11, 32
    .cfi_rel_offset lr, 36
    .cfi_adjust_cfa_offset 40
    sub sp, #8                        @ 2 words of space, bottom word will hold Method*
    .pad #8
    .cfi_adjust_cfa_offset 8
    // Ugly compile-time check, but we only have the preprocessor.
#if (FRAME_SIZE_REFS_AND_ARGS_CALLEE_SAVE != 40 + 8)
#error "REFS_AND_ARGS_CALLEE_SAVE_FRAME(ARM) size not as expected."
#endif
.endm

.macro SETUP_SAVE_ALL_CALLEE_SAVE_FRAME
    push {r4-r11, lr} @ 9 words of callee saves
    .save {r4-r11, lr}
    .cfi_adjust_cfa_offset 36
    .cfi_rel_offset r4, 0
    .cfi_rel_offset r5, 4
    .cfi_rel_offset r6, 8
    .cfi_rel_offset r7, 12
    .cfi_rel_offset r8, 16
    .cfi_rel_offset r9, 20
    .cfi_rel_offset r10, 24
    .cfi_rel_offset r11, 28
    .cfi_rel_offset lr, 32
    vpush {s0-s31}
    .pad #128
    .cfi_adjust_cfa_offset 128
    sub sp, #12       @ 3 words of space, bottom word will hold Method*
    .pad #12
    .cfi_adjust_cfa_offset 12

     // Ugly compile-time check, but we only have the preprocessor.
#if (FRAME_SIZE_SAVE_ALL_CALLEE_SAVE != 36 + 128 + 12)
#error "SAVE_ALL_CALLEE_SAVE_FRAME(ARM) size not as expected."
#endif
.endm

.macro RESTORE_REF_ONLY_CALLEE_SAVE_FRAME
    add sp, #4               @ bottom word holds Method*
    pop {r5-r8, r10-r11, lr} @ 7 words of callee saves
    .cfi_restore r5
    .cfi_restore r6
    .cfi_restore r7
    .cfi_restore r8
    .cfi_restore r10
    .cfi_restore r11
    .cfi_adjust_cfa_offset -32
.endm

    /*
     * Macro that set calls through to artDeliverPendingExceptionFromCode, where the pending
     * exception is Thread::Current()->exception_
     */
.macro DELIVER_PENDING_EXCEPTION
    .fnend
    .fnstart
    SETUP_SAVE_ALL_CALLEE_SAVE_FRAME           @ save callee saves for throw
    mov    r0, r9                              @ pass Thread::Current
    mov    r1, sp                              @ pass SP
    b      artDeliverPendingExceptionFromCode  @ artDeliverPendingExceptionFromCode(Thread*, SP)
.endm

     .extern artQuickDexposedInvokeHandler
ENTRY art_quick_dexposed_invoke_handler
    SETUP_REF_AND_ARGS_CALLEE_SAVE_FRAME
    str     r0, [sp, #0]           @ place proxy method at bottom of frame
    mov     r2, r9                 @ pass Thread::Current
    mov     r3, sp                 @ pass SP
    blx     artQuickDexposedInvokeHandler  @ (Method* proxy method, receiver, Thread*, SP)
    ldr     r2, [r9, #THREAD_EXCEPTION_OFFSET]  @ load Thread::Current()->exception_
    add     sp, #16                @ skip r1-r3, 4 bytes padding.
    .cfi_adjust_cfa_offset -16
    cbnz    r2, 1f                 @ success if no exception is pending
    RESTORE_REF_ONLY_CALLEE_SAVE_FRAME
    bx      lr                     @ return on success
1:
    RESTORE_REF_ONLY_CALLEE_SAVE_FRAME
    DELIVER_PENDING_EXCEPTION
END art_quick_dexposed_invoke_handler

#else

#include "asm_support_x86.S"

// For x86, the CFA is esp+4, the address above the pushed return address on the stack.

    /*
     * Macro that sets up the callee save frame to conform with
     * Runtime::CreateCalleeSaveMethod(kSaveAll)
     */
MACRO0(SETUP_SAVE_ALL_CALLEE_SAVE_FRAME)
    PUSH edi  // Save callee saves (ebx is saved/restored by the upcall)
    PUSH esi
    PUSH ebp
    subl  MACRO_LITERAL(16), %esp  // Grow stack by 4 words, bottom word will hold Method*
    CFI_ADJUST_CFA_OFFSET(16)
    // Ugly compile-time check, but we only have the preprocessor.
    // Last +4: implicit return address pushed on stack when caller made call.
#if (FRAME_SIZE_SAVE_ALL_CALLEE_SAVE != 3*4 + 16 + 4)
#error "SAVE_ALL_CALLEE_SAVE_FRAME(X86) size not as expected."
#endif
END_MACRO


    /*
     * Macro that sets up the callee save frame to conform with
     * Runtime::CreateCalleeSaveMethod(kRefsOnly)
     */
MACRO0(SETUP_REF_ONLY_CALLEE_SAVE_FRAME)
    PUSH edi  // Save callee saves (ebx is saved/restored by the upcall)
    PUSH esi
    PUSH ebp
    subl  MACRO_LITERAL(16), %esp  // Grow stack by 4 words, bottom word will hold Method*
    CFI_ADJUST_CFA_OFFSET(16)

    // Ugly compile-time check, but we only have the preprocessor.
    // Last +4: implicit return address pushed on stack when caller made call.
#if (FRAME_SIZE_REFS_ONLY_CALLEE_SAVE != 3*4 + 16 + 4)
#error "REFS_ONLY_CALLEE_SAVE_FRAME(X86) size not as expected."
#endif
END_MACRO

MACRO0(RESTORE_REF_ONLY_CALLEE_SAVE_FRAME)
    addl MACRO_LITERAL(16), %esp  // Unwind stack up to saved values
    CFI_ADJUST_CFA_OFFSET(-16)
    POP ebp  // Restore callee saves (ebx is saved/restored by the upcall)
    POP esi
    POP edi
END_MACRO

    /*
     * Macro that sets up the callee save frame to conform with
     * Runtime::CreateCalleeSaveMethod(kRefsAndArgs)
     */
MACRO0(SETUP_REF_AND_ARGS_CALLEE_SAVE_FRAME)
    PUSH edi  // Save callee saves
    PUSH esi
    PUSH ebp
    PUSH ebx  // Save args
    PUSH edx
    PUSH ecx
    PUSH eax   // Align stack, eax will be clobbered by Method*

    // Ugly compile-time check, but we only have the preprocessor.
    // Last +4: implicit return address pushed on stack when caller made call.
#if (FRAME_SIZE_REFS_AND_ARGS_CALLEE_SAVE != 7*4 + 4)
#error "REFS_AND_ARGS_CALLEE_SAVE_FRAME(X86) size not as expected."
#endif
END_MACRO


MACRO0(RESTORE_REF_AND_ARGS_CALLEE_SAVE_FRAME)
    addl MACRO_LITERAL(4), %esp  // Remove padding
    CFI_ADJUST_CFA_OFFSET(-4)
    POP ecx  // Restore args except eax
    POP edx
    POP ebx
    POP ebp  // Restore callee saves
    POP esi
    POP edi
END_MACRO

    /*
     * Macro that set calls through to artDeliverPendingExceptionFromCode, where the pending
     * exception is Thread::Current()->exception_.
     */
MACRO0(DELIVER_PENDING_EXCEPTION)
    SETUP_SAVE_ALL_CALLEE_SAVE_FRAME         // save callee saves for throw
    mov %esp, %ecx
    // Outgoing argument set up
    subl  MACRO_LITERAL(8), %esp             // Alignment padding
    CFI_ADJUST_CFA_OFFSET(8)
    PUSH ecx                                 // pass SP
    pushl %fs:THREAD_SELF_OFFSET             // pass Thread::Current()
    CFI_ADJUST_CFA_OFFSET(4)
    SETUP_GOT_NOSAVE                         // clobbers ebx (harmless here)
    call SYMBOL(artDeliverPendingExceptionFromCode)  // artDeliverPendingExceptionFromCode(Thread*, SP)
    int3                                     // unreached
END_MACRO

MACRO0(RETURN_OR_DELIVER_PENDING_EXCEPTION)
    mov %fs:THREAD_EXCEPTION_OFFSET, %ebx // get exception field
    testl %ebx, %ebx               // ebx == 0 ?
    jnz 1f                         // if ebx != 0 goto 1
    ret                            // return
1:                                 // deliver exception on current thread
    DELIVER_PENDING_EXCEPTION
END_MACRO

	.extern artQuickDexposedInvokeHandler
DEFINE_FUNCTION art_quick_dexposed_invoke_handler
    SETUP_REF_AND_ARGS_CALLEE_SAVE_FRAME   // save frame and Method*
    PUSH esp                      // pass SP
    pushl %fs:THREAD_SELF_OFFSET  // pass Thread::Current()
    CFI_ADJUST_CFA_OFFSET(4)
    PUSH ecx                      // pass receiver
    PUSH eax                      // pass proxy method
    SETUP_GOT_NOSAVE              // clobbers EBX
    call SYMBOL(artQuickDexposedInvokeHandler) // (proxy method, receiver, Thread*, SP)
    movd %eax, %xmm0              // place return value also into floating point return value
    movd %edx, %xmm1
    punpckldq %xmm1, %xmm0
    addl LITERAL(44), %esp        // pop arguments
    CFI_ADJUST_CFA_OFFSET(-44)
    RETURN_OR_DELIVER_PENDING_EXCEPTION    // return or deliver exception
END_FUNCTION art_quick_dexposed_invoke_handler

#endif
