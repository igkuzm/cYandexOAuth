cmake_minimum_required(VERSION 3.0)

set(TARGET cYandexOAuth)

#To include cYandexOAuth to your project use:
#set(CYANDEXOAUTH_SOURCE_DIR ${CMAKE_SOURCE_DIR}/path/to/cYandexOAuth)
#add_subdirectory(${CYANDEXOAUTH_SOURCE_DIR})

if (NOT DEFINED CYANDEXOAUTH_SOURCE_DIR)
	set(CYANDEXOAUTH_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif ()

project(
	${TARGET} VERSION 1.0 
	DESCRIPTION "C API for Yandex OAuth2 authorisation"
	HOMEPAGE_URL ""
	LANGUAGES C
)

set (CMAKE_C_STANDARD 99)

# Determine the location of the libraries to use based on the platform.
if(ANDROID)
	ADD_LIBRARY(curl STATIC IMPORTED)
	SET_TARGET_PROPERTIES(curl PROPERTIES IMPORTED_LOCATION ${CYANDEXOAUTH_SOURCE_DIR}/curl/android/${ANDROID_ABI}/libcurl.a)
	INCLUDE_DIRECTORIES(${CYANDEXOAUTH_SOURCE_DIR}/curl/android/include)
elseif(APPLE)
  if(IOS)
	ADD_LIBRARY(curl STATIC IMPORTED)
	SET_TARGET_PROPERTIES(curl PROPERTIES IMPORTED_LOCATION ${CYANDEXOAUTH_SOURCE_DIR}/curl/iOS/libcurl.a)
	INCLUDE_DIRECTORIES(${CYANDEXMUSIC_SOURCE_DIR}/curl/iOS/include)
  endif()
elseif(WIN32)
	#add strptime
	set(ADDSRC strptime.cc)
	ADD_LIBRARY(curl STATIC IMPORTED)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8) #64 bit	
		SET_TARGET_PROPERTIES(curl PROPERTIES IMPORTED_LOCATION
			${CYANDEXMUSIC_SOURCE_DIR}/curl/windows/win64/lib/libcurl.dll.a)
		INCLUDE_DIRECTORIES(${CYANDEXMUSIC_SOURCE_DIR}/curl/windows/win64/include)
		#SET(ADDDIRS ${CYANDEXMUSIC_SOURCE_DIR}/curl/windows/win64/lib)
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) #32 bit
		SET_TARGET_PROPERTIES(curl PROPERTIES IMPORTED_LOCATION
			${CYANDEXMUSIC_SOURCE_DIR}/curl/windows/win32/lib/libcurl.dll.a)	
		INCLUDE_DIRECTORIES(${CYANDEXMUSIC_SOURCE_DIR}/curl/windows/win32/include)
		#SET(ADDDIRS ${CYANDEXMUSIC_SOURCE_DIR}/curl/windows/win32/lib)
	endif()
endif()

if(APPLE)
	SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -framework Foundation")
endif()

add_library(${TARGET} STATIC cYandexMusic.c cJSON.c uuid4.c ${ADDSRC})
target_link_libraries(${TARGET} curl z)
target_link_directories(${TARGET} PRIVATE ${ADDDIRS})

if(${CYD_BUILD_TEST})
	add_executable(cYandexMusic_test test.c)
	target_link_libraries(cYandexMusic_test ${TARGET})
endif()

#copy files
if (WIN32)
	#curl libs
	#Copy DLLs
	if(CMAKE_SIZEOF_VOID_P EQUAL 8) #64 bit	
		file(GLOB files 
			"${CYANDEXMUSIC_SOURCE_DIR}/curl/windows/win64/lib/*.dll"
			)	
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) #32 bit
		file(GLOB files 
			"${CYANDEXMUSIC_SOURCE_DIR}/curl/windows/win32/lib/*.dll"
			)	
	endif()	
	#Copy
	foreach(_file ${files})
			file(INSTALL
				DESTINATION "${CMAKE_BINARY_DIR}"
				FILES "${_file}"
			)
	endforeach()	
endif()
