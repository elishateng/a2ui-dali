CMAKE_MINIMUM_REQUIRED(VERSION 3.8.2)

PROJECT(a2ui-dali
  VERSION 0.1.0
  DESCRIPTION "A DALi-based renderer for the A2UI protocol"
  LANGUAGES CXX C
)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_C_STANDARD 99)

# ============================================================
# Dependencies
# ============================================================
FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(DALICORE            REQUIRED dali2-core)
PKG_CHECK_MODULES(DALIADAPTOR         REQUIRED dali2-adaptor)
PKG_CHECK_MODULES(DALI_UI_FOUNDATION  REQUIRED dali2-ui-foundation)
PKG_CHECK_MODULES(DALI_UI_COMPONENTS  REQUIRED dali2-ui-components)
PKG_CHECK_MODULES(CURL                REQUIRED libcurl)

# ============================================================
# Source groups
# ============================================================
# Core: A2UI v0.9 message parsing, data model, expression evaluation,
# action dispatch — no DALi UI dependency, used by both the renderer
# and the conformance test.
SET(A2UI_CORE_SOURCES
  src/core/a2ui-message-processor.cpp
  src/core/a2ui-payload-fixer.cpp
  src/core/a2ui-sanitizer.cpp
  src/core/a2ui-stream-parser.cpp
  src/core/action-dispatcher.cpp
  src/core/data-context.cpp
  src/core/data-model.cpp
  src/core/diff-engine.cpp
  src/core/expression-parser.cpp
  src/core/surface-components-model.cpp
  src/core/surface-group-model.cpp
  src/core/surface-model.cpp
  src/core/template-cache.cpp
)

# Renderer + transport: DALi-side bindings.
SET(A2UI_LIB_SOURCES
  ${A2UI_CORE_SOURCES}
  src/renderer/a11y-bridge.cpp
  src/renderer/a2ui-renderer.cpp
  src/renderer/view-pool.cpp
  src/transport/a2a-transport.cpp
)

# ============================================================
# Library: a2ui-dali (static)
# ============================================================
ADD_LIBRARY(a2ui-dali STATIC ${A2UI_LIB_SOURCES})

TARGET_INCLUDE_DIRECTORIES(a2ui-dali PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${DALICORE_INCLUDE_DIRS}
  ${DALIADAPTOR_INCLUDE_DIRS}
  ${DALI_UI_FOUNDATION_INCLUDE_DIRS}
  ${DALI_UI_COMPONENTS_INCLUDE_DIRS}
  ${CURL_INCLUDE_DIRS}
)

TARGET_COMPILE_OPTIONS(a2ui-dali PRIVATE
  ${DALICORE_CFLAGS_OTHER}
  ${DALIADAPTOR_CFLAGS_OTHER}
  ${DALI_UI_FOUNDATION_CFLAGS_OTHER}
  ${DALI_UI_COMPONENTS_CFLAGS_OTHER}
)

TARGET_LINK_LIBRARIES(a2ui-dali PUBLIC
  ${DALICORE_LIBRARIES}
  ${DALIADAPTOR_LIBRARIES}
  ${DALI_UI_FOUNDATION_LIBRARIES}
  ${DALI_UI_COMPONENTS_LIBRARIES}
  ${CURL_LIBRARIES}
  pthread
)

TARGET_LINK_DIRECTORIES(a2ui-dali PUBLIC
  $ENV{DESKTOP_PREFIX}/lib
)

# ============================================================
# Conformance test: core parsing only, no DALi UI deps
# ============================================================
SET(TEST_NAME a2ui-conformance-test)
ADD_EXECUTABLE(${TEST_NAME}
  test/conformance-test.cpp
  ${A2UI_CORE_SOURCES}
)

TARGET_INCLUDE_DIRECTORIES(${TEST_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${DALICORE_INCLUDE_DIRS}
  ${DALIADAPTOR_INCLUDE_DIRS}
  ${DALI_UI_FOUNDATION_INCLUDE_DIRS}
  ${CURL_INCLUDE_DIRS}
)

TARGET_LINK_LIBRARIES(${TEST_NAME}
  ${DALICORE_LIBRARIES}
  ${DALIADAPTOR_LIBRARIES}
  ${DALI_UI_FOUNDATION_LIBRARIES}
  ${CURL_LIBRARIES}
  pthread
)

TARGET_LINK_DIRECTORIES(${TEST_NAME} PRIVATE
  $ENV{DESKTOP_PREFIX}/lib
)

SET_TARGET_PROPERTIES(${TEST_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

# ============================================================
# Example: examples/basic-renderer
# ============================================================
SET(EXAMPLE_NAME a2ui-basic-renderer)
ADD_EXECUTABLE(${EXAMPLE_NAME} examples/basic-renderer/main.cpp)

TARGET_LINK_LIBRARIES(${EXAMPLE_NAME} a2ui-dali)

SET_TARGET_PROPERTIES(${EXAMPLE_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)
