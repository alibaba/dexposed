/*
 * Original work Copyright (c) 2016, Lody
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

package com.taobao.android.dexposed.art.method;

import android.os.Build;

import com.taobao.android.dexposed.art.Memory;
import com.taobao.android.dexposed.utility.Runtime;

import java.lang.reflect.AccessibleObject;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;


public abstract class ArtMethod extends Member {

    protected Method method;

    private static int M_OBJECT_SIZE = -1;

    static {
        if (Build.VERSION.SDK_INT >= 23) {
            try {
                Field f_objectSize = Class.class.getDeclaredField("objectSize");
                f_objectSize.setAccessible(true);
                M_OBJECT_SIZE = f_objectSize.getInt(Method.class);
            } catch (Throwable e) {
                //Ignore
            }
        }
    }

    public ArtMethod(Method method) {
        super(Memory.getMethodAddress(method));
        this.method = method;
    }

    public static ArtMethod of(Method method) {

        if (Build.VERSION.SDK_INT >= 23) {
            return  Runtime.is64Bit()
                    ? new ArtMethodV23_64Bit(method)
                    : new ArtMethodV23(method);
        }
        // The artmethod struct of Android 5.0 equals to V19,so it should not use V22.
        else if (Build.VERSION.SDK_INT > 21) {
            return Runtime.is64Bit()
                    ? new ArtMethodV22_64Bit(method)
                    : new ArtMethodV22(method);
        }
        else {
            return new ArtMethodV19(method);
        }
    }

    public ArtMethod backup() {
        try {
            Class<?> abstractMethodClass = Class.forName("java.lang.reflect.AbstractMethod");
            if (Build.VERSION.SDK_INT < 23) {
                Class<?> artMethodClass = Class.forName("java.lang.reflect.ArtMethod");
                //Get the original artMethod field
                Field artMethodField = abstractMethodClass.getDeclaredField("artMethod");
                if (!artMethodField.isAccessible()) {
                    artMethodField.setAccessible(true);
                }
                Object srcArtMethod = artMethodField.get(method);

                Constructor<?> constructor = artMethodClass.getDeclaredConstructor();
                constructor.setAccessible(true);
                Object destArtMethod = constructor.newInstance();

                //Fill the fields to the new method we created
                for (Field field : artMethodClass.getDeclaredFields()) {
                    if (!field.isAccessible()) {
                        field.setAccessible(true);
                    }
                    field.set(destArtMethod, field.get(srcArtMethod));
                }
                Method newMethod = Method.class.getConstructor(artMethodClass).newInstance(destArtMethod);
                newMethod.setAccessible(true);
                ArtMethod artMethod = ArtMethod.of(newMethod);
                artMethod.setEntryPointFromInterpreter(getEntryPointFromInterpreter());
                artMethod.setEntryPointFromJni(getEntryPointFromJni());
                artMethod.setEntryPointFromQuickCompiledCode(getEntryPointFromQuickCompiledCode());
                //NOTICE: The clone method must set the access flags to private.
                int accessFlags = getAccessFlags();
                accessFlags &= ~Modifier.PUBLIC;
                accessFlags |= Modifier.PRIVATE;
                artMethod.setAccessFlags(accessFlags);
                return artMethod;
            }else {
                Constructor<Method> constructor = Method.class.getDeclaredConstructor();
                // we can't use constructor.setAccessible(true); because Google does not like it
                AccessibleObject.setAccessible(new AccessibleObject[]{constructor}, true);
                Method m = constructor.newInstance();
                m.setAccessible(true);
                for (Field field : abstractMethodClass.getDeclaredFields()) {
                    field.setAccessible(true);
                    field.set(m, field.get(method));
                }
                Field artMethodField = abstractMethodClass.getDeclaredField("artMethod");
                artMethodField.setAccessible(true);
                long originArtMethod = artMethodField.getLong(method);
                long memoryAddress = Memory.map(M_OBJECT_SIZE);
                byte[] data = Memory.memget(originArtMethod, M_OBJECT_SIZE);
                Memory.memput(data, memoryAddress);
                artMethodField.set(m, memoryAddress);
                ArtMethod artMethod = ArtMethod.of(m);
                //NOTICE: The clone method must set the access flags to private.
                int accessFlags = getAccessFlags();
                accessFlags &= ~Modifier.PUBLIC;
                accessFlags |= Modifier.PRIVATE;
                artMethod.setAccessFlags(accessFlags);

                return artMethod;
            }


        } catch (Throwable e) {
            throw new IllegalStateException("Cannot create backup method from :: " + method);
        }
    }

    public Method getMethod() {
        return method;
    }


    public abstract long getEntryPointFromInterpreter();

    public abstract void setEntryPointFromInterpreter(long pointer_entry_point_from_interpreter);

    public abstract long getEntryPointFromJni();

    public abstract void setEntryPointFromJni(long pointer_entry_point_from_jni);

    public abstract long getEntryPointFromQuickCompiledCode();

    public abstract void setEntryPointFromQuickCompiledCode(long pointer_entry_point_from_quick_compiled_code);

    public abstract int getAccessFlags();

    public abstract void setAccessFlags(int newFlags);
}
