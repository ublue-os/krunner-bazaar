add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_org.kde.bazaarrunner\")

# Shared Bazaar client library
add_library(bazaarclient STATIC
    bazaarclient.cpp
    bazaarclient.h
)

# Enable position-independent code for static library to allow linking into shared objects
set_target_properties(bazaarclient PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(bazaarclient
    Qt6::Core
    Qt6::DBus
)

# KRunner plugin
kcoreaddons_add_plugin(bazaarrunner INSTALL_NAMESPACE "kf6/krunner")

target_sources(bazaarrunner PRIVATE
    bazaarrunner.cpp
)

target_link_libraries(bazaarrunner
    Qt6::Core
    Qt6::Gui
    Qt6::DBus
    KF6::Runner
    KF6::I18n
    bazaarclient
)

# Standalone CLI test tool
add_executable(bazaar-test
    bazaar-test.cpp
)

target_link_libraries(bazaar-test
    Qt6::Core
    Qt6::DBus
    bazaarclient
)

# Install the CLI tool
install(TARGETS bazaar-test DESTINATION ${CMAKE_INSTALL_BINDIR})
