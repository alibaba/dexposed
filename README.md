What is it?
-----------

[![Download](https://api.bintray.com/packages/hwjump/maven/dexposed/images/download.svg) ](https://bintray.com/hwjump/maven/dexposed/_latestVersion)
[![Software License](https://rawgit.com/alibaba/dexposed/master/images/license.svg)](LICENSE)
[![Join the chat at https://gitter.im/alibaba/dexposed](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/alibaba/dexposed?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)   

Dexposed is a powerful yet non-invasive runtime [AOP (Aspect-oriented Programming)](http://en.wikipedia.org/wiki/Aspect-oriented_programming) framework
for Android app development, based on the work of open-source [Xposed](https://github.com/rovo89/Xposed) [framework](https://github.com/rovo89/XposedBridge) project.

The AOP of Dexposed is implemented purely non-invasive, without any annotation processor,
weaver or bytecode rewriter. The integration is as simple as loading a small JNI library
in just one line of code at the initialization phase of your app.

Not only the code of your app, but also the code of Android framework that running in your
app process can be hooked. This feature is extremely useful in Android development as we
developers heavily rely on the fragmented old versions of Android platform (SDK).

Together with dynamic class loading, a small piece of compiled Java AOP code can be loaded
into the running app, effectively altering the behavior of the target app without restart.

Typical use-cases
-----------------
* Classic AOP programming
* Instrumentation (for testing, performance monitoring and etc.)
* Online hot patch to fix critical, emergent or security bugs
* SDK hooking for a better development experience

Integration
-----------
Directly add dexposed aar to your project as compile libraries, it contains a jar file "dexposedbridge.jar" two so files "libdexposed.so libdexposed_l.so" from 'dexposed' directory.

Gradle dependency like following:

```groovy
	dependencies {
	    compile 'com.taobao.android:dexposed:0.1.1@aar'
	}
```

Insert the following line into the initialization phase of your app, as early as possible:

```java
    public class MyApplication extends Application {

        @Override public void onCreate() {        
            // Check whether current device is supported (also initialize Dexposed framework if not yet)
            if (DexposedBridge.canDexposed(this)) {
                // Use Dexposed to kick off AOP stuffs.
                ...
            }
        }
        ...
    }
```

It's done.

Basic usage
-----------

There are three injection points for a given method: *before*, *after*, *replace*.

Example 1: Attach a piece of code before and after all occurrences of `Activity.onCreate(Bundle)`.

```java
        // Target class, method with parameter types, followed by the hook callback (XC_MethodHook).
		DexposedBridge.findAndHookMethod(Activity.class, "onCreate", Bundle.class, new XC_MethodHook() {
        
            // To be invoked before Activity.onCreate().
			@Override protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
				// "thisObject" keeps the reference to the instance of target class.
				Activity instance = (Activity) param.thisObject;
        
				// The array args include all the parameters.
				Bundle bundle = (Bundle) param.args[0];
				Intent intent = new Intent();
				// XposedHelpers provide useful utility methods.
				XposedHelpers.setObjectField(param.thisObject, "mIntent", intent);
		
				// Calling setResult() will bypass the original method body use the result as method return value directly.
				if (bundle.containsKey("return"))
					param.setResult(null);
			}
					
			// To be invoked after Activity.onCreate()
			@Override protected void afterHookedMethod(MethodHookParam param) throws Throwable {
		        XposedHelpers.callMethod(param.thisObject, "sampleMethod", 2);
			}
		});
```

Example 2: Replace the original body of the target method.

```java
		DexposedBridge.findAndHookMethod(Activity.class, "onCreate", Bundle.class, new XC_MethodReplacement() {
		
			@Override protected Object replaceHookedMethod(MethodHookParam param) throws Throwable {
				// Re-writing the method logic outside the original method context is a bit tricky but still viable.
				...
			}

		});
```

Checkout the `example` project to find out more.

Support
----------
Dexposed support all dalvik runtime arm architecture devices from Android 2.3 to 4.4 (no include 3.0). The stability has been proved in our long term product practice.

Follow is support status.

Runtime | Android Version | Support
------  | --------------- | --------
Dalvik  | 2.2             | Not Test
Dalvik  | 2.3             | Yes
Dalvik  | 3.0             | No
Dalvik  | 4.0-4.4         | Yes
ART     | 5.0             | Testing
ART     | 5.1             | No
ART     | M               | No
 

Contribute
----------
We are open to constructive contributions from the community, especially pull request
and quality bug report. **Currently, the support for new Android Runtime (ART) is still
in early beta stage, we value your help to test or improve the implementation.**

Dexposed is aimed to be lightweight, transparent and productive. All improvements with
these principal in mind are welcome. At the same time, we are actively exploring more
potentially valuable use-cases and building powerful tools based upon Dexposed. We're
interested in any ideas expanding the use-cases and grateful for community developed
tools on top of Dexposed.
