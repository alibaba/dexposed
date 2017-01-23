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

public class Arm64 extends Assembly {

    @Override
    public int sizeOfDirectJump() {
        return 16;
    }

    @Override
    public byte[] createDirectJump(long targetAddress) {
        byte[] instructions = new byte[] {
                0x49, 0x00, 0x00, 0x58,         // ldr x9, _targetAddress
                0x20, 0x01, 0x1F, (byte) 0xD6,  // br x9
                0x00, 0x00, 0x00, 0x00,         // targetAddress
                0x00, 0x00, 0x00, 0x00          // targetAddress
        };
        writeLong(targetAddress, ByteOrder.LITTLE_ENDIAN, instructions, instructions.length - 8);
        return instructions;
    }

    @Override
    public int sizeOfTargetJump() {
        return 48;
    }

    @Override
    public byte[] createTargetJump(long targetAddress, long targetEntry, long srcAddress) {
        byte[] instructions = new byte[] {
                0x49, 0x01, 0x00, 0x58,        // ldr x9, _src_method_pos_x
                0x1F, 0x00, 0x09, (byte) 0xEB, // cmp x0, x9
                0x41, 0x01, 0x00, 0x54,        // bne _branch_1
                0x60, 0x00, 0x00, 0x58,        // ldr x0, _target_method_pos_x
                (byte) 0x89, 0x00, 0x00, 0x58, // ldr x9, _target_method_pc
                0x20, 0x01, 0x1F, (byte) 0xD6, // br x9
                0x00, 0x00, 0x00, 0x00,        // target_method_pos_x
                0x00, 0x00, 0x00, 0x00,        // target_method_pos_x
                0x00, 0x00, 0x00, 0x00,        // target_method_pc
                0x00, 0x00, 0x00, 0x00,        // target_method_pc
                0x00, 0x00, 0x00, 0x00,        // src_method_pos_x
                0x00, 0x00, 0x00, 0x00         // src_method_pos_x
        };
        writeLong(targetAddress, ByteOrder.LITTLE_ENDIAN, instructions, instructions.length - 24);
        writeLong(targetEntry, ByteOrder.LITTLE_ENDIAN, instructions, instructions.length - 16);
        writeLong(srcAddress, ByteOrder.LITTLE_ENDIAN, instructions, instructions.length - 8);
        return instructions;
    }

