project(Oak_Camera)

find_package( FlowCV REQUIRED )

add_library(
        ${PROJECT_NAME} SHARED
        oak_camera.cpp
        oak_plugin.cpp
        ${IMGUI_SRC}
        ${DSPatch_SRC}
        ${IMGUI_WRAPPER_SRC}
        ${IMGUI_OPENCV_SRC}
        ${FlowCV_SRC}
)
# Fix to make sure FlowCV include paths come first to avoid version mismatch from conflicting libraries
target_include_directories(${PROJECT_NAME} BEFORE PRIVATE ${FlowCV_DIR}/third-party)

target_link_libraries(
        ${PROJECT_NAME}
        ${IMGUI_LIBS}
        ${OpenCV_LIBS}
        ${DEPTHAI_LIBS}
)

if(WIN32)
set_target_properties(${PROJECT_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        SUFFIX ".fp"
        )
elseif(UNIX AND NOT APPLE)
    set_target_properties(${PROJECT_NAME}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
            SUFFIX ".fp"
            INSTALL_RPATH "${ORIGIN}"
            BUILD_WITH_INSTALL_RPATH ON
            )
elseif(APPLE)
    set_target_properties(${PROJECT_NAME}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
            SUFFIX ".fp"
            INSTALL_NAME_DIR "${ORIGIN}"
            BUILD_WITH_INSTALL_NAME_DIR ON
            )
endif()