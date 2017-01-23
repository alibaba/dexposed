package com.taobao.dexposed_test;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import com.taobao.android.dexposed.DexposedBridge;
import com.taobao.android.dexposed.XC_MethodHook;

import java.lang.reflect.Method;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button button = (Button) this.findViewById(R.id.hello);

        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                test();
            }
        });
    }


    private void test(){

        Method method1 = null;
        Method method2 = null;

        try {
            method1 = this.getClass().getDeclaredMethod("showlog1", int.class,Object.class,int.class,int.class,int.class,int.class,int.class,int.class,long.class,int.class);
            method2 = this.getClass().getDeclaredMethod("showlog2", int.class,Object.class,int.class,int.class,int.class,int.class,int.class,int.class,long.class,int.class);
        } catch(Exception e){

        }

//        try {
//            method1 = Log.class.getDeclaredMethod("d", String.class, String.class);
//            method2 = this.getClass().getDeclaredMethod("logd", String.class, String.class);
//        } catch(Exception e){
//
//        }

        DexposedBridge.findAndHookMethod(this.getClass(), "showlog1", int.class, Object.class, int.class, int.class, int.class, int.class, int.class, int.class, long.class, int.class,
                new XC_MethodHook() {
                    @Override
                    protected void beforeHookedMethod(MethodHookParam param) throws Throwable {

                        Log.d("test", "before hook1:"+param.method);

                        for(Object arg : param.args)
                            Log.d("test", "before hook1:"+arg);

//                        param.setResult(null);
                    }

                    @Override
                    protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                        Log.d("test", "after hook");
                    }
                });

        DexposedBridge.findAndHookMethod(this.getClass(), "showlog2", int.class, Object.class, int.class, int.class, int.class, int.class, int.class, int.class, long.class, int.class,
                new XC_MethodHook() {
                    @Override
                    protected void beforeHookedMethod(MethodHookParam param) throws Throwable {

                        Log.d("test", "before hook2:"+param.method);

                        for(Object arg : param.args)
                            Log.d("test", "before hook2:"+arg);

//                        param.setResult(null);
                    }

                    @Override
                    protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                        Log.d("test", "after hook2");
                    }
                });

        showlog1(0x111,this,0x333,0x444,0x555,0x666,0x777,0x888,0x999,0xaaa);
        showlog2(0x111,this,0x333,0x444,0x555,0x666,0x777,0x888,0x999,0xaaa);
        Log.d("test", "end");
    }


    public static void showlog1(int a1, Object a2, int a3, int a4, int a5, int a6, int a7, int a8, long a9, int a10){

        Log.d("test","this is show log 1");
    }

    private static void showlog2(int a1, Object a2, int a3, int a4, int a5, int a6, int a7, int a8, long a9, int a10){

        Log.d("test","this is show log 2");
    }
}
