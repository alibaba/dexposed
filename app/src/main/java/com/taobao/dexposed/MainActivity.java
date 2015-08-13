package com.taobao.dexposed;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;

import com.taobao.android.dexposed.DexposedBridge;
import com.taobao.android.dexposed.XC_MethodHook;

public class MainActivity extends AppCompatActivity
{
	public static final String TAG = "===[dexposed-demo]===";
	
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		if (DexposedBridge.canDexposed(this)) {
			DexposedBridge.findAndHookMethod(MainActivity.class, "onOptionsItemSelected", MenuItem.class, new XC_MethodHook()
			{

				// To be invoked before Activity.onCreate().
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
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		int id = item.getItemId();
		
		//noinspection SimplifiableIfStatement
		if (id == R.id.action_settings)
		{
			return true;
		}
		
		return super.onOptionsItemSelected(item);
	}
}
