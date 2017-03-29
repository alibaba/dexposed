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

import java.lang.reflect.Method;

public class ArtMethodV21 extends ArtMethod {

    @FieldMapping(offset = 28)
    private Field entry_point_from_interpreter_;

    @FieldMapping(offset = 36)
    private Field entry_point_from_jni_;

    @FieldMapping(offset = 44)
    private Field entry_point_from_quick_compiled_code_;

    @FieldMapping(offset = 52)
    private Field access_flags_;

    ArtMethodV21(Method method) {
        super(method);
    }

    @Override
    public long getEntryPointFromInterpreter() {
        return entry_point_from_interpreter_.readLong();
    }

    @Override
    public void setEntryPointFromInterpreter(long pointer_entry_point_from_interpreter) {
        this.entry_point_from_interpreter_.write(pointer_entry_point_from_interpreter);
    }

    @Override
    public long getEntryPointFromJni() {
        return entry_point_from_jni_.readLong();
    }

    @Override
    public void setEntryPointFromJni(long pointer_entry_point_from_jni) {
        this.entry_point_from_jni_.write(pointer_entry_point_from_jni);
    }

    @Override
    public long getEntryPointFromQuickCompiledCode() {
        return entry_point_from_quick_compiled_code_.readLong();
    }

    @Override
    public void setEntryPointFromQuickCompiledCode(long pointer_entry_point_from_quick_compiled_code) {
        this.entry_point_from_quick_compiled_code_.write(pointer_entry_point_from_quick_compiled_code);
    }

    @Override
    public int getAccessFlags() {
        return access_flags_.readInt();
    }

    @Override
    public void setAccessFlags(int newFlags) {
        access_flags_.write(newFlags);
    }

}
