/*
 * Original work Copyright (c) 2015, orlando1986
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

import com.taobao.android.dexposed.DexposedBridge;
import com.taobao.android.dexposed.utility.Debug;

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

public class Entry {

    private final static String TAG = "Entry";

    private static int[] getParamList(Method method) {
        Class<?>[] types = method.getParameterTypes();
        int index = 0;
        int[] result = new int[types.length];

        for (int i = 0; i < types.length; i++) {
            String type = types[i].toString();
            if (type.contains("int")) {
                result[index] = 8;
            } else if (type.contains("short")) {
                result[index] = 7;
            } else if (type.contains("float")) {
                result[index] = 6;
            } else if (type.contains("boolean")) {
                result[index] = 5;
            } else if (type.contains("char")) {
                result[index] = 4;
            } else if (type.contains("byte")) {
                result[index] = 3;
            } else if (type.contains("long")) {
                result[index] = 2;
            } else if (type.contains("double")) {
                result[index] = 1;
            } else { // Object
                result[index] = 0;
            }
            index++;
        }
        return result;
    }

    private static boolean isStatic(Method method){
        Debug.logd(TAG, "isStatic:" + Modifier.isStatic(method.getModifiers()));
        return  Modifier.isStatic(method.getModifiers());
    }

    private static final int BASE = 12;

    private static int getReturnType(Method method) {
        Class<?> type_class = method.getReturnType();
        String type = type_class.toString();
        if (type.contains("void")) {
            return 9 + BASE;
        } else if (type.contains("int")) {
            return 8 + BASE;
        } else if (type.contains("short")) {
            return 7 + BASE;
        } else if (type.contains("float")) {
            return 6 + BASE;
        } else if (type.contains("boolean")) {
            return 5 + BASE;
        } else if (type.contains("char")) {
            return 4 + BASE;
        } else if (type.contains("byte")) {
            return 3 + BASE;
        } else if (type.contains("long")) {
            return 2 + BASE;
        } else if (type.contains("double")) {
            return 1 + BASE;
        } else { // Object
            return 0 + BASE;
        }
    }

    private static int boxArgs(Object[] box, int index, int arg) {
        box[index] = Integer.valueOf(arg);
        return 1;
    }

    private static int boxArgs(Object[] box, int index, long arg) {
        box[index] = Long.valueOf(arg);
        return 2;
    }

    private static int boxArgs(Object[] box, int index, byte arg) {
        box[index] = Byte.valueOf(arg);
        return 1;
    }

    private static int boxArgs(Object[] box, int index, char arg) {
        box[index] = Character.valueOf(arg);
        return 1;
    }

    private static int boxArgs(Object[] box, int index, double arg) {
        box[index] = Double.valueOf(arg);
        return 2;
    }

    private static int boxArgs(Object[] box, int index, float arg) {
        box[index] = Float.valueOf(arg);
        return 1;
    }

    private static int boxArgs(Object[] box, int index, Object arg) {
        box[index] = arg;
        return 1;
    }

    private static int boxArgs(Object[] box, int index, short arg) {
        box[index] = Short.valueOf(arg);
        return 1;
    }

    private static int boxArgs(Object[] box, int index, boolean arg) {
        box[index] = Boolean.valueOf(arg);
        return 1;
    }

    private static int onHookInt(Object artmethod, Object receiver, Object[] args) {
        return (Integer) DexposedBridge.handleHookedArtMethod(artmethod, receiver, args);
    }

    private static long onHookLong(Object artmethod, Object receiver, Object[] args) {
        return (Long) DexposedBridge.handleHookedArtMethod(artmethod, receiver, args);
    }

    private static double onHookDouble(Object artmethod, Object receiver, Object[] args) {
        return (Double) DexposedBridge.handleHookedArtMethod(artmethod, receiver, args);
    }

    private static char onHookChar(Object artmethod, Object receiver, Object[] args) {
        return (Character) DexposedBridge.handleHookedArtMethod(artmethod, receiver, args);
    }

    private static short onHookShort(Object artmethod, Object receiver, Object[] args) {
        return (Short) DexposedBridge.handleHookedArtMethod(artmethod, receiver, args);
    }

    private static float onHookFloat(Object artmethod, Object receiver, Object[] args) {
        return (Float) DexposedBridge.handleHookedArtMethod(artmethod, receiver, args);
    }

    private static Object onHookObject(Object artmethod, Object receiver, Object[] args) {
        return DexposedBridge.handleHookedArtMethod(artmethod, receiver, args);
    }

    private static void onHookVoid(Object artmethod, Object receiver, Object[] args) {
        DexposedBridge.handleHookedArtMethod(artmethod, receiver, args);
    }

    private static boolean onHookBoolean(Object artmethod, Object receiver, Object[] args) {
        return (Boolean) DexposedBridge.handleHookedArtMethod(artmethod, receiver, args);
    }

    private static byte onHookByte(Object artmethod, Object receiver, Object[] args) {
        return (Byte) DexposedBridge.handleHookedArtMethod(artmethod, receiver, args);
    }

}
