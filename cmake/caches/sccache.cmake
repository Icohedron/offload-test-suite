if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
endif()

find_program(SCCACHE sccache HINTS /usr/local/bin)
if (SCCACHE)
  set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE} CACHE STRING "")
  set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE} CACHE STRING "")
endif()
