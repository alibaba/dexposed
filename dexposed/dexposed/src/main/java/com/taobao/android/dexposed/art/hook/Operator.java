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

import com.taobao.android.dexposed.utility.Debug;
import com.taobao.android.dexposed.art.Memory;
import com.taobao.android.dexposed.art.method.ArtMethod;

import java.lang.reflect.Method;

public abstract class Operator {

    public final ArtMethod source;
    public final ArtMethod target;

    public Operator(ArtMethod source, ArtMethod target){
        this.source = source;
        this.target = target;
        if(source != null) {
            long addr = Memory.getMethodAddress(source.getMethod());
            source.getMethod().setAccessible(true);
            Debug.logd("Operator", "source Method:" + Debug.longHex(addr));

            long originPointFromQuickCompiledCode = source.getEntryPointFromQuickCompiledCode();
            Debug.logd("Operator", "Orin Method QuickCompiledCode:" + Debug.longHex(originPointFromQuickCompiledCode));

        }
        if(target != null) {
            long addr = Memory.getMethodAddress(source.getMethod());
            target.getMethod().setAccessible(true);
            Debug.logd("Operator", "target Method:" + Debug.longHex(addr));

            long hookPointFromQuickCompiledCode = target.getEntryPointFromQuickCompiledCode();
            Debug.logd("Operator", "Hook Method QuickCompiledCode:" + Debug.longHex(hookPointFromQuickCompiledCode));
        }
    }

    public Method backupMethod(){
        Method method = source.backup().getMethod();
        method.setAccessible(true);
        return method;
    }

    public abstract boolean handle(Script script);
}
