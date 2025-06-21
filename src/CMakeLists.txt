add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_org.kde.bazaarrunner\")

kcoreaddons_add_plugin(bazaarrunner INSTALL_NAMESPACE "kf6/krunner")

target_sources(bazaarrunner PRIVATE
    bazaarrunner.cpp
)

target_link_libraries(bazaarrunner
    Qt6::Gui # QAction
    KF6::Runner
    KF6::I18n
)
