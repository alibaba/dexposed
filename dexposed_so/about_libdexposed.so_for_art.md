libdexposed.so for Art 
-----------
Due to ART's complexity, libdexposed.so for art in Master branch has some critical defect, it may bring a lot of maintenance effort. After do some research into the art source code, and some open source library. I think there still has some change to do ArtMethod AOP hook. Please refer to the dev_art branch. I upload some prototype for hook ArtMethod. Current now it still is in beta, and didn't do a lot test.

**Don't use it in production environment.**

-----
In dev_art branch, we use c java to hook artmethod, and we can compile in Android Studio, didn't need AOSP code for compile the libdexposed.so any more. This may be more convenient. 


Hook Principle
-----
* The hook stub is still the entry_point_from_compiled_code_
* Like inline hook in secutiry world, we use arm code to hook this stub, let it redirect to the zone we mmap, then jump to my c code to do some argument box,  and call the hook method.

Contribute
----------
We are open to constructive contributions from the community, especially pull request
and quality bug reports. We value your help to test or improve the implementation for ART.

Dexposed is aimed to be lightweight, transparent and productive. All improvements with
these principal in mind are welcome.

Discuss
---------
I create an [issue https://github.com/alibaba/dexposed/issues/8](https://github.com/alibaba/dexposed/issues/8)，we can discuss there! Thank you!
