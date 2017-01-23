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

import com.taobao.android.dexposed.art.Memory;

public class Member {

    protected long address;

    public Member(long address) {
        this.address = address;
        applyField();
    }

    private void applyField() {
        java.lang.reflect.Field[] fields = getClass().getDeclaredFields();
        for (java.lang.reflect.Field realfield : fields) {
            FieldMapping structMapping = realfield.getAnnotation(FieldMapping.class);
            if (structMapping != null) {
                int length = structMapping.length();
                int offset = structMapping.offset();
                if (offset == -1) {
                    throw new IllegalStateException("Field in " + getClass() + " named " + realfield.getName() + " must declare the offset.");
                }
                if (length == -1) {
                    length = 4;
                }
                Field field = new Field(address, offset, length);
                if (!realfield.isAccessible()) {
                    realfield.setAccessible(true);
                }
                try {
                    realfield.set(this, field);
                } catch (IllegalAccessException e) {
                    throw new RuntimeException(e.getMessage());
                }
            }
        }
    }

    public long getAddress() {
        return address;
    }

    public void write(byte[] data) {
        write(data, 0);
    }
    public void write(byte[] data,int offset) {
        Memory.memput(data, address + offset);
    }
}
