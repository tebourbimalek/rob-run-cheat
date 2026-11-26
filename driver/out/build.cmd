@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /c /kernel /GS- /GR- /EHs-c- /Ox /W4 /D_WIN64 /DAMD64 /D_AMD64_ /DPOOL_NX_OPTIN=1 /DNDEBUG /I"C:\Users\GIGABYTE\Desktop\new-rob-runhack\driver\ext\wdk\c\Include\10.0.26100.0\km" /Fo"C:\Users\GIGABYTE\Desktop\new-rob-runhack\driver\out\rb_drv.obj" /Fd"C:\Users\GIGABYTE\Desktop\new-rob-runhack\driver\out\rb_drv.pdb" "C:\Users\GIGABYTE\Desktop\new-rob-runhack\driver\rb_drv.cpp"
if errorlevel 1 exit /b 3
link /nologo /DRIVER /SUBSYSTEM:NATIVE /MACHINE:X64 /ENTRY:DriverEntry /NODEFAULTLIB:libcntpr /LIBPATH:"C:\Users\GIGABYTE\Desktop\new-rob-runhack\driver\ext\wdk\c\Lib\10.0.26100.0\km\x64" /OUT:"C:\Users\GIGABYTE\Desktop\new-rob-runhack\driver\out\rb_drv.sys" "C:\Users\GIGABYTE\Desktop\new-rob-runhack\driver\out\rb_drv.obj" ntoskrnl.lib
if errorlevel 1 exit /b 4
exit /b 0
