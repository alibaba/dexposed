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

import com.taobao.android.dexposed.art.assembly.Assembly;
import com.taobao.android.dexposed.art.method.ArtMethod;


public class MethodContextR extends MethodContext {

    public MethodContextR(ArtMethod source, ArtMethod target) {
        super(source, target);
    }

    @Override
    public int size(Assembly assembly) {
        return assembly.sizeOfTargetJump();
    }

    public byte[] createScript(Assembly assembly){
        long targetAddress = target.getAddress();
        long targetEntry = target.getEntryPointFromQuickCompiledCode();
        long sourceAddress = source.getAddress();
        return assembly.createTargetJump(targetAddress, targetEntry, sourceAddress);
    }
}
