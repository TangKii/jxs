# ============================================================================
# CMake Build Files for JPEG XS Reference Software (jpeg-xsm).
# 
# See LICENSE.md for copyright and licensing conditions.
# ============================================================================

set(JXS_UTILS_SRC
    cmdline_options.c
    cmdline_options.h
    converters/argb.c
    converters/argb.h
    converters/helpers.c
    converters/helpers.h
    converters/mono.c
    converters/mono.h
    converters/pgx.c
    converters/pgx.h
    converters/planar.c
    converters/planar.h
    converters/ppm.c
    converters/ppm.h
    converters/rgb16.c
    converters/rgb16.h
    converters/uyvy8.c
    converters/uyvy8.h
    converters/v210.c
    converters/v210.h
    converters/yuv16.c
    converters/yuv16.h
    file_io.c
    file_io.h
    file_sequence.c
    file_sequence.h
    ../std/getopt.h
    image_open.c
    image_open.h
)

add_library(jxs_utils STATIC ${JXS_UTILS_SRC})
target_include_directories(jxs_utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../std")
target_include_directories(jxs_utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(jxs_utils PUBLIC jxs ${M_LIBRARY})

# if(MSVC)
#     # Build with DLL exporting in mind, works also for static Windows lib
#     target_compile_definitions(jxs_utils PRIVATE "JXS_SHARED_LIB_API=__declspec(dllexport)")
# endif()

add_executable(jxs_encoder xs_enc_main.c)
target_link_libraries(jxs_encoder jxs jxs_utils)

add_executable(jxs_decoder xs_dec_main.c)
target_link_libraries(jxs_decoder jxs jxs_utils)

source_group("Private sources" REGULAR_EXPRESSION "[a-zA-Z0-9_]+\\.\(c|cpp|h\)")
source_group("Private sources//converters" REGULAR_EXPRESSION "converters/[a-zA-Z0-9_]+\\.\(c|cpp|h\)")
source_group("Private sources//std" REGULAR_EXPRESSION "std/[a-zA-Z0-9_]+\\.\(c|cpp|h\)")

set_target_properties(jxs_utils PROPERTIES FOLDER Libraries)
set_target_properties(jxs_encoder PROPERTIES FOLDER Executables)
set_target_properties(jxs_decoder PROPERTIES FOLDER Executables)
