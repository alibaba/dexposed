package com.taobao.patch;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;

import com.taobao.android.dexposed.XC_MethodReplacement;
import com.taobao.android.dexposed.DexposedBridge;

public class DialogPatch implements IPatch {

	@Override
	public void handlePatch(final PatchParam arg0) throws Throwable {    	
    	Class<?> cls = null;
		try {
			cls= arg0.context.getClassLoader()
					.loadClass("com.taobao.dexposed.MainActivity");
		} catch (ClassNotFoundException e) {
			e.printStackTrace();
			return;
		}     	
     	DexposedBridge.findAndHookMethod(cls, "showDialog",
				new XC_MethodReplacement() {
			@Override
			protected Object replaceHookedMethod(MethodHookParam param) throws Throwable {
				Activity mainActivity = (Activity) param.thisObject;
				AlertDialog.Builder builder = new AlertDialog.Builder(mainActivity);
				builder.setTitle("Dexposed sample")
						.setMessage("The dialog is shown from patch apk!")
						.setPositiveButton("ok", new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int whichButton) {
							}
						}).create().show();
				return null;                 
			}
		});
	}

}
