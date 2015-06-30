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

import java.io.File;
import java.util.Enumeration;
import java.util.HashMap;

import dalvik.system.DexClassLoader;
import dalvik.system.DexFile;

/**
* 
* <p>PatchMain is used to load a patch apk file.</p>
* 
* <p>Note:The patch apk must not include the classes which are already in loader apk, so the patch.jar and dexposed.jar
* should be included in hotpatchpatch project, but not output to patch apk.</p>
* 
* <p>Before using this, one necessary condition, the libdexposed.so, libdexposed2.3.so and dexposedbridge.jar must
* be included in loader project.</p>
* 
* <p>The patch apk file should just normal apk which include some patch classes to hook
* methods you want to modify. These patch classes will be invoked auto if load(...) of this called.</p>  
*
* <p>About patch class, which must implement IPatch interface and override handlePatch.</p>
* 
* The sample:
* <pre class="prettyprint">
*   public class HotPatchTest implements IPatch {
*      //@Override
*	   public void handlePatch(final PatchParam arg0) throws Throwable {
*	   }
*	}
* </pre>
*  
*  <p>The PatchParam includes context and an object HashMap which include objects may be used by patch class. 
*  And that object HashMap is equal the arg one sent in load() method. </p>
*  
*  <p>How to implement handlePatch method?</p>
*  
*  <p>First, it needs to find class which will be patched. 
*  PatchParams' member context has the getClassLoader to load class that you want to patch.</p>
*  
*  <p>Then, the XposedBridge.findAndHookMethod should be used here. The first argument is the patching class,
*  and the second is the patching method name, and the following arguments are the patching method' arguments type class.
*  The last argument is the instance of XC_MethodHook or XC_MethodReplacement, the following are description for these two.</p>
*  
*  <p>The XC_MethodHook has two methods beforeHookedMethod and afterHookedMethod, and these easy to understand that they are called
*  before/after the invocation of the hooked method. </p>
*  
*  <p>The XC_MethodReplacement has method replaceHookedMethod to replace the whole original method.</p>
*  
*  <P>The MethodHookParam is the only argument used in above three methods, which include some useful contents.
*  MethodHookParam.thisObject is the instance of this class.
*  In MethodHookParam.args, it offers all argument values of this method.
*  And MethodHookParam.setResult can modify the result of the method call. In a "before-method-call"
*  hook, prevents the call to the original method. But it still need to "return" from the hook handler if required.</p>
*  
*  <P>Finally, this load() method will return the PatchResult which show the result or error detail. </p>
*  
*  @version 1.0
*/

public class PatchMain {
	
	private static final ReadWriteSet<PatchCallback> loadedPatchCallbacks = new ReadWriteSet<PatchCallback>();	

	 /**
     * Load a runnable patch apk.
     *
     * @param context the application or activity context.
     * @param apkPath the path of patch apk file.
     * @param contentMap the object maps that will be used by patch classes.  
     * @return PatchResult include if success or error detail.
     */
	public static PatchResult load(Context context, String apkPath, HashMap<String, Object> contentMap) {
		
		if (!new File(apkPath).exists()) {
			return new PatchResult(false, PatchResult.FILE_NOT_FOUND, "FILE not found on " + apkPath);
		}

	    PatchResult result = loadAllCallbacks(context, apkPath,context.getClassLoader());
		if (!result.isSuccess()) {
			return result;
		}

		if (loadedPatchCallbacks.getSize() == 0) {
			return new PatchResult(false, PatchResult.NO_PATCH_CLASS_HANDLE, "No patch class to be handle");
		}
		
		PatchParam lpparam = new PatchParam(loadedPatchCallbacks);		
		lpparam.context = context;
		lpparam.contentMap = contentMap;
		
		return PatchCallback.callAll(lpparam);
	}

	private static PatchResult loadAllCallbacks(Context context, String apkPath, ClassLoader cl) {
		try {
//			String dexPath = new File(context.getFilesDir(), apkPath.).getAbsolutePath();
			File dexoptFile = new File(apkPath + "odex");
			if (dexoptFile.exists()) {
				dexoptFile.delete();
			}
			ClassLoader mcl = null;			
			try {
				mcl = new DexClassLoader (apkPath,context.getFilesDir().getAbsolutePath(),null, cl);
			}catch(Throwable e){
				return new PatchResult(false, PatchResult.FOUND_PATCH_CLASS_EXCEPTION, "Find patch class exception ", e);
			}
			DexFile dexFile = DexFile.loadDex(apkPath, context.getFilesDir().getAbsolutePath() + File.separator + "patch.odex", 0);
			Enumeration<String> entrys = dexFile.entries();
			// clean old callback
			synchronized (loadedPatchCallbacks) {
				loadedPatchCallbacks.clear();
			}
			while (entrys.hasMoreElements()) {
				String entry = entrys.nextElement();				
				Class<?> entryClass = null;
				try {
					entryClass = mcl.loadClass(entry);
				} catch (ClassNotFoundException e) {
					e.printStackTrace();
					break;
				}
				if (isImplementInterface(entryClass, IPatch.class)) {
					Object moduleInstance = entryClass.newInstance();
					hookLoadPatch(new PatchCallback((IPatch) moduleInstance));
				}
			}

		} catch (Exception e) {
			return new PatchResult(false, PatchResult.FOUND_PATCH_CLASS_EXCEPTION, "Find patch class exception ", e);
		}
		return new PatchResult(true, PatchResult.NO_ERROR, "");
	}
	
	private static boolean isImplementInterface(Class<?> entry, Class<?> interClass) {
		Class<?>[] interfaces = entry.getInterfaces();
		if (interfaces == null) {
			return false;
		}
		for (int i = 0; i < interfaces.length; i++) {
			if (interfaces[i].equals(interClass)) {
				return true;
			}
		}
		return false;		
	}
	
	/**
	 * Get notified when a patch is loaded. This is especially useful to hook some patch-specific methods.
	 */
	private static void hookLoadPatch(PatchCallback callback) {
		synchronized (loadedPatchCallbacks) {
			loadedPatchCallbacks.add(callback);
		}
	}
}
