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

public abstract class MethodContext {
    public final ArtMethod source;
    public final ArtMethod target;

    public MethodContext(ArtMethod source, ArtMethod target) {
        this.source = source;
        this.target = target;
    }

    public abstract int size(Assembly assembly);

    public abstract byte[] createScript(Assembly assembly);
}
