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

package com.taobao.android.dexposed.utility;

import android.util.Log;

import com.taobao.android.dexposed.art.method.ArtMethod;

import java.lang.reflect.Method;

public final class Debug {
    private static final String TAG = "Dexposed";
    private static final long HEXDUMP_BYTES_PER_LINE = 16;

    public static boolean DEBUG = true;
    public static boolean WARN = true;

    private Debug() {
    }

    public static String addrHex(long i) {
        if (Runtime.is64Bit()) {
            return longHex(i);
        } else {
            return intHex((int) i);
        }
    }

    public static String longHex(long i) {
        return String.format("0x%016X", i);
    }

    public static String intHex(int i) {
        return String.format("0x%08X", i);
    }

    public static String byteHex(byte b) {
        return String.format("%02X", b);
    }

    public static String hexdump(byte[] bytes, long start) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0 - (int) (start % HEXDUMP_BYTES_PER_LINE); i < bytes.length; i++) {
            long num = Math.abs((start + i) % HEXDUMP_BYTES_PER_LINE);
            if (num == 0 && sb.length() > 0)
                sb.append('\n');
            if (num == 0)
                sb.append(addrHex(start + i)).append(": ");
            if (num == 8)
                sb.append(" ");
            if (i >= 0)
                sb.append(Debug.byteHex(bytes[i])).append(" ");
            else
                sb.append("   ");
        }
        return sb.toString().trim();
    }

    public static void logd(String msg) {
        if (DEBUG) Log.d(TAG, msg);
    }

    public static void logd(String tagSuffix, String msg) {
        if (DEBUG) Log.d(TAG + "." + tagSuffix, msg);
    }

    public static void logw(Exception e) {
        if (WARN) Log.w(TAG, e);
    }

    public static void logw(String msg) {
        if (WARN) Log.w(TAG, msg);
    }

    public static String methodDescription(Method method) {
        return method.getDeclaringClass().getName() + "->" + method.getName() + " @" +
                addrHex(ArtMethod.of(method).getEntryPointFromQuickCompiledCode()) +
                " +" + addrHex(ArtMethod.of(method).getAddress());
    }
}
