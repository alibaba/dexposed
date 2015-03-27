libdexposed.so for Lollipop 
-----------
Due to ART's complexity, libdexposed.so for lollipop current now is only a trail version. **It's not stable**.

Faults
-----------------
* Some optimizations in the Ahead-of-Time-Compilation  make it hard to hook all methods. One example is that ART inlines "easy"/short methods into the caller, or places direct calls to the native entry point in the assembler code. Also, the code deduplication, may hooking one method could also hook other methods. It can't hook now.
* The target, For example, TARGET_CPU_SMP=true (needed for multicore processors) didn't support. Now it only support "quick, 32-bits, arm".
* Hooked methods are similar to proxy methods in many aspects. so we don't have to care about the stack layout ourself. A method is known to be "special" (i.e. a proxy or native method) didn't support.
* It didn't compatible with android5.1 which have changed a lot from lollipop.

Future
-----------
Up to now, we still work to solve these faults. We think there is still a hope to resolved，although not easy.

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