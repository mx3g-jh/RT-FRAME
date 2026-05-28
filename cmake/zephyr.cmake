# Resolve ZEPHYR_BASE
# Priority: cmake var -> env var -> middlewares/zephyr inside this repo

if(NOT DEFINED ZEPHYR_BASE AND DEFINED ENV{ZEPHYR_BASE})
  set(ZEPHYR_BASE $ENV{ZEPHYR_BASE})
endif()

if(NOT DEFINED ZEPHYR_BASE)
  get_filename_component(_repo_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
  set(_local_zephyr "${_repo_root}/middlewares/zephyr")
  if(EXISTS "${_local_zephyr}/CMakeLists.txt")
    set(ZEPHYR_BASE "${_local_zephyr}")
    message(STATUS "Using bundled Zephyr: ${ZEPHYR_BASE}")
  else()
    message(FATAL_ERROR
      "ZEPHYR_BASE not set and middlewares/zephyr not present.\n"
      "Either set the environment variable ZEPHYR_BASE or place zephyr under middlewares/zephyr/."
    )
  endif()
endif()

set(ZEPHYR_BASE "${ZEPHYR_BASE}" CACHE PATH "Zephyr base directory")

get_filename_component(_repo_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# Tell Zephyr to search our boards/ before its own
list(APPEND BOARD_ROOT "${_repo_root}")
set(BOARD_ROOT "${BOARD_ROOT}" CACHE STRING "" FORCE)

# Inject rtframe as a Zephyr module (provides dts_root)
list(APPEND ZEPHYR_EXTRA_MODULES "${_repo_root}")

if(DEFINED ENV{ZEPHYR_MODULES})
  string(REPLACE ":" ";" _env_modules "$ENV{ZEPHYR_MODULES}")
  list(APPEND ZEPHYR_EXTRA_MODULES ${_env_modules})
endif()

if(DEFINED RTFRAME_EXTRA_MODULES)
  list(APPEND ZEPHYR_EXTRA_MODULES ${RTFRAME_EXTRA_MODULES})
endif()

list(REMOVE_DUPLICATES ZEPHYR_EXTRA_MODULES)
set(ZEPHYR_EXTRA_MODULES "${ZEPHYR_EXTRA_MODULES}" CACHE STRING "" FORCE)

find_package(Zephyr REQUIRED HINTS "${ZEPHYR_BASE}")
