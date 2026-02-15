add_executable(CS2-File-Dumper
    main.cpp
    steam_finder.h
    steam_finder.cpp
    file_collector.h
    file_collector.cpp
    git_manager.h
    git_manager.cpp
)

if(MSVC)
    target_compile_options(CS2-File-Dumper PRIVATE /EHsc)
endif()

if(WIN32)
    target_link_libraries(CS2-File-Dumper PRIVATE Advapi32)
endif()

# Передаём путь к корню проекта в код
target_compile_definitions(CS2-File-Dumper PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)