/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ART_RUNTIME_ARCH_X86_ASM_SUPPORT_X86_S_
#define ART_RUNTIME_ARCH_X86_ASM_SUPPORT_X86_S_

#include "asm_support_x86.h"

#if defined(__APPLE__) || (defined(__clang__) && (__clang_major__ < 4) && (__clang_minor__ < 5))
    // Clang's as(1) doesn't let you name macro parameters prior to 3.5.
    #define MACRO0(macro_name) .macro macro_name
    #define MACRO1(macro_name, macro_arg1) .macro macro_name
    #define MACRO2(macro_name, macro_arg1, macro_args2) .macro macro_name
    #define MACRO3(macro_name, macro_arg1, macro_args2, macro_args3) .macro macro_name
    #define END_MACRO .endmacro

    // Clang's as(1) uses $0, $1, and so on for macro arguments.
    #define RAW_VAR(name,index) $index
    #define VAR(name,index) SYMBOL($index)
    #define PLT_VAR(name, index) SYMBOL($index)
    #define REG_VAR(name,index) %$index
    #define CALL_MACRO(name,index) $index

    //  The use of $x for arguments mean that literals need to be represented with $$x in macros.
    #define LITERAL(value) $value
    #define MACRO_LITERAL(value) $$value
#else
    // Regular gas(1) lets you name macro parameters.
    #define MACRO0(macro_name) .macro macro_name
    #define MACRO1(macro_name, macro_arg1) .macro macro_name macro_arg1
    #define MACRO2(macro_name, macro_arg1, macro_arg2) .macro macro_name macro_arg1, macro_arg2
    #define MACRO3(macro_name, macro_arg1, macro_arg2, macro_arg3) .macro macro_name macro_arg1, macro_arg2, macro_arg3
    #define END_MACRO .endm

    // Regular gas(1) uses \argument_name for macro arguments.
    // We need to turn on alternate macro syntax so we can use & instead or the preprocessor
    // will screw us by inserting a space between the \ and the name. Even in this mode there's
    // no special meaning to $, so literals are still just $x. The use of altmacro means % is a
    // special character meaning care needs to be taken when passing registers as macro arguments.
    .altmacro
    #define RAW_VAR(name,index) name&
    #define VAR(name,index) name&
    #define PLT_VAR(name, index) name&@PLT
    #define REG_VAR(name,index) %name
    #define CALL_MACRO(name,index) name&

    #define LITERAL(value) $value
    #define MACRO_LITERAL(value) $value
#endif

#if defined(__APPLE__)
    #define FUNCTION_TYPE(name,index)
    #define SIZE(name,index)
#elif defined(__clang__) && (__clang_major__ < 4) && (__clang_minor__ < 5)
    #define FUNCTION_TYPE(name,index) .type $index, @function
    #define SIZE(name,index) .size $index, .-$index
#else
    #define FUNCTION_TYPE(name,index) .type name&, @function
    #define SIZE(name,index) .size name, .-name
#endif

    // CFI support.
#if !defined(__APPLE__)
    #define CFI_STARTPROC .cfi_startproc
    #define CFI_ENDPROC .cfi_endproc
    #define CFI_ADJUST_CFA_OFFSET(size) .cfi_adjust_cfa_offset size
    #define CFI_DEF_CFA(reg,size) .cfi_def_cfa reg,size
    #define CFI_DEF_CFA_REGISTER(reg) .cfi_def_cfa_register reg
    #define CFI_RESTORE(reg) .cfi_restore reg
    #define CFI_REL_OFFSET(reg,size) .cfi_rel_offset reg,size
    #define CFI_RESTORE_STATE .cfi_restore_state
    #define CFI_REMEMBER_STATE .cfi_remember_state
#else
    // Mac OS' doesn't like cfi_* directives.
    #define CFI_STARTPROC
    #define CFI_ENDPROC
    #define CFI_ADJUST_CFA_OFFSET(size)
    #define CFI_DEF_CFA(reg,size)
    #define CFI_DEF_CFA_REGISTER(reg)
    #define CFI_RESTORE(reg)
    #define CFI_REL_OFFSET(reg,size)
    #define CFI_RESTORE_STATE
    #define CFI_REMEMBER_STATE
#endif

    // Symbols.
#if !defined(__APPLE__)
    #define SYMBOL(name) name
    #if defined(__clang__) && (__clang_major__ < 4) && (__clang_minor__ < 5)
        // TODO: Disabled for old clang 3.3, this leads to text relocations and there should be a
        // better fix.
        #define PLT_SYMBOL(name) name // ## @PLT
    #else
        #define PLT_SYMBOL(name) name ## @PLT
    #endif
#else
    // Mac OS' symbols have an _ prefix.
    #define SYMBOL(name) _ ## name
    #define PLT_SYMBOL(name) _ ## name
#endif

// Directive to hide a function symbol.
#if defined(__APPLE__)
    #define ASM_HIDDEN .private_extern
#else
    #define ASM_HIDDEN .hidden
#endif

    /* Cache alignment for function entry */
MACRO0(ALIGN_FUNCTION_ENTRY)
    .balign 16
END_MACRO

MACRO1(DEFINE_FUNCTION, c_name)
    FUNCTION_TYPE(\c_name, 0)
    ASM_HIDDEN VAR(c_name, 0)
    .globl VAR(c_name, 0)
    ALIGN_FUNCTION_ENTRY
VAR(c_name, 0):
    CFI_STARTPROC
    // Ensure we get a sane starting CFA.
    CFI_DEF_CFA(esp, 4)
END_MACRO

MACRO1(DEFINE_FUNCTION_NO_HIDE, c_name)
    FUNCTION_TYPE(\c_name, 0)
    .globl VAR(c_name, 0)
    ALIGN_FUNCTION_ENTRY
VAR(c_name, 0):
    CFI_STARTPROC
    // Ensure we get a sane starting CFA.
    CFI_DEF_CFA(esp, 4)
END_MACRO

MACRO1(END_FUNCTION, c_name)
    CFI_ENDPROC
    SIZE(\c_name, 0)
END_MACRO

MACRO1(PUSH, reg)
    pushl REG_VAR(reg, 0)
    CFI_ADJUST_CFA_OFFSET(4)
    CFI_REL_OFFSET(REG_VAR(reg, 0), 0)
END_MACRO

MACRO1(POP, reg)
    popl REG_VAR(reg,0)
    CFI_ADJUST_CFA_OFFSET(-4)
    CFI_RESTORE(REG_VAR(reg,0))
END_MACRO

MACRO1(UNIMPLEMENTED,name)
    FUNCTION_TYPE(\name, 0)
    .globl VAR(name, 0)
    ALIGN_FUNCTION_ENTRY
VAR(name, 0):
    CFI_STARTPROC
    int3
    int3
    CFI_ENDPROC
    SIZE(\name, 0)
END_MACRO

MACRO0(SETUP_GOT_NOSAVE)
#ifndef __APPLE__
    call __x86.get_pc_thunk.bx
    addl $_GLOBAL_OFFSET_TABLE_, %ebx
#endif
END_MACRO

MACRO0(SETUP_GOT)
    PUSH  ebx
    SETUP_GOT_NOSAVE
END_MACRO

MACRO0(UNDO_SETUP_GOT)
    POP  ebx
END_MACRO

#endif  // ART_RUNTIME_ARCH_X86_ASM_SUPPORT_X86_S_
