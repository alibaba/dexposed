package com.taobao.dexposed;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.taobao.android.dexposed.XC_MethodHook;
import com.taobao.android.dexposed.DexposedBridge;
import com.taobao.patch.PatchMain;
import com.taobao.patch.PatchResult;

import java.io.File;


public class MainActivity extends Activity {

	static {
		// load xposed lib for hook.
		try {
			if (android.os.Build.VERSION.SDK_INT == 22){
				System.loadLibrary("dexposed_l51");
			} else if (android.os.Build.VERSION.SDK_INT > 19 && android.os.Build.VERSION.SDK_INT <= 21){
				System.loadLibrary("dexposed_l");
			} else if (android.os.Build.VERSION.SDK_INT > 14){
				System.loadLibrary("dexposed");
			}
		} catch (Throwable e) {
			e.printStackTrace();
		}
	}

	private boolean isSupport = false;
    private boolean isLDevice= false;
	
	private TextView mLogContent;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		mLogContent = (TextView) (this.findViewById(R.id.log_content));
		// check device if support and auto load libs
        isSupport = true;
		isLDevice = android.os.Build.VERSION.SDK_INT >= 20;
	}

	//Hook system log click
	public void hookSystemLog(View view) {
		if (isSupport) {
			DexposedBridge.findAndHookMethod(isLDevice ? this.getClass(): Log.class, isLDevice ? "showLog" : "d", String.class, String.class, new XC_MethodHook() {
				@Override
				protected void afterHookedMethod(MethodHookParam arg0) throws Throwable {
					String tag = (String) arg0.args[0];
					String msg = (String) arg0.args[1];
					mLogContent.setText(tag + "," + msg);
				}
			});
            if (isLDevice) {
                showLog("dexposed", "It doesn't support AOP to system method on ART devices");
            } else {
                Log.d("dexposed", "Logs are redirected to display here");
            }
		} else {
			mLogContent.setText("This device doesn't support dexposed!");
		}
	}

    private void showLog(String tag, String msg) {
        Log.d(tag, msg);
    }
	
	// Hook choreographer click
	public void hookChoreographer(View view) {
		Log.d("dexposed", "hookChoreographer button clicked.");
		if (isSupport && !isLDevice) {
			ChoreographerHook.instance().start();
		} else {
			showLog("dexposed", "This device doesn't support this!");
		}
	}
	
	// Run patch apk
	public void runPatchApk(View view) {
		Log.d("dexposed", "runPatchApk button clicked.");
        if (isLDevice) {
            showLog("dexposed", "It doesn't support this function on L device.");
            return;
        }
        if (!isSupport) {
			Log.d("dexposed", "This device doesn't support dexposed!");
			return;
		}
		File cacheDir = getExternalCacheDir();
    	if(cacheDir != null){
    		String fullpath = cacheDir.getAbsolutePath() + File.separator + "patch.apk";
    		PatchResult result = PatchMain.load(this, fullpath, null);
    		if (result.isSuccess()) {
    			Log.e("Hotpatch", "patch success!");
    		} else {
    			Log.e("Hotpatch", "patch error is " + result.getErrorInfo());
    		}
    	}
    	showDialog();
	}
	
	private void showDialog() {
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setTitle("Dexposed sample")
				.setMessage(
						"Please clone patchsample project to generate apk, and copy it to \"/Android/data/com.taobao.dexposed/cache/patch.apk\"")
				.setPositiveButton("ok", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int whichButton) {
					}
				}).create().show();
	}
}