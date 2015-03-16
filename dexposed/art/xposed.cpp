#include "xposed.h"

#include <utils/Log.h>
#include <android_runtime/AndroidRuntime.h>
#include <stdio.h>
#include <sys/mman.h>
#include <cutils/properties.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <entrypoints/entrypoint_utils.h>

#include "quick_argument_visitor.cpp"
#include "quick_arg_array.cpp"

namespace art {

	jclass xposed_class = NULL;
	jmethodID xposed_handle_hooked_method = NULL;

	void logMethod(const char* tag, ArtMethod* method) {
		LOG(INFO) << "xposed:" << tag << " " << method << " " << PrettyMethod(method);
	}

	bool xposedOnVmCreated(JNIEnv* env, const char*) {

		xposed_class = env->FindClass(XPOSED_CLASS);
		xposed_class = reinterpret_cast<jclass>(env->NewGlobalRef(xposed_class));

		if (xposed_class == NULL) {
			LOG(ERROR) << "xposed: Error while loading Xposed class " << XPOSED_CLASS;
			env->ExceptionClear();
			return false;
		}

		LOG(INFO) << "xposed: now initializing, Found Xposed class " << XPOSED_CLASS;
		if (register_com_taobao_android_dexposed_XposedBridge(env) != JNI_OK) {
			LOG(ERROR) << "xposed: Could not register natives for " << XPOSED_CLASS;
			env->ExceptionClear();
			return false;
		}

		jmethodID xposedbridgeMainMethod = env->GetStaticMethodID(xposed_class,
				"main", "()V");
		if (xposedbridgeMainMethod == NULL) {
			LOG(ERROR) << "xposed: Could not find method " << XPOSED_CLASS << ".main()";
			env->ExceptionClear();
			return false;
		}
		env->CallStaticVoidMethod(xposed_class, xposedbridgeMainMethod);

		return true;
	}

	extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void*)
	{
		JNIEnv* env = NULL;
		jint result = -1;

		if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
			return result;
		}

		int keepLoadingXposed = xposedOnVmCreated(env, NULL);

		return JNI_VERSION_1_6;
	}

	static jboolean com_taobao_android_dexposed_XposedBridge_initNative(JNIEnv* env,
			jclass) {

		LOG(INFO) << "xposed: com_taobao_android_dexposed_XposedBridge_initNative";

		xposed_handle_hooked_method =
				env->GetStaticMethodID(xposed_class, "handleHookedMethod",
						"(Ljava/lang/reflect/Member;ILjava/lang/Object;Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");
		if (xposed_handle_hooked_method == NULL) {
			LOG(ERROR) << "xposed: Could not find method " << XPOSED_CLASS << ".handleHookedMethod()";
			env->ExceptionClear();
			return false;
		}
		return true;
	}

	extern "C" void art_quick_xposed_invoke_handler();
	static inline const void* GetQuickXposedInvokeHandler() {
		return reinterpret_cast<void*>(art_quick_xposed_invoke_handler);
	}

	JValue xposedCallHandler(ScopedObjectAccessAlreadyRunnable& soa,
			const char* shorty, jobject rcvr_jobj, jmethodID method,
			std::vector<jvalue>& args)
	SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {

		LOG(INFO) << "xposed: >>> xposedCallHandler";

		// Build argument array possibly triggering GC.
		soa.Self()->AssertThreadSuspensionIsAllowable();
		jobjectArray args_jobj = NULL;
		const JValue zero;
		int32_t target_sdk_version = Runtime::Current()->GetTargetSdkVersion();
		// Do not create empty arrays unless needed to maintain Dalvik bug compatibility.
		if (args.size() > 0
				|| (target_sdk_version > 0 && target_sdk_version <= 21)) {
			args_jobj = soa.Env()->NewObjectArray(args.size(),
					WellKnownClasses::java_lang_Object, NULL);
			if (args_jobj == NULL) {
				CHECK(soa.Self()->IsExceptionPending());
				return zero;
			}
			for (size_t i = 0; i < args.size(); ++i) {
				if (shorty[i + 1] == 'L') {
					jobject val = args.at(i).l;
					soa.Env()->SetObjectArrayElement(args_jobj, i, val);
				} else {
					JValue jv;
					jv.SetJ(args.at(i).j);
					Object* val = BoxPrimitive(Primitive::GetType(shorty[i + 1]),
							jv);
					if (val == NULL) {
						CHECK(soa.Self()->IsExceptionPending());
						return zero;
					}
					soa.Decode<ObjectArray<Object>*>(args_jobj)->Set<false>(i, val);
				}
			}
		}

		XposedHookInfo* hookInfo =
				(XposedHookInfo*) (soa.DecodeMethod(method)->GetNativeMethod());

		// Call XposedBridge.handleHookedMethod(Member method, int originalMethodId, Object additionalInfoObj,
		//                                      Object thisObject, Object[] args)
		jvalue invocation_args[5];
		invocation_args[0].l = hookInfo->reflectedMethod;
		invocation_args[1].i = 0;
		invocation_args[2].l = hookInfo->additionalInfo;
		invocation_args[3].l = rcvr_jobj;
		invocation_args[4].l = args_jobj;
		jobject result = soa.Env()->CallStaticObjectMethodA(xposed_class,
				xposed_handle_hooked_method, invocation_args);

		// Unbox the result if necessary and return it.
		if (UNLIKELY(soa.Self()->IsExceptionPending())) {
			return zero;
		} else {
			if (shorty[0] == 'V' || (shorty[0] == 'L' && result == NULL)) {
				return zero;
			}
			StackHandleScope < 1 > hs(soa.Self());
			MethodHelper mh_method(hs.NewHandle(soa.DecodeMethod(method)));
			// This can cause thread suspension.
			Object* rcvr = soa.Decode<Object*>(rcvr_jobj);
			ThrowLocation throw_location(rcvr, mh_method.GetMethod(), -1);
			Object* result_ref = soa.Decode<Object*>(result);
			Class* result_type = mh_method.GetReturnType();
			JValue result_unboxed;
			if (!UnboxPrimitiveForResult(throw_location, result_ref, result_type,
					&result_unboxed)) {
				DCHECK(soa.Self()->IsExceptionPending());
				return zero;
			}
			return result_unboxed;
		}
	}

	// Handler for invocation on proxy methods. On entry a frame will exist for the proxy object method
	// which is responsible for recording callee save registers. We explicitly place into jobjects the
	// incoming reference arguments (so they survive GC). We invoke the invocation handler, which is a
	// field within the proxy object, which will box the primitive arguments and deal with error cases.
	extern "C" uint64_t artQuickXposedInvokeHandler(ArtMethod* proxy_method,
			Object* receiver, Thread* self, StackReference<ArtMethod>* sp)
	SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {

		const bool is_static = proxy_method->IsStatic();
		// Ensure we don't get thread suspension until the object arguments are safely in jobjects.
		const char* old_cause = self->StartAssertNoThreadSuspension(
				"Adding to IRT proxy object arguments");
		// Register the top of the managed stack, making stack crawlable.
		DCHECK_EQ(sp->AsMirrorPtr(), proxy_method) << PrettyMethod(proxy_method);
		self->SetTopOfStack(sp, 0);
		DCHECK_EQ(proxy_method->GetFrameSizeInBytes(),
				Runtime::Current()->GetCalleeSaveMethod(Runtime::kRefsAndArgs)->GetFrameSizeInBytes())
				<< PrettyMethod(proxy_method);
		self->VerifyStack();
		// Start new JNI local reference state.
		JNIEnvExt* env = self->GetJniEnv();
		ScopedObjectAccessUnchecked soa(env);
		ScopedJniEnvLocalRefState env_state(env);
		// Create local ref. copies of proxy method and the receiver.
		jobject rcvr_jobj =
				is_static ? nullptr : soa.AddLocalReference < jobject > (receiver);

		// Placing arguments into args vector and remove the receiver.
		ArtMethod* non_proxy_method = proxy_method->GetInterfaceMethodIfProxy();

		std::vector < jvalue > args;
		uint32_t shorty_len = 0;
		const char* shorty = proxy_method->GetShorty(&shorty_len);
		BuildQuickArgumentVisitor local_ref_visitor(sp, false, shorty, shorty_len,
				&soa, &args);

		local_ref_visitor.VisitArguments();
		DCHECK_GT(args.size(), 0U) << PrettyMethod(proxy_method);
		if (!is_static) {
			args.erase(args.begin());
		}

		ArtMethod* mm = receiver->GetClass()->FindVirtualMethodForVirtualOrInterface(proxy_method);

		jmethodID proxy_methodid = soa.EncodeMethod(proxy_method);
		self->EndAssertNoThreadSuspension(old_cause);
		JValue result = xposedCallHandler(soa, shorty, rcvr_jobj,
				proxy_methodid, args);
		local_ref_visitor.FixupReferences();
		return result.GetJ();
	}

	static void com_taobao_android_dexposed_XposedBridge_hookMethodNative(
			JNIEnv* env, jclass, jobject java_method, jobject, jint,
			jobject additional_info) {

		LOG(INFO) << "xposed: >>> hookMethodNative";

		ScopedObjectAccess soa(env);
		art::Thread* self = art::Thread::Current();
		ArtMethod* method = ArtMethod::FromReflectedMethod(soa, java_method);
		if (method == NULL) {
			return;
		}
		if (method->IsStatic()) {
			StackHandleScope < 2 > shs(art::Thread::Current());
			Runtime::Current()->GetClassLinker()->EnsureInitialized(
					shs.NewHandle(method->GetDeclaringClass()), true, true);
		}
		if (xposedIsHooked(method)) {
			return;
		}

		XposedHookInfo* hookInfo = (XposedHookInfo*) calloc(1,
				sizeof(XposedHookInfo));

		ArtMethod* original_art_method =
				Runtime::Current()->GetClassLinker()->AllocArtMethod(self);
		memcpy(original_art_method, method, sizeof(ArtMethod));

		jobject reflect_method;
		if (method->IsConstructor()) {
			reflect_method = env->AllocObject(WellKnownClasses::java_lang_reflect_Constructor);
		} else {
			reflect_method = env->AllocObject(WellKnownClasses::java_lang_reflect_Method);
		}
		env->SetObjectField(reflect_method,
				WellKnownClasses::java_lang_reflect_AbstractMethod_artMethod,
				env->NewGlobalRef(
						soa.AddLocalReference < jobject > (original_art_method)));

		hookInfo->reflectedMethod = env->NewGlobalRef(java_method);
		hookInfo->additionalInfo = env->NewGlobalRef(additional_info);
		hookInfo->original_method = env->NewGlobalRef(reflect_method);

		method->SetNativeMethod((uint8_t*) hookInfo);

		method->SetEntryPointFromQuickCompiledCode(GetQuickXposedInvokeHandler());

		// Adjust access flags
		method->SetAccessFlags((method->GetAccessFlags() & ~kAccNative));
	}

	static bool xposedIsHooked(ArtMethod* method) {
		return (method->GetEntryPointFromQuickCompiledCode())
				== (void *) GetQuickXposedInvokeHandler();
	}

	jobject InvokeXposedMethod(const ScopedObjectAccessAlreadyRunnable& soa, jobject javaMethod,
	                     jobject javaReceiver, jobject javaArgs, bool accessible) {
	  // We want to make sure that the stack is not within a small distance from the
	  // protected region in case we are calling into a leaf function whose stack
	  // check has been elided.
	  if (UNLIKELY(__builtin_frame_address(0) <
	               soa.Self()->GetStackEndForInterpreter(true))) {
	    ThrowStackOverflowError(soa.Self());
	    return nullptr;
	  }
	  mirror::ArtMethod* m = mirror::ArtMethod::FromReflectedMethod(soa, javaMethod);

	  mirror::Class* declaring_class = m->GetDeclaringClass();
	  if (UNLIKELY(!declaring_class->IsInitialized())) {
	    StackHandleScope<1> hs(soa.Self());
	    Handle<mirror::Class> h_class(hs.NewHandle(declaring_class));
	    if (!Runtime::Current()->GetClassLinker()->EnsureInitialized(h_class, true, true)) {
	      return nullptr;
	    }
	    declaring_class = h_class.Get();
	  }
	  mirror::Object* receiver = nullptr;
	  if (!m->IsStatic()) {
	    // Check that the receiver is non-null and an instance of the field's declaring class.
	    receiver = soa.Decode<mirror::Object*>(javaReceiver);
	    if (!VerifyObjectIsClass(receiver, declaring_class)) {
	      return NULL;
	    }

	    // Find the actual implementation of the virtual method.
//	    m = receiver->GetClass()->FindVirtualMethodForVirtualOrInterface(m);
	  }
	  // Get our arrays of arguments and their types, and check they're the same size.
	  mirror::ObjectArray<mirror::Object>* objects =
	      soa.Decode<mirror::ObjectArray<mirror::Object>*>(javaArgs);
	  const DexFile::TypeList* classes = m->GetParameterTypeList();
	  uint32_t classes_size = (classes == nullptr) ? 0 : classes->Size();
	  uint32_t arg_count = (objects != nullptr) ? objects->GetLength() : 0;
	  if (arg_count != classes_size) {
	    ThrowIllegalArgumentException(NULL,
	                                  StringPrintf("Wrong number of arguments; expected %d, got %d",
	                                               classes_size, arg_count).c_str());
	    return NULL;
	  }

	  // If method is not set to be accessible, verify it can be accessed by the caller.
	  if (!accessible && !VerifyAccess(soa.Self(), receiver, declaring_class, m->GetAccessFlags())) {
	    ThrowIllegalAccessException(nullptr, StringPrintf("Cannot access method: %s",
	                                                      PrettyMethod(m).c_str()).c_str());
	    return nullptr;
	  }
	  // Invoke the method.
	  JValue result;
	  uint32_t shorty_len = 0;
	  const char* shorty = m->GetShorty(&shorty_len);
	  art::ArgArray arg_array(shorty, shorty_len);
	  StackHandleScope<1> hs(soa.Self());
	  MethodHelper mh(hs.NewHandle(m));
	  if (!arg_array.BuildArgArrayFromObjectArray(soa, receiver, objects, mh)) {
	    CHECK(soa.Self()->IsExceptionPending());
	    return nullptr;
	  }
	  InvokeWithArgArray(soa, m, &arg_array, &result, shorty);
	  // Wrap any exception with "Ljava/lang/reflect/InvocationTargetException;" and return early.
	  if (soa.Self()->IsExceptionPending()) {
	    jthrowable th = soa.Env()->ExceptionOccurred();
	    soa.Env()->ExceptionClear();
	    jclass exception_class = soa.Env()->FindClass("java/lang/reflect/InvocationTargetException");
	    jmethodID mid = soa.Env()->GetMethodID(exception_class, "<init>", "(Ljava/lang/Throwable;)V");
	    jobject exception_instance = soa.Env()->NewObject(exception_class, mid, th);
	    soa.Env()->Throw(reinterpret_cast<jthrowable>(exception_instance));
	    return NULL;
	  }
	  // Box if necessary and return.
	  return soa.AddLocalReference<jobject>(BoxPrimitive(mh.GetReturnType()->GetPrimitiveType(),
	                                                     result));
	}

	extern "C" jobject com_taobao_android_dexposed_XposedBridge_invokeOriginalMethodNative(
			JNIEnv* env, jclass, jobject java_method, jint, jobject, jobject,
			jobject thiz, jobject args)
	SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {

		LOG(INFO) << "xposed: >>> invokeOriginalMethodNative";

		ScopedObjectAccess soa(env);

		ArtMethod* method = ArtMethod::FromReflectedMethod(soa, java_method);

		XposedHookInfo* hookInfo = (XposedHookInfo*) method->GetNativeMethod();

		return InvokeXposedMethod(soa, hookInfo->original_method, thiz, args, true);
	}

	extern "C" jobject com_taobao_android_dexposed_XposedBridge_invokeSuperNative(
			JNIEnv* env, jclass, jobject thiz, jobject args, jobject java_method, jobject, jobject,
			jint slot, jboolean check)

	SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {

		LOG(INFO) << "xposed: >>> invokeNonVirtualNative";

		ScopedObjectAccess soa(env);
		ArtMethod* method = ArtMethod::FromReflectedMethod(soa, java_method);

		// Find the actual implementation of the virtual method.
		ArtMethod* mm = method->FindOverriddenMethod();

		jobject reflect_method = env->AllocObject(WellKnownClasses::java_lang_reflect_Method);
		env->SetObjectField(reflect_method,
						WellKnownClasses::java_lang_reflect_AbstractMethod_artMethod,
						env->NewGlobalRef(
								soa.AddLocalReference < jobject > (mm)));

		return InvokeXposedMethod(soa, reflect_method, thiz, args, true);
	}

	static const JNINativeMethod xposedMethods[] =
	{
		{ "initNative", "()Z", (void*) com_taobao_android_dexposed_XposedBridge_initNative },
		{ "hookMethodNative", "(Ljava/lang/reflect/Member;Ljava/lang/Class;ILjava/lang/Object;)V",
							(void*) com_taobao_android_dexposed_XposedBridge_hookMethodNative },
		{ "invokeOriginalMethodNative",
				  "(Ljava/lang/reflect/Member;I[Ljava/lang/Class;Ljava/lang/Class;Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;",
							(void*) com_taobao_android_dexposed_XposedBridge_invokeOriginalMethodNative },
		{ "invokeSuperNative", "(Ljava/lang/Object;[Ljava/lang/Object;Ljava/lang/reflect/Member;Ljava/lang/Class;[Ljava/lang/Class;Ljava/lang/Class;I)Ljava/lang/Object;",
				(void*) com_taobao_android_dexposed_XposedBridge_invokeSuperNative},
	};

	static int register_com_taobao_android_dexposed_XposedBridge(JNIEnv* env) {
		return env->RegisterNatives(xposed_class, xposedMethods, sizeof(xposedMethods) / sizeof(xposedMethods[0]));
	}
}

