package com.taobao.dexposed;

import android.os.Build;
import android.util.Log;
import android.view.Choreographer;

import com.taobao.android.dexposed.XC_MethodHook;
import com.taobao.android.dexposed.XC_MethodHook.Unhook;
import com.taobao.android.dexposed.DexposedBridge;


public class ChoreographerHook {


    private static ChoreographerHook choreographerHook;

    synchronized public static ChoreographerHook instance(){
        if(choreographerHook == null)
            choreographerHook = new ChoreographerHook();

        return choreographerHook;
    }

	private boolean isEnable(){
		if(Build.VERSION.SDK_INT >= 16 && Build.VERSION.SDK_INT < 21) {
			return true;
		} else 
			return false;
	}
	
	private ChoreographerHook() {

		if(!isEnable())
			return;

		try {
			mFrameIntervalNanos = (Long) Choreographer.class.getDeclaredField("mFrameIntervalNanos").get(Choreographer.getInstance());
		} catch (Exception e) {
			mFrameIntervalNanos = 1000000000 / 60;
		}

        mTolerableSkippedNanos = 15 * mFrameIntervalNanos;
	}
	
	private void hook() {
				
		Class<?> cls = null;
		try {
			cls = Class.forName("android.view.Choreographer$FrameDisplayEventReceiver");
		} catch (ClassNotFoundException e) {
			Log.w(TAG, "fail to find class FrameDisplayEventReceiver");
			e.printStackTrace();
			return;
		}

		XC_MethodHook mehodHook =  new XC_MethodHook() {
			protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
				
				long timestampNanos = (Long)param.args[0];
//				int  builtInDisplayId = (Integer)param.args[1];
//				int  frame = (Integer)param.args[2];
				
				long startNanos = System.nanoTime();
				jitterNanos = startNanos - timestampNanos;
				// we skip the frame that had been violate the mTolerableSkippedNanos when onVsync() arrived.
				if (jitterNanos >= mTolerableSkippedNanos) {
					final long skippedFrames = jitterNanos / mFrameIntervalNanos;
                    Log.i(TAG, "main thread skip " + skippedFrames + "frames");
				}
			}
		};
		//Choreographer 在4.1系统才有，并且4.2与4.1在onVsync方法的参数上有区别
		if(Build.VERSION.SDK_INT >= 17) {
			onVsync = DexposedBridge.findAndHookMethod(cls, "onVsync", long.class, int.class, int.class, mehodHook);
		}
		else if(Build.VERSION.SDK_INT == 16){
			onVsync = DexposedBridge.findAndHookMethod(cls, "onVsync", long.class, int.class, mehodHook);
		}
	}
	
	private void unhook() {

		if(onVsync != null) {
            onVsync.unhook();
            onVsync = null;
        }
	}
	
	
	private static final String TAG = "Lag";
	private long mFrameIntervalNanos;
	private long mTolerableSkippedNanos;
	private Unhook onVsync;
	private long jitterNanos;
    private boolean isStart = false;

	public void start() {
		if(!isEnable()) return;

        if(isStart) return;

		hook();

        isStart = true;
	}

	public void stop() {
		if(!isEnable()) return;

        if(!isStart) return;

        unhook();

        isStart = false;
	}
}
