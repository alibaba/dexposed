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

public class Arm32 extends Assembly {

    @Override
    public int sizeOfDirectJump() {
        return 8;
    }

    @Override
    public byte[] createDirectJump(long targetAddress) {
        byte[] instructions = new byte[] {
                0x04, (byte) 0xf0, 0x1f, (byte) 0xe5,   // ldr pc, [pc, #-4]
                0, 0, 0, 0                              // .int 0x0
        };
        writeInt((int) targetAddress, ByteOrder.LITTLE_ENDIAN, instructions,
                instructions.length - 4);
        return instructions;
    }

    @Override
    public int sizeOfTargetJump() {
        return 32;
    }

    @Override
    public byte[] createTargetJump(long targetAddress, long targetEntry, long srcAddress) {
        byte[] instructions = new byte[] {
                0x14, (byte) 0xc0, (byte) 0x9f, (byte) 0xe5,    // ldr ip, [pc, #20]
                0x0c, 0x00, 0x50, (byte) 0xe1,                  // cmp r0, ip
                0x04, 0x00, 0x00, 0x1a,                         // bne next
                0x00, 0x00, (byte) 0x9f, (byte) 0xe5,           // ldr r0, [pc, #4]
                0x04, (byte) 0xf0, (byte) 0x9f, (byte) 0xe5,    // ldr pc, [pc, #4]
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

        byte[] instructions = new byte[]{
                0x00, (byte) 0xf0, 0x20, (byte) 0xe3, //nop
                0x34, (byte) 0xc0, (byte) 0x9f, (byte) 0xe5,    // ldr ip, [pc, #52]

                0x0c, 0x00, 0x50, (byte) 0xe1,                  // cmp r0, ip
                0x0c, 0x00, 0x00, 0x1a,                         // bne next

                (byte)0xf8, 0x4f, 0x2d, (byte)0xe9,       //push     {r3, r4, r5, r6, r7, r8, sb, sl, fp, lr}
                0x10, (byte)0xd0, 0x4d, (byte)0xe2,       // sub sp, #0x10

                0x08, 0x30, (byte)0x8d, (byte)0xe5,       // str r3, [sp, #8]
                0x0d, 0x30, (byte)0xa0, (byte)0xe1,       //mov r3, sp

                0x00, (byte)0xd0, (byte)0x8d, (byte)0xe5,       // str sp, [sp]
                0x04, (byte)0x90, (byte)0x8d, (byte)0xe5,       // str r9, [sp, #4]

                0x00, (byte)0xe0, (byte)0x9f, (byte)0xe5,       // ldr lr, [pc]
                0x08, (byte)0xc0, (byte)0x9f, (byte)0xe5,       // ldr ip, [pc, #8]
                0x1c, (byte)0xff, 0x2f, (byte)0xe1,       // bx ip

                0x10, (byte)0xd0, (byte)0x8d, (byte)0xe2,       //add sp, #0x10
                (byte)0xf8, (byte)0x8f, (byte)0xbd, (byte)0xe8,       // pop  {r3, r4, r5, r6, r7, r8, sb, sl, fp, pc}

                0x0, 0x0, 0x0, 0x0,           // target_address
                0x0, 0x0, 0x0, 0x0,           // src_method_pos_x
        };

        writeInt((int) targetAddress,
                ByteOrder.LITTLE_ENDIAN, instructions, instructions.length - 8);
        writeInt((int) srcAddress, ByteOrder.LITTLE_ENDIAN, instructions,
                instructions.length - 4);
        return instructions;
    }

    @Override
    public int sizeOfBridgeJump() {
        return 17*4;
    }

    @Override
    public long toPC(long code) {
        return code;
    }

    @Override
    public long toMem(long pc) {
        return pc;
    }

    @Override
    public String getName() {
        return "32-bit ARM";
    }
}
