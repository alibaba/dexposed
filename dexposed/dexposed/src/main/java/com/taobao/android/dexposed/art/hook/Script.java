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

package com.taobao.android.dexposed.art.hook;

import com.taobao.android.dexposed.art.Memory;
import com.taobao.android.dexposed.art.assembly.Assembly;
import com.taobao.android.dexposed.utility.Debug;

import java.util.HashSet;
import java.util.Set;

public class Script {
    private final Assembly assembly;
    private final long originalAddress;
    private final byte[] originalCode;
    private final Set<MethodContext> methodContexts = new HashSet<>();
    private int allocatedSize;
    private long allocatedAddress;
    private boolean active;

    public Script(Assembly assembly, long originalAddress) {
        this.assembly = assembly;
        this.originalAddress = originalAddress;

        this.originalCode = Memory.get(originalAddress, assembly.sizeOfDirectJump());
    }

    public void addMethodContext(MethodContext methodContext) {
        methodContexts.add(methodContext);
    }

    private long getBaseAddress() {
        if (getSize() != allocatedSize) {
            allocate();
        }
        return allocatedAddress;
    }

    public long getCallHook() {
        return assembly.toPC(getBaseAddress());
    }

    private void allocate() {
        if (allocatedAddress != 0)
            deallocate();
        allocatedSize = getSize();
        allocatedAddress = Memory.map(allocatedSize);
    }

    private void deallocate() {
        if (allocatedAddress != 0) {
            Memory.unmap(allocatedAddress, allocatedSize);
            allocatedAddress = 0;
            allocatedSize = 0;
        }

        if (active) {
            Memory.put(originalCode, originalAddress);
        }
    }

    public int getSize() {

        int count = 0;
        for(MethodContext c : methodContexts){
            count += c.size(assembly);
        }

        count += assembly.sizeOfCallOriginal();
        return count;
    }

    public byte[] create() {
        byte[] mainPage = new byte[getSize()];
        int offset = 0;

        for (MethodContext mc : methodContexts) {
            byte[] script = mc.createScript(assembly);
            System.arraycopy(script, 0, mainPage, offset, script.length);
            offset += script.length;
        }

        byte[] callOriginal = assembly.createCallOriginal(originalAddress, originalCode);
        System.arraycopy(callOriginal, 0, mainPage, offset, callOriginal.length);

        return mainPage;
    }

    public void update() {
        byte[] page = create();
        Memory.put(page, getBaseAddress());
    }

    public boolean activate() {
        Debug.logd("Writing hook to " + Debug.addrHex(getCallHook()) + " in " + Debug.addrHex(originalAddress));
        boolean result = Memory.unprotect(originalAddress, assembly.sizeOfDirectJump());
        if (result) {
            Memory.put(assembly.createDirectJump(getCallHook()), originalAddress);
            active = true;
            return true;
        } else {
            Debug.logw("Writing hook failed: Unable to unprotect memory at " + Debug.addrHex(originalAddress) + "!");
            active = false;
            return false;
        }
    }

    @Override
    protected void finalize() throws Throwable {
        deallocate();
        super.finalize();
    }
}
