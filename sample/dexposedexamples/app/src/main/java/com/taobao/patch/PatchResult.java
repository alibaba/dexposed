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

public class PatchResult {

	private boolean result;
	private int erroCode;
	private String ErrorInfo;
	private Throwable throwable;
	
	/**
	 * Success
	 */
	public static int NO_ERROR = 0;
	
	/**
	 * This device is not support. 
	 */
	public static int DEVICE_UNSUPPORT = 1;
	
	/**
	 * Exception happened during System.loadLibrary loading so.
	 */
	public static int LOAD_SO_EXCEPTION = 2;
	
	/**
	 * The dvm crashed during loading so at last time, so it tell this crash if try to load again.
	 */
	public static int LOAD_SO_CRASHED = 3;
	
	/**
	 * Please check the hotpatch file path if correct.
	 */
	public static int FILE_NOT_FOUND = 4;
	
	/**
	 * Exception happened during loading patch classes.
	 */
	public static int FOUND_PATCH_CLASS_EXCEPTION = 5;
	
	/**
	 * The hotpatch apk doesn't include some classes to patch. 
	 */
	public static int NO_PATCH_CLASS_HANDLE = 6;
	
	/**
	 * All patched classes run failed. Please check them if correct.
	 */
	public static int ALL_PATCH_FAILED = 7;

	public PatchResult(boolean isSuccess, int code, String info) {
		this.result = isSuccess;
		this.erroCode = code;
		this.ErrorInfo = info;
	}
	
	public PatchResult(boolean isSuccess, int code, String info, Throwable t) {
		this.result = isSuccess;
		this.erroCode = code;
		this.ErrorInfo = info;
		this.throwable = t;
	}

	public boolean isSuccess() {
		return this.result;
	}

	public int getErrocode() {
		return this.erroCode;
	}

	public String getErrorInfo() {
		return this.ErrorInfo;
	}
	
	public Throwable getThrowbale() {
		return this.throwable;
	}

}
