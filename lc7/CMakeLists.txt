CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(lc7)

INCLUDE(../cmake/PCH.cmake)
INCLUDE(../cmake/FindQt5.cmake)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5Network REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Core REQUIRED)

SET(lc7_SOURCES
	src/stdafx.cpp
	src/CLC7ColorManager.cpp
	src/CLC7GUILinkage.cpp
	src/CLC7MenuWidget.cpp
	src/CLC7NavBar.cpp
	src/CLC7NavBarGroup.cpp
	src/CLC7NavBarHeader.cpp
	src/CLC7NavBarItem.cpp
	src/CLC7NavBarWidget.cpp
	src/CLC7ProgressBox.cpp
	src/CLC7StartupWidget.cpp
	src/CAnimatedBarChart.cpp
	src/CLC7WorkQueueWidget.cpp
	src/CLC7PresetWidget.cpp
	src/CLC7Updater.cpp
	src/CLC7UpdateDialog.cpp
	src/CLC7FileDownloader.cpp
	src/CLC7ModifiedListWidget.cpp
	src/CSystemOptions.cpp
	src/CSystemPage.cpp
	src/LC7Main.cpp
	src/main.cpp
	src/trdebug.cpp
	src/lc7app.cpp
	src/Attach.cpp
	src/CAttachServer.cpp
	src/CAttachClient.cpp
	src/CIOProcessor.cpp
	src/CLC7ExitWarningDlg.cpp
	src/CLC7AutosaveDlg.cpp
	src/CLC7LoginDlg.cpp
)
IF(MSVC)
	LIST(APPEND lc7_SOURCES resources/lc7.rc)
ENDIF()

SET(lc7_FORMS 
	resources/LC7Main.ui
	resources/LC7MenuWidget.ui
	resources/LC7ProgressDialog.ui
	resources/LC7Startup.ui
	resources/LC7WorkQueueWidget.ui
	resources/LC7ExitWarningDlg.ui
	resources/LC7UpdateDialog.ui
	resources/system.ui
	resources/presetwidget.ui
	resources/presetbuttons.ui
	resources/LC7AutosaveDlg.ui
	resources/LC7LoginDlg.ui
)

SET(lc7_RESOURCES
	resources/darkstyle.qrc
	resources/lc7.qrc
	resources/qtpropertymanager.qrc
)

SET(lc7_HEADERS
	include/stdafx.h
	include/appversion.h
	include/CLC7ColorManager.h
	include/CLC7GUILinkage.h
	include/CLC7MenuWidget.h
	include/CLC7NavBar.h
	include/CLC7NavBarGroup.h
	include/CLC7NavBarHeader.h
	include/CLC7NavBarItem.h
	include/CLC7NavBarWidget.h
	include/CLC7ProgressBox.h
	include/CLC7StartupWidget.h
	include/CAnimatedBarChart.h
	include/CLC7WorkQueueWidget.h
	include/CLC7PresetWidget.h
	include/CLC7Updater.h
	include/CLC7UpdateDialog.h
	include/CLC7FileDownloader.h
	include/CLC7ModifiedListWidget.h
	include/ILC7GUIComponent.h
	include/CSystemOptions.h
	include/CSystemPage.h
	include/LC7Main.h
	include/lc7app.h
	include/Attach.h
	include/CIOProcessor.h
	include/CAttachServer.h
	include/CAttachClient.h
	include/CLC7ExitWarningDlg.h
	include/CLC7AutosaveDlg.h
	include/CLC7LoginDlg.h
)

#MESSAGE(lc7_SOURCES=${lc7_SOURCES})
#MESSAGE(lc7_FORMS=${lc7_FORMS})
#MESSAGE(lc7_HEADERS=${lc7_HEADERS})


IF(MSVC)
	SET(CRASHREPORTERLIB CrashRpt)
	SET(CRASHREPORTERINCLUDE  "../external/crashrpt/include")
ELSE()
	SET(CRASHREPORTERLIB "")
	SET(CRASHREPORTERINCLUDE "")
ENDIF()


INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} "include" "../api/include" "../external/qtsingleapplication/src" "../external/qtpropertybrowser/src" ${CRASHREPORTERINCLUDE})
LINK_DIRECTORIES(${CRASHREPORTERLIBDIR})

QT5_WRAP_UI(lc7_UI_HEADERS
    ${lc7_FORMS}
)

