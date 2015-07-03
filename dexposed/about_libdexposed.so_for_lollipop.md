libdexposed.so for Lollipop 
-----------
Due to ART's complexity, libdexposed.so for lollipop is currently only a beta version. **Don't use it in production environment.**

Flaws
-----
* Some optimizations in the Ahead-of-Time compilation make it harder to hook all methods. One example is the inlined "easy" (short) methods. Another example is the direct call into the native entry point in the assembly code. Also, the code deduplication, may hooking one method could also hook other methods. It can't hook now.
* Targets other than "quick, 32-bits, arm" are not supported yet. (TARGET_CPU_SMP=true for example)
* Hooked methods are similar to proxy methods in many aspects. so we don't need to deal with the stack layout by ourselves. "Special" methods (i.e. proxy, native method) is not supported.
* ART in Android 5.1 is not supported yet, due to huge code base changes since Lollipop.

Current state
-------------
We are still working hard on eliminating the flaws. This page will be updated if progress has been made.

Contribute
----------
We are open to constructive contributions from the community, especially pull request
and quality bug reports. We value your help to test or improve the implementation for ART.

Dexposed is aimed to be lightweight, transparent and productive. All improvements with
these principal in mind are welcome.

Discuss
---------
I create an [issue https://github.com/alibaba/dexposed/issues/8](https://github.com/alibaba/dexposed/issues/8)，we can discuss there! Thank you!
