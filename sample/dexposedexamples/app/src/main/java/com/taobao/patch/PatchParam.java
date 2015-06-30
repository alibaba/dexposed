/*
 * Original work Copyright (c) 2015, Alibaba Mobile Infrastructure (Android) Team
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

package com.taobao.patch;

import android.content.Context;

import java.util.HashMap;

public class PatchParam {
	
	protected final Object[] callbacks;
	
	public PatchParam(ReadWriteSet<PatchCallback> callbacks) {
		this.callbacks = callbacks.getSnapshot();
	}

	/**
	 * The context can be get by patch class. The most important is offer classloader.
	 */
	public Context context;
	
	/**
	 * This map contains objects that may be used by patch class.
	 */
	public HashMap<String, Object> contentMap;
}
