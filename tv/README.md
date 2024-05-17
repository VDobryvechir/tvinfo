Prerequisites

You must install boost and download opencv
C:\prg\libc\boost_1_85_0
C:\prg\libc\opencv

To install boost,
1) download it from https://www.boost.org/users/download/ (zip file)
2) Extract the content of the archive to C:\prg\libc\boost_1_85_0
3) Enter the folder and run bootstrap.bat, wait, it will build and create b2.exe in the same folder
4) Run b2.exe without any parameters
5) Wait and it is done.
 
In Visual Studio settings you must specify as follows:

C++ Language Standard: ISO C++20 Standard

VC++ Directories --- Include Directories:
  includes;C:\prg\libc\boost_1_85_0;$(VC_IncludePath);$(WindowsSDK_IncludePath);
VC++ Directories --- Library Directories:
  lib;$(VC_LibraryPath_x64);$(WindowsSDK_LibraryPath_x64)
Linker --- General --- Additional Library Directories
  C:\prg\libc\opencv\build\x64\vc16\bin;C:\prg\libc\boost_1_85_0\stage\lib


