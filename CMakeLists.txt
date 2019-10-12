cmake_minimum_required(VERSION 3.0)

project (pngdiff)

set(QT_MIN_VERSION "5.3.0")
set(KF5_MIN_VERSION "5.2.0")

set(CMAKE_CXX_FLAGS "-O2 -Wall -Wextra")

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

# Find Qt modules
find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core    # QCommandLineParser, QStringLiteral
    Widgets # QApplication
)

# Find KDE modules
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    KIO
    CoreAddons      # KAboutData
    I18n            # KLocalizedString
    WidgetsAddons   # KMessageBox
)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(pngdiff_SRCS pngdiff.cpp)

add_library(pngdiff ${pngdiff_SRCS})
kde_target_enable_exceptions(pngdiff PRIVATE)

set(crc_utils_SRCS crc_utils.cpp)
add_library(crc_utils STATIC ${crc_utils_SRCS})
set(png_SRCS png.cpp)
add_library(png STATIC ${png_SRCS})

target_link_libraries(crc_utils
    KF5::KIOWidgets
)

target_link_libraries(png
    KF5::KIOWidgets
    crc_utils
)

target_link_libraries(pngdiff
    KF5::KIOWidgets
    png
)

set(pngdiff_thumbcreator_SRCS pngdiff_thumbcreator.cpp)
add_library(pngdiff_thumbcreator MODULE ${pngdiff_thumbcreator_SRCS})
target_link_libraries(pngdiff_thumbcreator PRIVATE KF5::KIOWidgets)

install(TARGETS pngdiff DESTINATION ${PLUGIN_INSTALL_DIR}/imageformats)
install(TARGETS pngdiff_thumbcreator DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES pngdiffthumbnail.desktop DESTINATION ${KDE_INSTALL_FULL_KSERVICES5DIR})
install(FILES image-x-pngdiff.xml DESTINATION ${KDE_INSTALL_FULL_MIMEDIR})

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

#[[get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
kde_target_enable_exceptions
endforeach()]]

install(CODE "execute_process(COMMAND update-desktop-database)")

# There must be a better way but I searched and couldn't find anything
install(CODE "execute_process(COMMAND update-mime-database /usr/share/mime)")


install(CODE "execute_process(COMMAND pypy3 -m pip install ../pngdiff_utils)")
