/*
 * Original work Copyright (c) 2016, Alibaba Mobile Infrastructure (Android) Team
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

import android.util.Log;

import com.taobao.android.dexposed.utility.Debug;
import com.taobao.android.dexposed.art.Memory;
import com.taobao.android.dexposed.art.assembly.Assembly;
import com.taobao.android.dexposed.art.method.ArtMethod;


public class MethodContextU extends MethodContext {

    public MethodContextU(ArtMethod source) {
        super(source, null);
    }

    @Override
    public int size(Assembly assembly) {
        return assembly.sizeOfBridgeJump();
    }

    public byte[] createScript(Assembly assembly){
        long targetAddress = Memory.getBridgeFunction();
        long sourceAddress = source.getAddress();
        Debug.logd("targetAddress:"+ Debug.longHex(targetAddress));
        Debug.logd("sourceAddress:"+ Debug.longHex(sourceAddress));

        return assembly.createBridgeJump(targetAddress, sourceAddress);
    }
}
