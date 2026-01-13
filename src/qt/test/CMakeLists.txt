# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include(DiscoverTests)

set(CMAKE_AUTOMOC_MOC_OPTIONS "-p${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(test_bitcoin-qt
  apptests.cpp
  optiontests.cpp
  rpcnestedtests.cpp
  test_main.cpp
  uritests.cpp
  util.cpp
  ../../init/bitcoin-qt.cpp
)

add_windows_application_manifest(test_bitcoin-qt)

target_link_libraries(test_bitcoin-qt
  core_interface
  bitcoinqt
  test_util
  bitcoin_node
  Boost::headers
  Qt6::Test
)

import_plugins(test_bitcoin-qt)

if(ENABLE_WALLET)
  target_sources(test_bitcoin-qt
    PRIVATE
      addressbooktests.cpp
      wallettests.cpp
      ../../wallet/test/wallet_test_fixture.cpp
  )
endif()

set(QT_TEST_PROPERTIES
  LABELS "qt.test"
  SKIP_REGULAR_EXPRESSION "Test is skipped"
)

if(WIN32 AND VCPKG_TARGET_TRIPLET)
  set(plugin_path "$<SHELL_PATH:$<PATH:GET_PARENT_PATH,$<PATH:GET_PARENT_PATH,$<TARGET_PROPERTY:Qt6::QWindowsIntegrationPlugin,LOCATION_$<CONFIG>>>>>")
  list(APPEND QT_TEST_PROPERTIES
    ENVIRONMENT_MODIFICATION
      # On Windows, vcpkg configures Qt with `-opengl dynamic`, which makes
      # the "minimal" platform plugin unusable due to internal Qt bugs.
      QT_QPA_PLATFORM=set:windows
      QT_PLUGIN_PATH=set:${plugin_path}
  )
  unset(plugin_path)
endif()

discover_tests(test_bitcoin-qt
  DISCOVERY_ARGS        [[-functions]]
  DISCOVERY_MATCH       [[(.+)]]
  TEST_NAME_REPLACEMENT [[bitcoin.qt.test.\1]]
  TEST_ARGS_REPLACEMENT [[\1]]
  PROPERTIES  ${QT_TEST_PROPERTIES}
)

install_binary_component(test_bitcoin-qt INTERNAL)
