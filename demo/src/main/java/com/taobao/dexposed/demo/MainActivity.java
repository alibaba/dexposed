package com.taobao.dexposed.demo;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import com.taobao.android.dexposed.DexposedBridge;
import com.taobao.android.dexposed.XC_MethodHook;
import com.taobao.android.dexposed.XC_MethodReplacement;

public class MainActivity extends Activity
{
	public static final String TAG = "===[dexposed-demo]===";
	
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		Log.d(TAG, "=========================onCreate==============");
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		((Button)findViewById(R.id.button)).setOnClickListener(new View.OnClickListener()
		{
			@Override
			public void onClick(View v)
			{
				if (!DexposedBridge.canDexposed(MainActivity.this)) {
					Log.d(TAG, "=========================canDexposed false==============");
					return;
				}

				XC_MethodHook hook = new XC_MethodHook()
				{

					@Override
					protected void beforeHookedMethod(XC_MethodHook.MethodHookParam param) throws Throwable
					{
						Log.d(TAG, "=========================beforeHookedMethod==============");

					}

					@Override
					protected void afterHookedMethod(MethodHookParam param) throws Throwable
					{
						Log.d(TAG, "=========================afterHookedMethod==============");
					}
				};

				DexposedBridge.findAndHookMethod(MainActivity.class, "testReturnString", String.class,  int.class, long.class, Double.class, hook);
				Log.d(TAG, "=========================call ==============" + testReturnString("testReturnString", 111, 222, 333d));

				DexposedBridge.findAndHookMethod(MainActivity.class, "testReturnInt", String.class, int.class, long.class, Double.class, hook);
				Log.d(TAG, "=========================call ==============" + testReturnInt("testReturnInt", 111, 222, 333d));

				DexposedBridge.findAndHookMethod(MainActivity.class, "testReturnDouble", String.class, int.class, long.class, Double.class, hook);
				Log.d(TAG, "=========================call ==============" + testReturnDouble("testReturnDouble", 111, 222, 333d));

				DexposedBridge.findAndHookMethod(MainActivity.class, "testReturnFloat", String.class, int.class, long.class, Double.class, hook);
				Log.d(TAG, "=========================call ==============" + testReturnFloat("testReturnFloat", 111, 222, 333d));


				DexposedBridge.findAndHookMethod(MainActivity.class, "testReturnFloat", String.class, int.class, long.class, Double.class, new XC_MethodReplacement()
				{
					@Override
					protected Object replaceHookedMethod(MethodHookParam param) throws Throwable
					{
						return new Float(3333.0f);
					}
				});
				Log.d(TAG, "=========================replaceHookedMethod testReturnFloat==============" + testReturnFloat("testReturnFloat", 111, 222, 333d));


				DexposedBridge.findAndHookMethod(MainActivity.class, "testReturnDouble", String.class, int.class, long.class, Double.class, new XC_MethodReplacement()
				{
					@Override
					protected Object replaceHookedMethod(MethodHookParam param) throws Throwable
					{
						return new Double(66666.0);
					}
				});
				Log.d(TAG, "=========================replaceHookedMethod testReturnDouble==============" + testReturnDouble("testReturnDouble", 111, 222, 333d));

				DexposedBridge.findAndHookMethod(MainActivity.class, "testReturnString", String.class, int.class, long.class, Double.class, new XC_MethodReplacement()
				{
					@Override
					protected Object replaceHookedMethod(MethodHookParam param) throws Throwable
					{
						return "replaceHookedMethod";
					}
				});
				Log.d(TAG, "=========================replaceHookedMethod testReturnString==============" + testReturnString("testReturnString", 111, 222, 333d));
			}
		});

	}

	public String testReturnString(String param,int p2, long p3, Double p4)
	{
		Log.d(TAG, "=========================test call==============" + param + p2 + p3 + p4);
		return "9999999";

	}

	public int testReturnInt(String param,int p2, long p3, Double p4)
	{
		Log.d(TAG, "=========================test call=============="+param+p2+p3+p4);
		return 999;

	}

	public double testReturnDouble(String param,int p2, long p3, Double p4)
	{
		Log.d(TAG, "=========================test call=============="+param+p2+p3+p4);
		return 7777.0;

	}

	public Float testReturnFloat(String param,int p2, long p3, Double p4)
	{
		Log.d(TAG, "=========================test call=============="+param+p2+p3+p4);
		return 7777.0f;

	}
}
