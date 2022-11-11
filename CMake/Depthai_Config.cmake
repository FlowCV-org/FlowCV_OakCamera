# Luxonis Depthai external Library Dependency

IF( DEFINED ENV{depthai_DIR} )
    SET( depthai_DIR "$ENV{depthai_DIR}" )
ELSE()
    find_path(depthai_DIR "depthai_DIR")
ENDIF()

if ("${depthai_DIR}" STREQUAL "depthai_DIR-NOTFOUND")
    if(WIN32)
        message(FATAL_ERROR "No depthai_DIR Found")
    elseif(APPLE)
        message(FATAL_ERROR "No depthai_DIR Found")
    else()
        # Placeholder for Linux Depthai
    endif()
else()
    set(depthai_DIR ${depthai_DIR})
endif()

add_compile_definitions(DEPTHAI_HAVE_OPENCV_SUPPORT)

# Depthai Dir
message(STATUS "Depthai SDK Dir: ${depthai_DIR}")

# Configure include paths
include_directories(${depthai_DIR}/include)
include_directories(${depthai_DIR}/include/depthai-shared/3rdparty)
# Make sure depthai include search paths come after FlowCV to avoid json library conflict
include_directories(AFTER ${depthai_DIR}/lib/cmake/depthai/dependencies/include)

# Configure lib paths
link_directories(${depthai_DIR}/lib)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (WIN32)
        LIST(APPEND DEPTHAI_LIBS ${depthai_DIR}/lib/depthai-cored.lib)
        LIST(APPEND DEPTHAI_LIBS ${depthai_DIR}/lib/depthai-opencvd.lib)
        file(COPY "${depthai_DIR}/bin/depthai-cored.dll" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")
        file(COPY "${depthai_DIR}/bin/depthai-opencvd.dll" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")
        file(COPY "${depthai_DIR}/bin/libusb-1.0d.dll" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")
    elseif(APPLE)

    else()
        LIST(APPEND DEPTHAI_LIBS libdepthai-cored.so)
        LIST(APPEND DEPTHAI_LIBS libdepthai-opencvd.so)
        file(COPY "${depthai_DIR}/lib/libdepthai-cored.so" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Oak_Camera/")
        file(COPY "${depthai_DIR}/lib/libdepthai-opencvd.so" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Oak_Camera/")
        file(COPY "${depthai_DIR}/lib/cmake/depthai/dependencies/lib/libusbd-1.0.so" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Oak_Camera/")
    endif()
else()
    if (WIN32)
        LIST(APPEND DEPTHAI_LIBS ${depthai_DIR}/lib/depthai-core.lib)
        LIST(APPEND DEPTHAI_LIBS ${depthai_DIR}/lib/depthai-opencv.lib)
        file(COPY "${depthai_DIR}/bin/depthai-core.dll" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")
        file(COPY "${depthai_DIR}/bin/depthai-opencv.dll" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")
        file(COPY "${depthai_DIR}/bin/libusb-1.0.dll" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")
    elseif(APPLE)

    else()
        LIST(APPEND DEPTHAI_LIBS libdepthai-core.so)
        LIST(APPEND DEPTHAI_LIBS libdepthai-opencv.so)
        file(COPY "${depthai_DIR}/lib/libdepthai-core.so" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Oak_Camera/")
        file(COPY "${depthai_DIR}/lib/libdepthai-opencv.so" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Oak_Camera/")
        file(COPY "${depthai_DIR}/lib/cmake/depthai/dependencies/lib/libusb-1.0.so" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Oak_Camera/")
    endif()
endif()

