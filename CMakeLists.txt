cmake_minimum_required(VERSION 3.5)

project(Allocator)

configure_file(before_deploy.sh.in before_deploy.sh COPYONLY)

add_executable(allocator_traits main.cpp)
set_target_properties(allocator_traits PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)


##----------------------------------------------------------
## Handle warnings as errors
if (MSVC)
    target_compile_options(allocator_traits PRIVATE
        /W4
    )
else ()
    target_compile_options(allocator_traits PRIVATE
        -Wall -Wextra -pedantic -Werror
    )
endif()
##----------------------------------------------------------


install(TARGETS allocator_traits RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_NAME "${AK_DEB_PKG_NAME}")

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT example@example.com)

include(CPack)
