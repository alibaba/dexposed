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


class PatchCallback {
	
	private final IPatch instance;
	
	protected PatchCallback(IPatch instance) {
		this.instance = instance;
	}
	
	protected static final PatchResult callAll(PatchParam param) {
		boolean isAllFailed =true;
        for (int i = 0; i < param.callbacks.length; i++) {
            try {
                ((PatchCallback) param.callbacks[i]).call(param);
                isAllFailed = false;
            } catch (Throwable t) {
            	t.printStackTrace();
            }
        }
        if (isAllFailed) {
            return new PatchResult(true, PatchResult.ALL_PATCH_FAILED, "All patch classes excute failed");
        } else {
            return new PatchResult(true, PatchResult.NO_ERROR, "");
        }        
	}
	
	protected void call(PatchParam param) throws Throwable {
		if (param instanceof PatchParam)
			handlePatch((PatchParam) param);
	}
	
	protected void handlePatch(PatchParam lpparam) throws Throwable {
		instance.handlePatch(lpparam);
	}
}
