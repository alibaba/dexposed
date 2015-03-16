What is it?
-----------
Dexposed is a powerful yet non-invasive runtime [AOP (Aspect-oriented Programming)](http://en.wikipedia.org/wiki/Aspect-oriented_programming) framework
for Android app development.

The AOP of Dexposed is implemented purely non-invasive, without any annotation processor,
weaver or bytecode rewriter. The integration is as simple as loading a small JNI library
in just one line of code at the initialization phase of your app.

Not only the code of your app, but also the code of Android framework that running in your
app process can be hooked. This feature is extremely useful in Android development as we
developers heavily rely on the fragmented old versions of Android platform (SDK).

Together with dynamic class loading, a small piece of compiled Java AOP code can be loaded
into the running app, effectively altering the behavior of the target app without restart.

Typcial use-cases
-----------------
* Classic AOP programming
* Instrumentation (for testing, performance monitoring and etc.)
* Online hot patch to fix critical, emergent or security bugs
* SDK hooking for a better development experience

Integration
-----------
Insert the following line into the initialization phase of your app, as early as possible:

    public class MyApplication extends Application {

        public static boolean isSupport = false;
        @Override public void onCreate() {        
            // check device if support and auto load libs
            isSupport = XposedBridge.canDexposed(this);
        }

        ...
    }

It's done.

Basic usage
-----------
First,you can skip this part if you are good at using xposed. Because the usage is basically similar as xposed.

It's easy to understand how to use it. Just show two samples:

Sample One: Add logic before and after the Activity.onCreate(Bundle);

	//The first argument is the patching class,
		//and the second is the patching method name, and the following arguments are the patching method' arguments type class.
		//The last argument is the instance of XC_MethodHook or XC_MethodReplacement
		XposedBridge.findAndHookMethod(Activity.class, "oncreate", Bundle.class,
				new XC_MethodHook() {
                    // Add the logic before Activity.oncreate method.
					protected void beforeHookedMethod(MethodHookParam param)
							throws Throwable {
						//param.thisObject is the instance of the hook class.
						Activity instance = (Activity) param.thisObject;
						
						//The args[] is an array include all method arguments with the same index.
						Bundle bundle = (Bundle) param.args[0];
						Intent intent = new Intent();
						// With XposedHelpers, it can reflect/invoke any field/method.	
						XposedHelpers.setObjectField(param.thisObject, "mIntent", intent);
					    
						// Just call param.setResult If you want to just run before method and not continue to run original method.
						if (bundle.containsKey("return")) {
							param.setResult(null);
						}
					}
					
					// Add the logic after Activity.oncreate method.
					@Override
					protected void afterHookedMethod(MethodHookParam param)
							throws Throwable {
						XposedHelpers.callMethod(param.thisObject, "sampleMethod", 2);
					}
				});
				
Sample Two: Replace the original method.

		XposedBridge.findAndHookMethod(Activity.class, "oncreate", Bundle.class, new XC_MethodReplacement() {
			// Replace original method
			@Override
			protected Object replaceHookedMethod(MethodHookParam arg0)
					throws Throwable {
				// Just add logic as replacement
				........
			}

		});
		
Finally, you can check the dexposedexample project to find more usages. 

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