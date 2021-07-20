:: File: make.bat
:: Statically compile TerminalChat for Windows
:: Copyright (C) 2021 Muhammad Bin Zafar <midnightquantumprogrammer@gmail.com>


:: -------- Init --------
@echo off


set out=.\bin
set outFn=chatnet_win.exe
if not exist %out%\ (
	mkdir %out%
)
set mainFn=.\chatnet-client.cpp
set mainCmd=x86_64-w64-mingw32-gcc-10.2.0.exe  -static  -o %out%\%outFn% -DCURL_STATICLIB %mainFn% 


set curlDeps=curl ssl crypto brotlienc-static brotlidec-static brotlicommon-static gsasl idn2 ssh2 nghttp2 z zstd
set lib=.\lib-win32
for %%i in (%curlDeps%) do (
	call set "_curlDeps=%%_curlDeps%% %%lib%%\lib%%i.a"
)
set winDeps= -lws2_32 -lwldap32 -ladvapi32 -lkernel32  -lcomdlg32 -lcrypt32 -lnormaliz



%mainCmd% %_curlDeps% %winDeps%


:: x86_64-w64-mingw32-gcc-10.2.0.exe -static   -o chatnet-win.exe -DCURL_STATICLIB  main.c .\lib\libcurl.a  .\lib\libssl.a .\lib\libcrypto.a  .\lib\libbrotlienc-static.a    .\lib\libbrotlidec-static.a   .\lib\libbrotlicommon-static.a  .\lib\libgsasl.a .\lib\libidn2.a .\lib\libssh2.a .\lib\libnghttp2.a .\lib\libz.a .\lib\libzstd.a      -lws2_32 -lwldap32 -ladvapi32 -lkernel32  -lcomdlg32 -lcrypt32 -lnormaliz
