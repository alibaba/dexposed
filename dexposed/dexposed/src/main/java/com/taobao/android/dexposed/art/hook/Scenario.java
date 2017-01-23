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

import com.taobao.android.dexposed.art.Memory;
import com.taobao.android.dexposed.art.assembly.Arm32;
import com.taobao.android.dexposed.art.assembly.Arm64;
import com.taobao.android.dexposed.art.assembly.Assembly;
import com.taobao.android.dexposed.art.assembly.Thumb2;
import com.taobao.android.dexposed.art.method.ArtMethod;
import com.taobao.android.dexposed.utility.Debug;
import com.taobao.android.dexposed.utility.Runtime;

import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import static com.taobao.android.dexposed.utility.Debug.logd;

public final class Scenario {

    private static final Map<String, List<Method>> backupMethodsMapping = new ConcurrentHashMap<String, List<Method>>();

    private static final Map<Long, Script> scripts = new HashMap<>();
    private static Assembly ASSEMBLY;

    static {
        try {
            boolean isArm = true;
            if (isArm) {
                if (Runtime.is64Bit()) {
                    ASSEMBLY = new Arm64();
                } else if (Runtime.isThumb2()) {
                    ASSEMBLY = new Thumb2();
                } else {
                    ASSEMBLY = new Arm32();
                }
            }
            logd("Using: " + ASSEMBLY.getName());
        } catch (Exception ignored) {

        }
    }

    public static boolean hookMethod(Method origin, Method hook) {
        if (origin == null) {
            throw new IllegalArgumentException("Origin method cannot be null");
        }

        String methodName = origin.getName();

        ArtMethod artOrigin = ArtMethod.of(origin);
        Operator op = null;

        if(hook != null){
            ArtMethod artHook = ArtMethod.of(hook);
            op = new OperatorR(artOrigin, artHook);
        } else {
            op = new OperatorU(artOrigin);
        }

        long originalEntryPoint = ASSEMBLY.toMem(artOrigin.getEntryPointFromQuickCompiledCode());
        if (!scripts.containsKey(originalEntryPoint)) {
            scripts.put(originalEntryPoint, new Script(ASSEMBLY, originalEntryPoint));
        }
        Script script = scripts.get(originalEntryPoint);

        boolean ret = op.handle(script);
        if(!ret)
            return false;

        String className = origin.getDeclaringClass().getName();
        Method backupMethod = op.backupMethod();

        Debug.logd("backAddress:"+ Debug.longHex(Memory.getMethodAddress(backupMethod)));


        List<Method> backupList = backupMethodsMapping.get(methodName);
        if (backupList == null) {
            backupList = new LinkedList<Method>();
            backupMethodsMapping.put(methodName, backupList);
        }
        backupList.add(backupMethod);
        return true;
    }

    public static boolean hookMethod(Method origin) {
        return hookMethod(origin, null);
    }

    public static Method getBackMethod(Method origin){

        String methodName = origin.getName();
        List<Method> backupList = backupMethodsMapping.get(methodName);
        if (backupList == null) {
            return null;
        }

        return backupList.get(backupList.size()-1);
    }

}
