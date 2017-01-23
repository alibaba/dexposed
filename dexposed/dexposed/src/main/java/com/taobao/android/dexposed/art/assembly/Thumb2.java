/*
 * Original work Copyright (c) 2014-2015, Marvin Wißfeld
 * Modified work Copyright (c) 2016, Alibaba Mobile Infrastructure (Android) Team
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

package com.taobao.android.dexposed.art.assembly;

import java.nio.ByteOrder;


public class Thumb2 extends Assembly {

    @Override
    public int sizeOfDirectJump() {
        return 8;
    }

    @Override
    public byte[] createDirectJump(long targetAddress) {
        byte[] instructions = new byte[] {
                (byte) 0xdf, (byte) 0xf8, 0x00, (byte) 0xf0,        // ldr pc, [pc]
                0, 0, 0, 0
        };
        writeInt((int) targetAddress, ByteOrder.LITTLE_ENDIAN, instructions,
                instructions.length - 4);
        return instructions;
    }

    @Override
    public int sizeOfTargetJump() {
        return 28;
    }

    @Override
    public byte[] createTargetJump(long targetAddress, long targetEntry, long srcAddress) {
        byte[] instructions = new byte[] {
                (byte) 0xdf, (byte) 0xf8, 0x14, (byte) 0xc0,    // ldr ip, [pc, #20]
                (byte) 0x60, 0x45,                              // cmp r0, ip
                0x40, (byte) 0xf0, 0x09, (byte) 0x80,           // bne next
                0x01, 0x48,                                     // ldr r0, [pc, #4]
                (byte) 0xdf, (byte) 0xf8, 0x04, (byte) 0xf0,    // ldr pc, [pc, #4]
                0x0, 0x0, 0x0, 0x0,                             // target_method_pos_x
                0x0, 0x0, 0x0, 0x0,                             // target_method_pc
                0x0, 0x0, 0x0, 0x0,                             // src_method_pos_x
        };
        writeInt((int) targetAddress, ByteOrder.LITTLE_ENDIAN, instructions,
                instructions.length - 12);
        writeInt((int) targetEntry,
                ByteOrder.LITTLE_ENDIAN, instructions, instructions.length - 8);
        writeInt((int) srcAddress, ByteOrder.LITTLE_ENDIAN, instructions,
                instructions.length - 4);
        return instructions;
    }

    @Override
    public byte[] createBridgeJump(long targetAddress, long srcAddress) {
        byte[] instructions = new byte[] {

                (byte)0xaf, (byte)0xf3, 0x00, (byte)0x80, //nop

                (byte) 0xdf, (byte) 0xf8, 0x2c, (byte) 0xc0,    // ldr ip, [pc, #20]
                (byte) 0x60, 0x45,                              // cmp r0, ip
                0x40, (byte) 0xf0, 0x15, (byte) 0x80,           // bne next   ?

                0x2d, (byte)0xe9, (byte)0xf8, 0x4f,             //push.w     {r3, r4, r5, r6, r7, r8, sb, sl, fp, lr}
                (byte)0x84, (byte)0xb0,         // sub sp, #0x10

                (byte)0xcd, (byte)0xf8, 0x08, (byte)0x30,       // str r3, [sp, #8]
                0x6B, 0x46, //mov r3, sp
                0x6B, 0x46, //mov r3, sp

                (byte)0xcd, (byte)0xf8, 0x00, (byte)0xd0,       // str sp, [sp]
                (byte)0xcd, (byte)0xf8, 0x04, (byte)0x90,       // str r9, [sp, #4]

                (byte)0xdf, (byte)0xf8, 0x08, (byte)0xc0,       // ldr ip, [pc, #8]
                (byte)0xe0, 0x47,// blx ip

                (byte)0x04, (byte)0xb0,//add sp, #0x10
                (byte)0xbd, (byte)0xe8, (byte)0xf8, (byte)0x8f, // pop.w  {r3, r4, r5, r6, r7, r8, sb, sl, fp, pc}

                0x0, 0x0, 0x0, 0x0,                             // target_address
                0x0, 0x0, 0x0, 0x0,                             // src_method_pos_x
        };

        writeInt((int) targetAddress,
                ByteOrder.LITTLE_ENDIAN, instructions, instructions.length - 8);
        writeInt((int) srcAddress, ByteOrder.LITTLE_ENDIAN, instructions,
                instructions.length - 4);

        return instructions;
    }

    @Override
    public int sizeOfBridgeJump() {
        return 14*4;
    }

    @Override
    public long toPC(long code) {
        return toMem(code) + 1;
    }

    @Override
    public long toMem(long pc) {
        return pc & ~0x1;
    }

    @Override
    public String getName() {
        return "16/32-bit Thumb2";
    }
}
