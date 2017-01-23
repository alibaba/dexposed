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

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public abstract class Assembly {

    public abstract byte[] createDirectJump(long targetAddress);

    public abstract int sizeOfDirectJump();

    public abstract long toPC(long code);

    public abstract long toMem(long pc);

    public byte[] createCallOriginal(long originalAddress, byte[] originalPrologue) {
        byte[] callOriginal = new byte[sizeOfCallOriginal()];
        System.arraycopy(originalPrologue, 0, callOriginal, 0, sizeOfDirectJump());
        byte[] directJump = createDirectJump(toPC(originalAddress + sizeOfDirectJump()));
        System.arraycopy(directJump, 0, callOriginal, sizeOfDirectJump(), sizeOfDirectJump());
        return callOriginal;
    }

    public int sizeOfCallOriginal() {
        return sizeOfDirectJump() * 2;
    }

    public abstract byte[] createTargetJump(long targetAddress, long targetEntry, long srcAddress);

    public abstract int sizeOfTargetJump();

    public abstract byte[] createBridgeJump(long targetAddress, long srcAddress);

    public abstract int sizeOfBridgeJump();

    // Helpers
    protected static void writeInt(int i, ByteOrder order, byte[] target, int pos) {
        System.arraycopy(ByteBuffer.allocate(4).order(order).putInt(i).array(), 0, target, pos, 4);
    }

    protected static void writeLong(long i, ByteOrder order, byte[] target, int pos) {
        System.arraycopy(ByteBuffer.allocate(8).order(order).putLong(i).array(), 0, target, pos, 8);
    }

    public abstract String getName();
}
