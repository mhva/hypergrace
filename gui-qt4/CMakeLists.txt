find_package(Qt4 4.6.0 COMPONENTS QtCore QtGui REQUIRED)
include(${QT_USE_FILE})
include_directories(${QT_INCLUDE_DIR}
                    ${CMAKE_SOURCE_DIR}/libhypergrace
                    ${CMAKE_CURRENT_BINARY_DIR})

set(HG_QT4_SOURCES
    fileiconprovider_linux.cc
    prettyprint.cc
    previewtorrentdialog.cc
    torrentfilesystemmodel.cc
    torrentfilesystemproxymodel.cc
    torrentstatewidget.cc
    torrentview.cc
    main.cc)

set(HG_QT4_QOBJECT_FILES
    torrentfilesystemmodel.hh
    torrentfilesystemproxymodel.hh
    torrentview.hh
    torrentstatewidget.hh
    previewtorrentdialog.hh)

set(HG_QT4_LNG_FILES
    hypergrace_en.ts)

set(HG_QT4_UI_FILES ui/previewtorrent.ui)

qt4_wrap_cpp(HG_QT4_MOC ${HG_QT4_QOBJECT_FILES})
qt4_wrap_ui(HG_QT4_UIC ${HG_QT4_UI_FILES})
qt4_add_resources(HG_QT4_RESOURCES resources/resources.qrc)
qt4_add_translation(HG_QT4_BINLNG_FILES ${HG_QT4_LNG_FILES})

add_executable(hypergrace-qt ${HG_QT4_SOURCES} ${HG_QT4_MOC} ${HG_QT4_UIC} ${HG_QT4_RESOURCES} ${HG_QT4_BINLNG_FILES})
target_link_libraries(hypergrace-qt ${QT_LIBRARIES} hypergrace)
