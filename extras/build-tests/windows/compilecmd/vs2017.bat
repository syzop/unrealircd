rem Build command for Visual Studio 2017

nmake -f makefile.win32 ^
LIBRESSL_INC_DIR="c:\projects\unrealircd-deps\libressl\include" ^
LIBRESSL_LIB_DIR="c:\projects\unrealircd-deps\libressl\lib" ^
SSLLIB="crypto-44.lib ssl-46.lib" ^
USE_REMOTEINC=1 ^
LIBCURL_INC_DIR="c:\projects\unrealircd-deps\curl-ssl\include" ^
LIBCURL_LIB_DIR="c:\projects\unrealircd-deps\curl-ssl\builds\libcurl-vc-x86-release-dll-ssl-dll-ipv6-sspi-obj-lib" ^
CARES_LIB_DIR="c:\projects\unrealircd-deps\c-ares\msvc\cares\dll-release" ^
CARES_INC_DIR="c:\projects\unrealircd-deps\c-ares" ^
CARESLIB="cares.lib" ^
TRE_LIB_DIR="c:\projects\unrealircd-deps\tre\win32\release" ^
TRE_INC_DIR="c:\projects\unrealircd-deps\tre" ^
TRELIB="tre.lib" ^
PCRE2_INC_DIR="c:\projects\unrealircd-deps\pcre2\include" ^
PCRE2_LIB_DIR="c:\projects\unrealircd-deps\pcre2\lib" ^
PCRE2LIB="pcre2-8.lib" ^
ARGON2_LIB_DIR="c:\projects\unrealircd-deps\argon2\vs2015\build" ^
ARGON2_INC_DIR="c:\projects\unrealircd-deps\argon2\include" ^
ARGON2LIB="Argon2RefDll.lib" %*
