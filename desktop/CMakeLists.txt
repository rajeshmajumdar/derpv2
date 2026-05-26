cmake_minimum_required(VERSION 3.20)
project(dERP)

set(CMAKE_PREFIX_PATH "C:/Qt/6.11.0/mingw_64")

set (CMAKE_AUTOMOC on)
set (CMAKE_AUTOUIC on)
set (CMAKE_AUTORCC on)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Network)

# ----------------------------
# KERNEL EXECUTABLE
# ----------------------------
set (KERNEL_DIR src/kernel)

add_executable(kernel
  ${KERNEL_DIR}/main.cpp
  ${KERNEL_DIR}/Kernel.cpp
  ${KERNEL_DIR}/Kernel.h

  # Managers
  ${KERNEL_DIR}/managers/PluginManager.cpp
  ${KERNEL_DIR}/managers/PluginManager.h
  ${KERNEL_DIR}/managers/NetworkManager.cpp
  ${KERNEL_DIR}/managers/NetworkManager.h
  ${KERNEL_DIR}/managers/InputManager.cpp
  ${KERNEL_DIR}/managers/InputManager.h

  # Controllers
  ${KERNEL_DIR}/controllers/ServerManager.cpp
  ${KERNEL_DIR}/controllers/ServerManager.h

  # UI
  ${KERNEL_DIR}/ui/LoginDialog.cpp
  ${KERNEL_DIR}/ui/LoginDialog.h
  ${KERNEL_DIR}/ui/SetupWizard.cpp
  ${KERNEL_DIR}/ui/SetupWizard.h
  ${KERNEL_DIR}/ui/UIManager.cpp
  ${KERNEL_DIR}/ui/UIManager.h
  ${KERNEL_DIR}/ui/CommandPalette.cpp
  ${KERNEL_DIR}/ui/CommandPalette.h

  resources.qrc
)

target_link_libraries(kernel PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Network)

target_include_directories(kernel PRIVATE
  src/interfaces
  ${KERNEL_DIR}
  ${KERNEL_DIR}/managers
  ${KERNEL_DIR}/ui
  ${KERNEL_DIR}/controllers
)

# --------------------------
# Plugins
# --------------------------
set(DUMMY_PLUGIN_DIR src/plugins/dummy_plugin)

add_library(dummy_plugin MODULE
  ${DUMMY_PLUGIN_DIR}/DummyPlugin.h
  ${DUMMY_PLUGIN_DIR}/DummyPlugin.cpp
  src/interfaces/DBaseModule.h
)

target_link_libraries(dummy_plugin PRIVATE Qt6::Widgets Qt6::Core)

target_include_directories(dummy_plugin PRIVATE
  src/interfaces
)



set(DASHBOARD_PLUGIN_DIR src/plugins/dashboard_plugin)

add_library(dashboard_plugin MODULE
  ${DASHBOARD_PLUGIN_DIR}/DashboardPlugin.h
  ${DASHBOARD_PLUGIN_DIR}/DashboardPlugin.cpp
  src/interfaces/DBaseModule.h
)

target_link_libraries(dashboard_plugin PRIVATE Qt6::Widgets Qt6::Core)

target_include_directories(dashboard_plugin PRIVATE
  src/interfaces
)



set(STAFF_MANAGER_DIR src/plugins/staff_manager)

add_library(staff_manager MODULE
  ${STAFF_MANAGER_DIR}/StaffManager.h
  ${STAFF_MANAGER_DIR}/StaffManager.cpp
  src/interfaces/DBaseModule.h
)

target_link_libraries(staff_manager PRIVATE Qt6::Widgets Qt6::Core)

target_include_directories(staff_manager PRIVATE
  src/interfaces
)

# Deploy server.exe alongside kernel.exe
add_custom_command(TARGET kernel POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/server.exe"
    "${CMAKE_BINARY_DIR}/server.exe"
)

# ------------------------
# POST-BUILD
# ------------------------
set_target_properties(dummy_plugin PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/com.derp.dummy"
)
add_custom_command(TARGET dummy_plugin POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/${DUMMY_PLUGIN_DIR}/manifest.json"
    "${CMAKE_BINARY_DIR}/plugins/com.derp.dummy/manifest.json"
)

set_target_properties(dashboard_plugin PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/com.derp.dashboard"
)
add_custom_command(TARGET dashboard_plugin POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/${DASHBOARD_PLUGIN_DIR}/manifest.json"
    "${CMAKE_BINARY_DIR}/plugins/com.derp.dashboard/manifest.json"
)

set_target_properties(staff_manager PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/com.derp.staff_manager"
)
add_custom_command(TARGET staff_manager POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/${STAFF_MANAGER_DIR}/manifest.json"
    "${CMAKE_BINARY_DIR}/plugins/com.derp.staff_manager/manifest.json"
)
