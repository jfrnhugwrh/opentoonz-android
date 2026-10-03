#=============================================================================
#
#  Android support libraries.
#
#  Two static libraries are built here, both of them only for Android:
#
#    ot_gl_compat  - the OpenGL ES compatibility layer.  OpenToonz renders
#                    through the desktop OpenGL fixed function pipeline and
#                    GLU; neither exists on OpenGL ES, so the entry points are
#                    re-implemented on top of ES 3.0.  This is what allows the
#                    rendering core to stay untouched.
#
#    ot_android_platform - the JNI bridge to the storage services (SAF,
#                    MediaStore, application storage).
#
#  The third party libraries the rest of the build links against are set up in
#  android_thirdparty.cmake, which is included before this file.
#
#=============================================================================

set(OT_GL_COMPAT_DIR ${CMAKE_CURRENT_LIST_DIR}/gl_compat)
set(OT_PLATFORM_DIR   ${CMAKE_CURRENT_LIST_DIR}/platform)

#-----------------------------------------------------------------------------
# opengl es compatibility layer
#-----------------------------------------------------------------------------

add_library(ot_gl_compat STATIC
    ${OT_GL_COMPAT_DIR}/src/gl_fixed_function.cpp
    ${OT_GL_COMPAT_DIR}/src/glu_impl.cpp
    ${OT_GL_COMPAT_DIR}/src/glut_impl.cpp
    ${OT_GL_COMPAT_DIR}/src/glew_impl.cpp
    ${OT_GL_COMPAT_DIR}/src/gles_resolve_android.cpp
)

# The shim headers must come before any system GL header, so they are added as
# a normal (non SYSTEM) include directory of every target that draws.
target_include_directories(ot_gl_compat PUBLIC ${OT_GL_COMPAT_DIR}/include)
target_include_directories(ot_gl_compat PRIVATE ${OT_GL_COMPAT_DIR}/src)

target_compile_definitions(ot_gl_compat PUBLIC
    # The application links against libGLESv3; the shim supplies the legacy
    # entry points on top of it.
    -DGL_GLEXT_PROTOTYPES=1
)

#-----------------------------------------------------------------------------
# android platform layer (JNI bridge)
#-----------------------------------------------------------------------------

add_library(ot_android_platform STATIC
    ${OT_PLATFORM_DIR}/otandroid_jni.cpp
    ${OT_PLATFORM_DIR}/otandroid_storage.cpp
)

target_include_directories(ot_android_platform PUBLIC
    ${OT_PLATFORM_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_link_libraries(ot_android_platform PUBLIC
    Qt5::Core
    android
    log
)

message(STATUS "subdir: android")
