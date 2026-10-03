#=============================================================================
#
#  Host build of the OpenGL ES compatibility layer smoke test.
#
#  This project is configured on its own, with the *host* compiler, by the
#  Android workflow: it lets the parts of the shim that carry real algorithmic
#  responsibility (the GLU tessellator and the projection maths) be verified
#  without a device or an emulator.
#
#      cmake -S toonz/sources/android/gl_compat/test -B build/gl_compat_test
#      cmake --build build/gl_compat_test
#      ./build/gl_compat_test/ot_gl_compat_test
#
#=============================================================================

cmake_minimum_required(VERSION 3.10)
project(ot_gl_compat_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OT_GL_COMPAT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

add_executable(ot_gl_compat_test
    test_tessellator.cpp
    mock_gles.cpp
    ${OT_GL_COMPAT_DIR}/src/gl_fixed_function.cpp
    ${OT_GL_COMPAT_DIR}/src/glu_impl.cpp
    ${OT_GL_COMPAT_DIR}/src/glut_impl.cpp
    ${OT_GL_COMPAT_DIR}/src/glew_impl.cpp
)

target_include_directories(ot_gl_compat_test PRIVATE
    ${OT_GL_COMPAT_DIR}/include
    ${OT_GL_COMPAT_DIR}/src
)

# Exercises the mock backend instead of a real OpenGL ES driver.
target_compile_definitions(ot_gl_compat_test PRIVATE ANDROID_GL_COMPAT_MOCK_GLES=1)

enable_testing()
add_test(NAME gl_compat COMMAND ot_gl_compat_test)
