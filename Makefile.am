#config to create cYandexDisk lib with automake
ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES  = libcYandexOAuth.la
libcYandexOAuth_la_SOURCES = \
		cYandexOAuth.h cYandexOAuth.c\
	  cJSON.h cJSON.c\
	  uuid4.h uuid4.c

libcYandexOAuth_la_CFLAGS = -fPIC

if WIN32
AM_CFLAGS = \
	-Icurl/windows/win32/include
CURL_LINK = -lcurl -L`pwd`/curl/windows/win32/bin/libcurl.dll
endif

if WIN64
AM_CFLAGS = \
	-Icurl/windows/win64/include
CURL_LINK = -lcurl -L`pwd`/curl/windows/win64/bin/libcurl.dll
endif

if WINDOWS
libcYandexOAuth_la_LDFLAGS = -Wl,-no-undefined
else
CURL_LINK = -lcurl
endif

libcYandexOAuth_la_LIBADD = $(CURL_LINK)
