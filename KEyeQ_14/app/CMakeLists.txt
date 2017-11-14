cmake_minimum_required(VERSION 3.4.1)

add_library( native-lib

             SHARED

             src/main/cpp/native-lib.cpp )


find_library(
              log-lib
              log )


add_library(
    libusb
    SHARED
    IMPORTED
    )
set_target_properties(
    libusb
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_CURRENT_LIST_DIR}/src/main/libs/${ANDROID_ABI}/libusb.so
    )

include_directories( ${CMAKE_CURRENT_LIST_DIR}/src/main/cpp/usb/ )

add_library( ezusb-lib

             SHARED

             src/main/cpp/ezusb-lib.cpp )


find_library(
              log-lib
              log )

add_library( fxload-lib

             SHARED

             src/main/cpp/fxload-lib.cpp )


find_library(
              log-lib
              log )

target_link_libraries( # Specifies the target library.
                       native-lib
                       libusb
                       ezusb-lib
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )