CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(lc7importwin)

INCLUDE(../cmake/PCH.cmake)
IF(MSVC)
	INCLUDE(../cmake/IDL.cmake)
ENDIF()

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(OpenSSL REQUIRED)
IF(NOT APPLE)
	FIND_PACKAGE(libssh2 REQUIRED)
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	SET(PLATFORM_LIBS Shlwapi.lib netapi32.lib crypt32.lib esent.lib rpcrt4.lib Secur32.lib Activeds.lib Adsiid.lib ws2_32.lib)
	SET(PLATFORM_INCLUDE_DIRS "")
	SET(PLATFORM_LIBRARY_DIRS "")
	SET(PLATFORM_DEFINITIONS "")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	SET(PLATFORM_LIBS "")
	SET(PLATFORM_INCLUDE_DIRS "")
	SET(PLATFORM_LIBRARY_DIRS "")
	SET(PLATFORM_DEFINITIONS "")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	SET(PLATFORM_LIBS "")
	SET(PLATFORM_INCLUDE_DIRS "")
	SET(PLATFORM_LIBRARY_DIRS "")
	SET(PLATFORM_DEFINITIONS "")
ENDIF()

IF(APPLE)
# macOS: PWDump, SAM/SYSTEM, and NTDS.DIT (via libesedb). No Local/Remote/DRSR (require Windows APIs).
FIND_LIBRARY(LIBESEDB_LIB NAMES esedb libesedb
    PATHS /opt/homebrew/lib /usr/local/lib
    REQUIRED)
FIND_PATH(LIBESEDB_INCLUDE NAMES libesedb.h
    PATHS /opt/homebrew/include /usr/local/include
    REQUIRED)

SET(lc7importwin_SOURCES
	src/stdafx.cpp
	src/PWDumpImporter.cpp
	src/SAMImporter.cpp
	src/NTDSImporter.cpp
	src/ntds_esedb.cpp
	src/CImportPWDump.cpp
	src/CImportPWDumpGUI.cpp
	src/CImportSAM.cpp
	src/CImportSAMGUI.cpp
	src/CImportNTDS.cpp
	src/CImportNTDSGUI.cpp
	src/CLC7ImportWinPlugin.cpp
	src/importpwdumpconfig.cpp
	src/importsamconfig.cpp
	src/importntdsconfig.cpp
	src/linkage.cpp
	src/slow_des.cpp
	src/slow_md5.cpp
	src/slow_rc4.cpp
	src/ntreg.cpp
	src/crypt.cpp
	src/utils.cpp
	src/windows_abstraction.cpp
	src/CWindowsImportSettings.cpp
)

SET(lc7importwin_FORMS
	resources/importpwdumpconfig.ui
	resources/importsamconfig.ui
	resources/importntdsconfig.ui
)
ELSE()
SET(lc7importwin_SOURCES
	src/stdafx.cpp
	src/CImportWindows.cpp
	src/PWDumpImporter.cpp
	src/SAMImporter.cpp
	src/NTDSImporter.cpp
	src/DRSRImporter.cpp
	src/CImportPWDump.cpp
	src/CImportPWDumpGUI.cpp
	src/CImportSAM.cpp
	src/CImportSAMGUI.cpp
	src/CImportNTDS.cpp
	src/CImportNTDSGUI.cpp
	src/CImportWindowsLocal.cpp
	src/CImportWindowsLocalGUI.cpp
	src/CImportWindowsRemote.cpp
	src/CImportWindowsRemoteGUI.cpp
	src/CLC7ImportWinPlugin.cpp
	src/importpwdumpconfig.cpp
	src/importsamconfig.cpp
	src/importntdsconfig.cpp
	src/importlocalconfig.cpp
	src/importremoteconfig.cpp
	src/linkage.cpp
	src/slow_des.cpp
	src/slow_md5.cpp
	src/slow_rc4.cpp
	src/ntreg.cpp
	src/ntds.cpp
	src/crypt.cpp
	src/utils.cpp
	src/midl_alloc.cpp
	src/drsr_addons.cpp
	src/drsr_alloc.cpp
	src/windows_abstraction.cpp
	src/GenerateRemoteAgentDlg.cpp
	src/CWindowsImportSettings.cpp
	../win-agent/lc7dump/PubKeyFile.cpp
)

SET(lc7importwin_FORMS
	resources/importlocalconfig.ui
	resources/importremoteconfig.ui
	resources/importpwdumpconfig.ui
	resources/importsamconfig.ui
	resources/importntdsconfig.ui
	resources/generateremoteagent.ui
)
ENDIF()

#SET(lc7importwin_RESOURCES
#	resources/lc7importwin.qrc
#)

IF(NOT APPLE)
set(lc7importwin_IDL
	${CMAKE_CURRENT_SOURCE_DIR}/idl/drsr.idl
)
ENDIF()

IF(APPLE)
SET(lc7importwin_HEADERS
	include/PWDumpImporter.h
	include/SAMImporter.h
	include/NTDSImporter.h
	include/CImportPWDump.h
	include/CImportPWDumpGUI.h
	include/CImportSAM.h
	include/CImportSAMGUI.h
	include/CImportNTDS.h
	include/CImportNTDSGUI.h
	include/CLC7ImportWinPlugin.h
	include/importpwdumpconfig.h
	include/importsamconfig.h
	include/importntdsconfig.h
	include/linkage.h
	include/stdafx.h
	include/uuids.h
	include/slow_md5.h
	include/slow_des.h
	include/slow_rc4.h
	include/ntreg.h
	include/utils.h
	include/crypt.h
	include/windows_abstraction.h
	include/win_compat.h
	include/ntds.h
	include/CWindowsImportSettings.h
)
ELSE()
SET(lc7importwin_HEADERS
	include/CImportWindows.h
	include/PWDumpImporter.h
	include/SAMImporter.h
	include/NTDSImporter.h
	include/DRSRImporter.h
	include/CImportPWDump.h
	include/CImportPWDumpGUI.h
	include/CImportSAM.h
	include/CImportSAMGUI.h
	include/CImportNTDS.h
	include/CImportNTDSGUI.h
	include/CImportWindowsLocal.h
	include/CImportWindowsLocalGUI.h
	include/CImportWindowsRemote.h
	include/CImportWindowsRemoteGUI.h
	include/CLC7ImportWinPlugin.h
	include/importlocalconfig.h
	include/importpwdumpconfig.h
	include/importsamconfig.h
	include/importntdsconfig.h
	include/importremoteconfig.h
	include/linkage.h
	include/stdafx.h
	include/uuids.h
	include/slow_md5.h
	include/slow_des.h
	include/slow_rc4.h
	include/ntreg.h
	include/ntds.h
	include/utils.h
	include/crypt.h
	include/midl_alloc.h
	include/drsr_addons.h
	include/drsr_alloc.h
	include/windows_abstraction.h
	include/GenerateRemoteAgentDlg.h
	include/CWindowsImportSettings.h
	../win-agent/lc7dump/PubKeyFile.h
)
ENDIF()

#MESSAGE(lc7importwin_SOURCES=${lc7importwin_SOURCES})
#MESSAGE(lc7importwin_FORMS=${lc7importwin_FORMS})
#MESSAGE(lc7importwin_HEADERS=${lc7importwin_HEADERS})

QT5_WRAP_UI(lc7importwin_UI_HEADERS
    ${lc7importwin_FORMS}
)

QT5_ADD_RESOURCES(ResourceSources ${lc7importwin_RESOURCES})

ADD_DEFINITIONS(-DUNICODE=1 ${PLATFORM_DEFINITIONS})

LINK_DIRECTORIES( 
	${PLATFORM_LIBRARY_DIRS}
)

IF(APPLE)
INCLUDE_DIRECTORIES("include" "${CMAKE_CURRENT_BINARY_DIR}" "../api/include" ${OPENSSL_INCLUDE_DIR} ${LIBESEDB_INCLUDE})
ELSE()
INCLUDE_DIRECTORIES("include" "idl" "${CMAKE_CURRENT_BINARY_DIR}" "../api/include" "../win-agent/lc7dump" ${OPENSSL_INCLUDE_DIR} ${PLATFORM_INCLUDE_DIRS})
ENDIF()

IF(NOT APPLE)
# Build idl files
MACRO_ADD_INTERFACES(GENERATED_FILES_IDL ${lc7importwin_IDL})
ENDIF()

SOURCE_GROUP("Source Files" FILES ${lc7importwin_SOURCES})
SOURCE_GROUP("Header Files" FILES ${lc7importwin_HEADERS})
IF(NOT APPLE)
SOURCE_GROUP("IDL" FILES ${GENERATED_FILES_IDL} ${lc7importwin_IDL})
ENDIF()

################

ADD_LIBRARY(lc7importwin SHARED
	${lc7importwin_SOURCES} ${lc7importwin_HEADERS}
	${lc7importwin_UI_HEADERS}
	${lc7importwin_IDL} ${GENERATED_FILES_IDL}
	${lc7importwin_RESOURCES}
	${ResourceSources}
)

IF(APPLE)
TARGET_LINK_LIBRARIES(lc7importwin ${OPENSSL_LIBRARIES} ${LIBESEDB_LIB} lc7core)
ELSE()
TARGET_LINK_LIBRARIES(lc7importwin ${OPENSSL_LIBRARIES} ${PLATFORM_LIBS} lc7core)
ENDIF()

qt5_use_modules(lc7importwin Widgets)

IF(APPLE)
ADD_CUSTOM_COMMAND(TARGET lc7importwin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/manifest.macos.json"
        "$<TARGET_FILE_DIR:lc7importwin>/manifest.json"
    COMMENT "lc7importwin manifest for macOS (plugin folder requires manifest.json)"
)
# Fix library paths to use @loader_path (relative to the plugin in the app bundle)
# so the plugin uses the bundled Qt and OpenSSL, not Homebrew absolute paths
ADD_CUSTOM_COMMAND(TARGET lc7importwin POST_BUILD
    COMMAND install_name_tool
        -change "/opt/homebrew/opt/qt@5/lib/QtWidgets.framework/Versions/5/QtWidgets"
                "@loader_path/../../../Frameworks/QtWidgets.framework/Versions/5/QtWidgets"
        -change "/opt/homebrew/opt/qt@5/lib/QtGui.framework/Versions/5/QtGui"
                "@loader_path/../../../Frameworks/QtGui.framework/Versions/5/QtGui"
        -change "/opt/homebrew/opt/qt@5/lib/QtCore.framework/Versions/5/QtCore"
                "@loader_path/../../../Frameworks/QtCore.framework/Versions/5/QtCore"
        -change "/opt/homebrew/opt/openssl@3/lib/libssl.3.dylib"
                "@loader_path/../../../Frameworks/libssl.3.dylib"
        -change "/opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib"
                "@loader_path/../../../Frameworks/libcrypto.3.dylib"
        "$<TARGET_FILE:lc7importwin>"
    COMMENT "Fixing library paths for macOS app bundle"
)
ENDIF()

ADD_PRECOMPILED_HEADER(lc7importwin stdafx lc7importwin_SOURCES)

