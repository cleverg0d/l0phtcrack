CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(lc7jtr)

INCLUDE(../cmake/PCH.cmake)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Core REQUIRED)

SET(lc7jtr_SOURCES 
	src/stdafx.cpp
	src/CLC7CharsetEditor.cpp
	src/CLC7CharsetEditorDlg.cpp
	src/CLC7ExecuteJTR.cpp
	src/CLC7JTRConsole.cpp
	src/CLC7JTR.cpp
	src/CLC7JTRGPUManager.cpp
	src/CLC7JTRPlugin.cpp
	src/CSystemJTR.cpp
	src/CTechniqueJTR.cpp
	src/CTechniqueJTRSingleGUI.cpp
	src/CTechniqueJTRBruteGUI.cpp
	src/CTechniqueJTRDictionaryGUI.cpp
	src/CTechniqueJTRFinalyseGUI.cpp
	src/jtrnodeworker.cpp
	src/jtrworker.cpp
	src/linkage.cpp
	src/techniquebruteconfig.cpp
	src/techniquedictionaryconfig.cpp
	src/CLC7JTRDLL.cpp
	src/CLC7JTREXE.cpp
	src/CLC7JTRPasswordEngine.cpp
	src/turbo_linecount.cpp
)

SET(lc7jtr_FORMS 
	resources/techniquebruteconfig.ui
	resources/techniquedictionaryconfig.ui
	resources/charseteditor.ui
	resources/charseteditordlg.ui
	resources/jtrconsole.ui
)

SET(lc7jtr_RESOURCES
	resources/lc7jtr.qrc
)

SET(lc7jtr_HEADERS
	include/stdafx.h
	include/CLC7CharsetEditor.h
	include/CLC7CharsetEditorDlg.h
	include/CLC7ExecuteJTR.h
	include/CLC7JTRConsole.h
	include/CLC7JTR.h
	include/CLC7JTRGPUManager.h
	include/CLC7JTRPlugin.h
	include/CSystemJTR.h
	include/CTechniqueJTR.h
	include/CTechniqueJTRSingleGUI.h
	include/CTechniqueJTRBruteGUI.h
	include/CTechniqueJTRDictionaryGUI.h
	include/CTechniqueJTRFinalyseGUI.h
	include/jtrnodeworker.h
	include/jtrworker.h
	include/jtrexe_crashdump.h
	include/linkage.h
	include/techniquebruteconfig.h
	include/techniquedictionaryconfig.h
	include/uuids.h
	include/CLC7JTRDLL.h
	include/CLC7JTREXE.h
	include/CLC7JTRPasswordEngine.h
	include/turbo_linecount.h
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} "include" "../api/include" "../external/jtrdll/jtrdll")

QT5_WRAP_UI(lc7jtr_UI_HEADERS
    ${lc7jtr_FORMS}
)

IF(MSVC)
	ADD_DEFINITIONS( -D_SCL_SECURE_NO_WARNINGS=1 )
ENDIF()

ADD_LIBRARY(lc7jtr SHARED ${lc7jtr_SOURCES} ${lc7jtr_HEADERS} ${lc7jtr_UI_HEADERS} )
#TARGET_LINK_LIBRARIES(lc7jtr)

qt5_use_modules(lc7jtr Widgets)

IF(APPLE)
    ADD_CUSTOM_COMMAND(TARGET lc7jtr POST_BUILD
        COMMAND install_name_tool
            -change "/opt/homebrew/opt/qt@5/lib/QtWidgets.framework/Versions/5/QtWidgets"
                    "@loader_path/../../../Frameworks/QtWidgets.framework/Versions/5/QtWidgets"
            -change "/opt/homebrew/opt/qt@5/lib/QtGui.framework/Versions/5/QtGui"
                    "@loader_path/../../../Frameworks/QtGui.framework/Versions/5/QtGui"
            -change "/opt/homebrew/opt/qt@5/lib/QtCore.framework/Versions/5/QtCore"
                    "@loader_path/../../../Frameworks/QtCore.framework/Versions/5/QtCore"
            "$<TARGET_FILE:lc7jtr>"
        COMMENT "Fixing Qt framework paths in lc7jtr"
    )
    ADD_CUSTOM_COMMAND(TARGET lc7jtr POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_BINARY_DIR}/dist/lc7.app/Contents/MacOS/lcplugins/lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}"
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:lc7jtr>"
            "${CMAKE_BINARY_DIR}/dist/lc7.app/Contents/MacOS/lcplugins/lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}/liblc7jtr.dylib"
        COMMENT "Copying lc7jtr plugin into app bundle"
    )
    IF(TARGET jtrdll_apple_silicon)
        ADD_DEPENDENCIES(lc7jtr jtrdll_apple_silicon)
        ADD_CUSTOM_COMMAND(TARGET lc7jtr POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:jtrdll_apple_silicon>"
                "${CMAKE_BINARY_DIR}/dist/lc7.app/Contents/MacOS/lcplugins/lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}/jtrdll_apple-silicon.dylib"
            COMMENT "Sync jtrdll_apple-silicon into app bundle (must match lc7jtr)"
        )
    ENDIF()
ENDIF()

ADD_PRECOMPILED_HEADER(lc7jtr stdafx lc7jtr_SOURCES)
