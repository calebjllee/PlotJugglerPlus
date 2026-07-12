include_directories(../)

set(SRC dataload_mf4.cpp)

add_library(DataLoadMF4 SHARED ${SRC})

target_link_libraries(DataLoadMF4 PRIVATE Qt5::Widgets Qt5::Xml plotjuggler_base)

target_compile_definitions(DataLoadMF4 PRIVATE QT_PLUGIN)

install(TARGETS DataLoadMF4 DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