    @Override
    public byte[] createBridgeJump(long targetAddress, long srcAddress) {
        byte[] instructions = new byte[] {

                0x1f, 0x20, 0x03, (byte)0xd5,         // nop     case for recycle hook and get the wrong _src_method_pos_x
                0x1f, 0x20, 0x03, (byte)0xd5,         // nop
                0x1f, 0x20, 0x03, (byte)0xd5,         // nop
                0x1f, 0x20, 0x03, (byte)0xd5,         // nop

                0x49, 0x05, 0x00, 0x58,                         // ldr x9, _src_method_pos_x
                0x1f, 0x00, 0x09, (byte)0xeb,                   // cmp x0, x9
                0x41, 0x05, 0x00, 0x54,                         // bne _branch_1

                (byte)0xff, (byte)0x83, 0x03, (byte)0xd1,       // sub sp, sp, #0xe0
                (byte)0xe0, 0x07, 0x01, 0x6d,                   // stp d0, d1, [sp, #0x10]
                (byte)0xe2, 0x0f, 0x02, 0x6d,                   // stp d2, d3, [sp, #0x20]
                (byte)0xe4, 0x17, 0x03, 0x6d,                   // stp d4, d5, [sp, #0x30]
                (byte)0xe6, 0x1f, 0x04, 0x6d,                   // stp d6, d7, [sp, #0x40]
                (byte)0xe1, 0x0b, 0x05, (byte)0xa9,             // stp x1, x2, [sp, #0x50]
                (byte)0xe3, 0x13, 0x06, (byte)0xa9,             // stp x3, x4, [sp, #0x60]
                (byte)0xe5, 0x1b, 0x07, (byte)0xa9,             // stp x5, x6, [sp, #0x70]
                (byte)0xe7, 0x53, 0x08, (byte)0xa9,             // stp x7, x20, [sp, #0x80]
                (byte)0xf5, 0x5b, 0x09, (byte)0xa9,             // stp x21, x22, [sp, #0x90]
                (byte)0xf7, 0x63, 0x0a, (byte)0xa9,             // stp x23, x24, [sp, #0xa0]
                (byte)0xf9, 0x6b, 0x0b, (byte)0xa9,             // stp x25, x26, [sp, #0xb0]
                (byte)0xfb, 0x73, 0x0c, (byte)0xa9,             // stp x27, x28, [sp, #0xc0]
                (byte)0xfd, 0x7b, 0x0d, (byte)0xa9,             // stp x29, x30, [sp, #0xd0]

                (byte)0xf5, 0x03, 0x12, (byte)0xaa,             // mov        x21, x18
                (byte)0xe9, (byte)0x83, 0x03, (byte)0x91,       // add        x9, sp, #0xe0
                (byte)0xe9, 0x03, 0x00, (byte)0xf9,             // str        x9, [sp]
                (byte)0xf2, 0x07, 0x00, (byte)0xf9,             // str        x18, [sp, #0x8]

                //jump to unified hook entry
                0x69, 0x02, 0x00, 0x58,         // ldr x9, _targetAddress
                0x20, 0x01, 0x3f, (byte) 0xd6,  // blr x9

                (byte)0xf2, 0x03, 0x15, (byte)0xaa,             // mov        x18, x21
                (byte)0xe0, 0x07, 0x41, (byte)0x6d,             // ldp        d0, d1, [sp, #0x10]
                (byte)0xe2, 0x0f, 0x42, (byte)0x6d,             // ldp        d2, d3, [sp, #0x20]
                (byte)0xe4, 0x17, 0x43, (byte)0x6d,             // ldp        d4, d5, [sp, #0x30]
                (byte)0xe6, 0x1f, 0x44, (byte)0x6d,             // ldp        d6, d7, [sp, #0x40]
                (byte)0xe1, 0x0b, 0x45, (byte)0xa9,             // ldp        x1, x2, [sp, #0x50]
                (byte)0xe3, 0x13, 0x46, (byte)0xa9,             // ldp        x3, x4, [sp, #0x60]
                (byte)0xe5, 0x1b, 0x47, (byte)0xa9,             // ldp        x5, x6, [sp, #0x70]
                (byte)0xe7, 0x53, 0x48, (byte)0xa9,             // ldp        x7, x20, [sp, #0x80]
                (byte)0xf5, 0x5b, 0x49, (byte)0xa9,             // ldp        x21, x22, [sp, #0x90]
                (byte)0xf7, 0x63, 0x4a, (byte)0xa9,             // ldp        x23, x24, [sp, #0xa0]
                (byte)0xf9, 0x6b, 0x4b, (byte)0xa9,             // ldp        x25, x26, [sp, #0xb0]
                (byte)0xfb, 0x73, 0x4c, (byte)0xa9,             // ldp        x27, x28, [sp, #0xc0]
                (byte)0xfd, 0x7b, 0x4d, (byte)0xa9,             // ldp        x29, x30, [sp, #0xd0]

                (byte)0xff, (byte)0x83, 0x03, (byte)0x91,       // add        sp, sp, #0xe0
                0x00, 0x00, 0x67, (byte)0x9e,                   // fmov       d0, x0
                (byte)0xc0, 0x03, 0x5f, (byte)0xd6,             // ret

                0x00, 0x00, 0x00, 0x00,          // targetAddress
                0x00, 0x00, 0x00, 0x00,          // targetAddress

                0x00, 0x00, 0x00, 0x00,          // src_method_pos_x
                0x00, 0x00, 0x00, 0x00,          // src_method_pos_x


        };

        writeLong(targetAddress, ByteOrder.LITTLE_ENDIAN, instructions, instructions.length - 16);
        writeLong(srcAddress, ByteOrder.LITTLE_ENDIAN, instructions, instructions.length - 8);
        return instructions;
    }

    @Override
    public int sizeOfBridgeJump() {
        return (48)*4;
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
        return "64-bit ARM";
    }
}
