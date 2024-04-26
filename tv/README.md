Prerequisites

You must install boost and download opencv
C:\prg\libc\boost_1_85_0
C:\prg\libc\opencv

In Visual Studio settings you must specify as follows:

C++ Language Standard: ISO C++20 Standard

VC++ Directories --- Include Directories:
  includes;C:\prg\libc\boost_1_85_0;$(VC_IncludePath);$(WindowsSDK_IncludePath);
VC++ Directories --- Library Directories:
  lib;$(VC_LibraryPath_x64);$(WindowsSDK_LibraryPath_x64)
Linker --- General --- Additional Library Directories
  C:\prg\libc\opencv\build\x64\vc16\bin;C:\prg\libc\boost_1_85_0\stage\lib


