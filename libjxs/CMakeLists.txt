# ============================================================================
# CMake Build Files for JPEG XS Reference Software (jpeg-xsm).
# 
# See LICENSE.md for copyright and licensing conditions.
# ============================================================================

set(JXS_SRC
    public/libjxs.h
    src/bitpacking.c
    src/bitpacking.h
    src/budget.c
    src/budget.h
    src/buf_mgmt.c
    src/buf_mgmt.h
    src/common.h
    src/data_budget.c
    src/data_budget.h
    src/dwt.c
    src/dwt.h
    src/gcli_budget.c
    src/gcli_budget.h
    src/gcli_methods.c
    src/gcli_methods.h
    src/ids.c
    src/ids.h
    src/image.c
    src/mct.c
    src/mct.h
    src/nlt.c
    src/nlt.h
    src/packing.c
    src/packing.h
    src/precinct.c
    src/precinct.h
    src/precinct_budget.c
    src/precinct_budget.h
    src/precinct_budget_table.c
    src/precinct_budget_table.h
    src/pred.c
    src/pred.h
    src/predbuffer.c
    src/predbuffer.h
    src/quant.c
    src/quant.h
    src/rate_control.c
    src/rate_control.h
    src/sb_weighting.c
    src/sb_weighting.h
    src/sig_flags.c
    src/sig_flags.h
    src/sigbuffer.c
    src/sigbuffer.h
    src/version.c
    src/version.h
    src/xs_config.c
    src/xs_config.h
    src/xs_config_parser.c
    src/xs_dec.c
    src/xs_enc.c
    src/xs_gains.h
    src/xs_markers.c
    src/xs_markers.h
)

if(DEFINED XS_ENABLE_THEORETICAL_GAINS_CALCULATION_SRC)
    message(STATUS "Found an implementation of XS gains, enabling inclusion.")
    list(APPEND JXS_SRC "${XS_ENABLE_THEORETICAL_GAINS_CALCULATION_SRC}")
    source_group("Private external sources" FILES "${XS_ENABLE_THEORETICAL_GAINS_CALCULATION_SRC}")
endif()

#add_library(jxs SHARED ${JXS_SRC})
add_library(jxs STATIC ${JXS_SRC})
target_include_directories(jxs PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
target_include_directories(jxs PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(jxs PUBLIC ${M_LIBRARY})

if(DEFINED XS_ENABLE_THEORETICAL_GAINS_CALCULATION_SRC)
    target_compile_definitions(jxs PRIVATE "XS_THEORETICAL_GAINS_CALCULATION=1")
endif()

if(DEFINED XS_CUSTOM_BUILD_IDENTIFIER)
    target_compile_definitions(jxs PRIVATE "XS_CUSTOM_BUILD_IDENTIFIER=\"${XS_CUSTOM_BUILD_IDENTIFIER}\"")
endif()

if(MSVC)
    # Build with DLL exporting in mind, works also for static Windows lib
    target_compile_definitions(jxs PRIVATE "JXS_SHARED_LIB_API=__declspec(dllexport)")
endif()

source_group("Private sources" REGULAR_EXPRESSION "src/[a-zA-Z0-9_]+\\.\(c|cpp|h\)")
source_group("Public headers" REGULAR_EXPRESSION "public/[a-zA-Z0-9_]+\\.\(h\)")
set_target_properties(jxs PROPERTIES FOLDER Libraries)
