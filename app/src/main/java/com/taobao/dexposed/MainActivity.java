package com.taobao.dexposed;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;

import com.taobao.android.dexposed.DexposedBridge;
import com.taobao.android.dexposed.XC_MethodHook;

public class MainActivity extends AppCompatActivity
{
	public static final String TAG = "===[dexposed-demo]===";

	MainActivity instanc;
	
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		instanc = this;
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);


		if (DexposedBridge.canDexposed(MainActivity.this)) {
			DexposedBridge.findAndHookMethod(MainActivity.class, "test",  new XC_MethodHook()
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
				instanc.test();
			}
		});

	}

	public void test()
	{
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.menu_main, menu);
		return true;
	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		return true;
	}
}
