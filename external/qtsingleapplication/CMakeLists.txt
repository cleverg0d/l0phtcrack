CMAKE_MINIMUM_REQUIRED(VERSION 3.10 FATAL_ERROR)

PROJECT(qtsingleapplication)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)

SET(qtsingleapplication_SOURCES 
	src/qtlocalpeer.cpp
	#src/qtlockedfile.cpp
	#src/qtlockedfile_unix.cpp
	#src/qtlockedfile_win.cpp
	src/qtsingleapplication.cpp
	#src/qtsinglecoreapplication.cpp
)

SET(qtsingleapplication_HEADERS
	src/qtlocalpeer.h
	#src/qtlockedfile.h
	src/qtsingleapplication.h
	#src/qtsinglecoreapplication.h
)

#MESSAGE(qtsingleapplication_SOURCES=${qtsingleapplication_SOURCES})
#MESSAGE(qtsingleapplication_HEADERS=${qtsingleapplication_HEADERS})

ADD_DEFINITIONS(-DQT_QTSINGLEAPPLICATION_EXPORT=1 -DUNICODE=1)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} "src")
ADD_LIBRARY(qtsingleapplication SHARED ${qtsingleapplication_SOURCES} ${qtsingleapplication_HEADERS} )
qt5_use_modules(qtsingleapplication Widgets Network)
#TARGET_LINK_LIBRARIES(qtsingleapplication)

IF(APPLE)
    ADD_CUSTOM_COMMAND(TARGET qtsingleapplication POST_BUILD
        COMMAND install_name_tool
            -change "/opt/homebrew/opt/qt@5/lib/QtWidgets.framework/Versions/5/QtWidgets"
                    "@executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets"
            -change "/opt/homebrew/opt/qt@5/lib/QtGui.framework/Versions/5/QtGui"
                    "@executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui"
            -change "/opt/homebrew/opt/qt@5/lib/QtNetwork.framework/Versions/5/QtNetwork"
                    "@executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork"
            -change "/opt/homebrew/opt/qt@5/lib/QtCore.framework/Versions/5/QtCore"
                    "@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore"
            "$<TARGET_FILE:qtsingleapplication>"
        COMMENT "Fixing Qt framework paths in libqtsingleapplication"
    )
ENDIF()
