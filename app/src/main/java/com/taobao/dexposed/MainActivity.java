package com.taobao.dexposed;

import android.app.Activity;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;

import com.taobao.android.dexposed.DexposedBridge;
import com.taobao.android.dexposed.XC_MethodHook;

public class MainActivity extends Activity
{
	public static final String TAG = "===[dexposed-demo]===";
	
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);


		if (DexposedBridge.canDexposed(MainActivity.this)) {
			DexposedBridge.findAndHookMethod(MainActivity.class, "test", String.class, new XC_MethodHook()
			{

				@Override
				protected void beforeHookedMethod(XC_MethodHook.MethodHookParam param) throws Throwable
				{
					Log.d(TAG, "=========================beforeHookedMethod==============");
				}

				// To be invoked after Activity.onCreate()
				@Override
				protected void afterHookedMethod(MethodHookParam param) throws Throwable
				{
					Log.d(TAG, "=========================afterHookedMethod==============");
				}
			});
		}else{
			Log.d(TAG, "=========================canDexposed false==============");
		}

		((Button)findViewById(R.id.button)).setOnClickListener(new View.OnClickListener()
		{
			@Override
			public void onClick(View v)
			{
				Log.d(TAG, "=========================onClick==============" + test("1111"));
			}
		});

	}

	public String test(String param)
	{
		Log.d(TAG, "=========================test call=============="+param);
		return param;
	}
}
