function(enable_warnings_gnu)
  target_compile_options(
    ${ARGN}
    "-Wall"
    "-Wcast-align"
    "-Wcast-qual"
    "-Wconversion"
    "-Wctor-dtor-privacy"
    "-Wdisabled-optimization"
    "-Weffc++"
    "-Wextra"
    "-Wfloat-equal"
    "-Wformat=2"
    "-Wimport"
    "-Winvalid-pch"
    "-Wlogical-op"
    "-Wmissing-format-attribute"
    "-Wmissing-include-dirs"
    "-Wmissing-noreturn"
    "-Woverloaded-virtual"
    "-Wpacked"
    "-Wpointer-arith"
    "-Wredundant-decls"
    "-Wshadow"
    "-Wsign-conversion"
    "-Wsign-promo"
    "-Wstack-protector"
    "-Wstrict-aliasing=2"
    "-Wstrict-null-sentinel"
    "-Wstrict-overflow"
    "-Wswitch"
    "-Wundef"
    "-Wunreachable-code"
    "-Wunused"
    "-Wvariadic-macros"
    "-Wwrite-strings"
    "-pedantic"
    "-pedantic-errors"
  )
endfunction()

function(enable_warnings_clang)
  target_compile_options(
    ${ARGN}
    "-Weverything"
    "-Wno-c++98-compat"
    "-Wno-documentation"
    "-Wno-documentation-html"
    "-Wno-documentation-unknown-command"
    "-Wno-exit-time-destructors"
    "-Wno-global-constructors"
    "-Wno-padded"
    "-Wno-switch-enum"
    "-Wno-covered-switch-default"
    "-Wno-weak-vtables")
endfunction()

function(enable_warnings)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    enable_warnings_clang(${ARGN})
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    enable_warnings_gnu(${ARGN})
  endif()
endfunction()