QT5_ADD_RESOURCES(ResourceSources ${lc7_RESOURCES})
ADD_DEFINITIONS(-DDISABLE_UPDATER)	# Remove this when updates are working again
IF(MSVC)
	ADD_EXECUTABLE(lc7 WIN32 ${lc7_SOURCES} ${lc7_HEADERS} ${lc7_UI_HEADERS} ${lc7_RESOURCES} ${ResourceSources})
ELSEIF(APPLE)
	ADD_EXECUTABLE(lc7 MACOSX_BUNDLE ${lc7_SOURCES} ${lc7_HEADERS} ${lc7_UI_HEADERS} ${lc7_RESOURCES} ${ResourceSources})
ELSE()
	ADD_EXECUTABLE(lc7 ${lc7_SOURCES} ${lc7_HEADERS} ${lc7_UI_HEADERS} ${lc7_RESOURCES} ${ResourceSources})
ENDIF()
FIND_PACKAGE(ZLIB REQUIRED)
TARGET_LINK_LIBRARIES(lc7 qtsingleapplication qtpropertybrowser qtpropertybrowser ${CRASHREPORTERLIB} ZLIB::ZLIB)

qt5_use_modules(lc7 Widgets Network)

IF(APPLE)
    SET_TARGET_PROPERTIES(lc7 PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.l0phtcrack.lc7"
        MACOSX_BUNDLE_BUNDLE_NAME "L0phtCrack 7"
        MACOSX_BUNDLE_ICON_FILE "lc7"
        MACOSX_BUNDLE_BUNDLE_VERSION "7.3.3"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "7.3.3"
        MACOSX_BUNDLE_LONG_VERSION_STRING "7.3.3 macOS community fork"
        MACOSX_BUNDLE_COPYRIGHT "L0pht Holdings, LLC. Unofficial community builds maintain separate changelogs."
        MACOSX_RPATH TRUE
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "@loader_path"
    )
    ADD_CUSTOM_COMMAND(TARGET lc7 POST_BUILD
        COMMAND install_name_tool
            -change "/opt/homebrew/opt/qt@5/lib/QtWidgets.framework/Versions/5/QtWidgets"
                    "@executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets"
            -change "/opt/homebrew/opt/qt@5/lib/QtGui.framework/Versions/5/QtGui"
                    "@executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui"
            -change "/opt/homebrew/opt/qt@5/lib/QtNetwork.framework/Versions/5/QtNetwork"
                    "@executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork"
            -change "/opt/homebrew/opt/qt@5/lib/QtCore.framework/Versions/5/QtCore"
                    "@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore"
            "$<TARGET_FILE:lc7>"
        COMMENT "Fixing Qt framework paths in lc7 binary"
    )
    ADD_CUSTOM_COMMAND(TARGET lc7 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/lc7/resources/lc7.icns"
            "$<TARGET_BUNDLE_CONTENT_DIR:lc7>/Resources/lc7.icns"
        COMMENT "Bundle application icon (regenerate with tools/macos/mk-icns.sh if art changes)"
    )
    ADD_CUSTOM_COMMAND(TARGET lc7 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:lc7>/MacOS/common"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources/wordlists"
            "$<TARGET_BUNDLE_CONTENT_DIR:lc7>/MacOS/common/wordlists"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources/rules"
            "$<TARGET_BUNDLE_CONTENT_DIR:lc7>/MacOS/common/rules"
        COMMENT "Copying bundled wordlists and rules into app bundle (from resources/)"
    )
    # QLibrary loads liblc7core.dylib from the executable directory; helper dylibs must sit next to lc7.
    ADD_CUSTOM_COMMAND(TARGET lc7 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:lc7core>"
            "$<TARGET_FILE_DIR:lc7>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:qtsingleapplication>"
            "$<TARGET_FILE_DIR:lc7>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:qtpropertybrowser>"
            "$<TARGET_FILE_DIR:lc7>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:quazip>"
            "$<TARGET_FILE_DIR:lc7>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:qt5keychain>"
            "$<TARGET_FILE_DIR:lc7>/"
        COMMAND /bin/cp
            -a
            "$<TARGET_FILE_DIR:lc7>/../../../libqt5keychain.0.dylib"
            "$<TARGET_FILE_DIR:lc7>/"
        COMMAND /bin/cp
            -a
            "$<TARGET_FILE_DIR:lc7>/../../../libqt5keychain.dylib"
            "$<TARGET_FILE_DIR:lc7>/"
        COMMENT "Copy lc7core and dependent dylibs into app bundle MacOS"
    )
    ADD_CUSTOM_COMMAND(TARGET lc7 POST_BUILD
        COMMAND install_name_tool
            -change "/opt/homebrew/opt/qt@5/lib/QtWidgets.framework/Versions/5/QtWidgets"
                    "@executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets"
            -change "/opt/homebrew/opt/qt@5/lib/QtSql.framework/Versions/5/QtSql"
                    "@executable_path/../Frameworks/QtSql.framework/Versions/5/QtSql"
            -change "/opt/homebrew/opt/qt@5/lib/QtGui.framework/Versions/5/QtGui"
                    "@executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui"
            -change "/opt/homebrew/opt/qt@5/lib/QtCore.framework/Versions/5/QtCore"
                    "@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore"
            "$<TARGET_FILE_DIR:lc7>/liblc7core.dylib"
        COMMAND install_name_tool
            -change "/opt/homebrew/opt/qt@5/lib/QtWidgets.framework/Versions/5/QtWidgets"
                    "@executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets"
            -change "/opt/homebrew/opt/qt@5/lib/QtGui.framework/Versions/5/QtGui"
                    "@executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui"
            -change "/opt/homebrew/opt/qt@5/lib/QtCore.framework/Versions/5/QtCore"
                    "@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore"
            "$<TARGET_FILE_DIR:lc7>/libquazip.dylib"
        COMMAND install_name_tool
            -change "/opt/homebrew/opt/qt@5/lib/QtCore.framework/Versions/5/QtCore"
                    "@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore"
            "$<TARGET_FILE_DIR:lc7>/$<TARGET_FILE_NAME:qt5keychain>"
        COMMENT "Point bundled lc7core, quazip, qt5keychain at embedded Qt frameworks"
    )
    # Sync lc7password into bundle BEFORE macdeployqt so it can patch its Qt framework paths.
    SET(_LC7PASSWORD_PLUGDIR "lcplugins/lc7password{072cbbd8-84ad-481f-b0a6-398b121434db}")
    ADD_CUSTOM_COMMAND(TARGET lc7 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:lc7>/${_LC7PASSWORD_PLUGDIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/dist/${_LC7PASSWORD_PLUGDIR}/liblc7password.dylib"
            "$<TARGET_FILE_DIR:lc7>/${_LC7PASSWORD_PLUGDIR}/liblc7password.dylib"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/dist/${_LC7PASSWORD_PLUGDIR}/manifest.json"
            "$<TARGET_FILE_DIR:lc7>/${_LC7PASSWORD_PLUGDIR}/manifest.json"
        COMMENT "Sync lc7password plugin into bundle (before macdeployqt patches Qt paths)"
    )

    FIND_PROGRAM(
        LC7_MACDEPLOYQT_EXECUTABLE
        NAMES macdeployqt
        HINTS
            "${Qt5_DIR}/../../../bin"
            /opt/homebrew/opt/qt@5/bin
            /usr/local/opt/qt@5/bin
        DOC "Qt mac deployment tool (relays PlugIns to bundled Qt, prevents duplicate Homebrew Qt)"
    )
    IF(LC7_MACDEPLOYQT_EXECUTABLE)
        ADD_CUSTOM_COMMAND(TARGET lc7 POST_BUILD
            COMMAND ${LC7_MACDEPLOYQT_EXECUTABLE}
                "$<TARGET_BUNDLE_DIR:lc7>"
                -libpath=${CMAKE_BINARY_DIR}/dist
                -always-overwrite
            COMMENT "macdeployqt: bundle Qt plugins against embedded Frameworks"
        )
    ENDIF()
    # macdeployqt copies only dylibs into the bundle; manifest.json must sit next to each plugin.
    ADD_CUSTOM_COMMAND(TARGET lc7 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/dist/lcplugins/lc7importwin{17324176-3fa7-4c1a-9204-3f391b6b3599}/manifest.json"
            "$<TARGET_FILE_DIR:lc7>/lcplugins/lc7importwin{17324176-3fa7-4c1a-9204-3f391b6b3599}/manifest.json"
        COMMENT "lc7importwin manifest into bundle (macdeployqt does not copy JSON)"
    )
    # macdeployqt may replace or thin plugin folders; re-sync lc7jtr from dist so the .app always runs.
    SET(_LC7JTR_PLUGDIR "lcplugins/lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}")
    ADD_CUSTOM_COMMAND(TARGET lc7 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:lc7>/${_LC7JTR_PLUGDIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/dist/lcplugins/lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}/liblc7jtr.dylib"
            "$<TARGET_FILE_DIR:lc7>/${_LC7JTR_PLUGDIR}/liblc7jtr.dylib"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/dist/lcplugins/lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}/manifest.json"
            "$<TARGET_FILE_DIR:lc7>/${_LC7JTR_PLUGDIR}/manifest.json"
        COMMENT "Sync lc7jtr plugin into bundle (after macdeployqt)"
    )
    IF(TARGET jtrdll_apple_silicon)
        ADD_DEPENDENCIES(lc7 jtrdll_apple_silicon)
        ADD_CUSTOM_COMMAND(TARGET lc7 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:jtrdll_apple_silicon>"
                "$<TARGET_FILE_DIR:lc7>/${_LC7JTR_PLUGDIR}/jtrdll_apple-silicon.dylib"
            COMMENT "Sync John engine dylib into bundle (after macdeployqt)"
        )
    ENDIF()
    ADD_CUSTOM_COMMAND(TARGET lc7 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/dist/${_LC7JTR_PLUGDIR}/john_run"
            "$<TARGET_FILE_DIR:lc7>/${_LC7JTR_PLUGDIR}/john_run"
        COMMENT "Sync John Jumbo john_run (john.conf) into app bundle"
    )
    ADD_DEPENDENCIES(lc7 lc7jtr)
    ADD_DEPENDENCIES(lc7 lc7password)
    FIND_PROGRAM(LC7_CODESIGN_EXECUTABLE codesign)
    IF(LC7_CODESIGN_EXECUTABLE)
        ADD_CUSTOM_COMMAND(TARGET lc7 POST_BUILD
            COMMAND ${LC7_CODESIGN_EXECUTABLE} --force --deep -s -
                "$<TARGET_BUNDLE_DIR:lc7>"
            COMMENT "Ad-hoc codesign: Gatekeeper and plugin dylibs load reliably from Finder"
        )
    ENDIF()
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Linux: simple executable, no bundle, use RPATH for plugin loading.
    # NOTE: cmake's LIBRARY_OUTPUT_DIRECTORY (set in root CMakeLists.txt via
    # SET_LC7_TARGET_PROPERTIES_CONFIG) already places each plugin .so directly
    # into dist/lcplugins/<guid>/ — the same location $<TARGET_FILE_DIR:lc7>
    # resolves to (dist/).  Copying .so files to themselves would fail; instead
    # we only need to copy the manifest.json side-files from the source tree.
    SET_TARGET_PROPERTIES(lc7 PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
    # Plugin directory GUIDs (relative to dist/)
    SET(_LC7PASSWORD_PLUGDIR "lcplugins/lc7password{072cbbd8-84ad-481f-b0a6-398b121434db}")
    SET(_LC7JTR_PLUGDIR      "lcplugins/lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}")
    # Source-tree manifests (committed under dist/win64/common/...)
    SET(_MANIFEST_SRC "${CMAKE_SOURCE_DIR}/dist/win64/common/lcplugins")
    # Copy manifests into build dist (plugins are already there via OUTPUT_DIRECTORY)
    ADD_CUSTOM_COMMAND(TARGET lc7 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_BINARY_DIR}/dist/${_LC7PASSWORD_PLUGDIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_MANIFEST_SRC}/lc7password{072cbbd8-84ad-481f-b0a6-398b121434db}/manifest.json"
            "${CMAKE_BINARY_DIR}/dist/${_LC7PASSWORD_PLUGDIR}/manifest.json"
        COMMENT "Copy lc7password manifest.json (Linux)"
    )
    ADD_CUSTOM_COMMAND(TARGET lc7 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_BINARY_DIR}/dist/${_LC7JTR_PLUGDIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_MANIFEST_SRC}/lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}/manifest.json"
            "${CMAKE_BINARY_DIR}/dist/${_LC7JTR_PLUGDIR}/manifest.json"
        COMMENT "Copy lc7jtr manifest.json (Linux)"
    )
    # Copy manifests for the remaining plugins (lc7base, lc7importunix, lc7wizard)
    FOREACH(_PLUG
        "lc7base{2f63a714-8518-4ab6-ba7d-5440888dfc8a}"
        "lc7importunix{62a9e7aa-58f4-45ba-8fd7-5ebeb3d80551}"
        "lc7wizard{43b6f871-7951-4b81-814b-f41ef64c993b}"
    )
        ADD_CUSTOM_COMMAND(TARGET lc7 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "${CMAKE_BINARY_DIR}/dist/lcplugins/${_PLUG}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_MANIFEST_SRC}/${_PLUG}/manifest.json"
                "${CMAKE_BINARY_DIR}/dist/lcplugins/${_PLUG}/manifest.json"
            COMMENT "Copy ${_PLUG} manifest.json (Linux)"
        )
    ENDFOREACH()
    # lc7importwin is now built as a real .so on Linux — manifest.json is copied by lc7importwin POST_BUILD
    # and the .so is placed by SET_TARGET_PROPERTIES in root CMakeLists.txt
    ADD_DEPENDENCIES(lc7 lc7importwin)
    # hashcat shim is copied under the correct name (jtrdll_apple-silicon.so / jtrdll_sse2.so etc.)
    # by POST_BUILD in external/jtrdll/jtrdll/CMakeLists.txt
    IF(TARGET jtrdll_linux_hashcat)
        ADD_DEPENDENCIES(lc7 jtrdll_linux_hashcat)
    ENDIF()
    ADD_DEPENDENCIES(lc7 lc7jtr)
    ADD_DEPENDENCIES(lc7 lc7password)
    # Copy bundled wordlists and rules into dist/common/ so the app can find them at runtime.
    # The plugin (lc7hashcat/lc7jtr) searches: startup/common/wordlists and startup/common/rules.
    ADD_CUSTOM_COMMAND(TARGET lc7 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/dist/common/wordlists"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/dist/common/rules"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources/wordlists"
            "${CMAKE_BINARY_DIR}/dist/common/wordlists"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources/rules"
            "${CMAKE_BINARY_DIR}/dist/common/rules"
        COMMENT "Copying bundled wordlists and rules into dist/common/ (Linux)"
    )

    # -----------------------------------------------------------------------
    # Generate lc7.sh launcher script.
    # Use ADD_CUSTOM_COMMAND with OUTPUT + DEPENDS so the file is regenerated
    # automatically whenever lc7.sh.in changes — even without a full rebuild.
    # (CONFIGURE_FILE-only approach failed because it runs at configure time
    # and POST_BUILD only fires when the lc7 binary itself is relinked.)
    # -----------------------------------------------------------------------
    ADD_CUSTOM_COMMAND(
        OUTPUT "${CMAKE_BINARY_DIR}/dist/lc7.sh"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/lc7.sh.in"
            "${CMAKE_BINARY_DIR}/dist/lc7.sh"
        COMMAND chmod 755 "${CMAKE_BINARY_DIR}/dist/lc7.sh"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/lc7.sh.in"
        COMMENT "Generate lc7.sh launcher with execute permissions (Linux)"
    )
    ADD_CUSTOM_TARGET(lc7_sh ALL
        DEPENDS "${CMAKE_BINARY_DIR}/dist/lc7.sh"
    )
    ADD_DEPENDENCIES(lc7 lc7_sh)

    # -----------------------------------------------------------------------
    # Bundle Qt5, ICU, xcb, and Qt platform plugins into dist/lib/ and dist/platforms/.
    # This makes the distribution self-contained: no system Qt required on target machine.
    # -----------------------------------------------------------------------
    ADD_CUSTOM_COMMAND(TARGET lc7 POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DLC7_BINARY="$<TARGET_FILE:lc7>"
            -DDIST_DIR="${CMAKE_BINARY_DIR}/dist"
            -P "${CMAKE_SOURCE_DIR}/cmake/BundleLinuxQt.cmake"
        COMMENT "Bundle Qt5/ICU/xcb into dist/lib and dist/platforms (Linux)"
    )
ENDIF()

ADD_PRECOMPILED_HEADER(lc7 stdafx lc7_SOURCES)

#IF(MSVC)
#	SET( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /level='requireAdministrator' /uiAccess='false' /MANIFEST /MANIFEST:EMBED" )
#ENDIF()
