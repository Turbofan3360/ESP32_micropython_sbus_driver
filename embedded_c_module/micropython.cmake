add_library(usermod_sbus INTERFACE)

target_sources(usermod_sbus INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/sbus.c
)

target_include_directories(usermod_sbus INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(usermod INTERFACE usermod_sbus)