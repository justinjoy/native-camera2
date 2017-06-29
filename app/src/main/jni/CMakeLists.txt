cmake_minimum_required(VERSION 3.4.1)

add_library(native-camera2-jni SHARED 
            native-camera2-jni.cpp)

target_link_libraries(native-camera2-jni
                      android
                      log
                      camera2ndk)
