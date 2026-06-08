function(add_cserialport_dependency)
    if (TARGET libcserialport)
        return()
    endif()

    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CSERIALPORT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(CSERIALPORT_BUILD_BINDING_C OFF CACHE BOOL "" FORCE)
    set(CSERIALPORT_BUILD_DOC OFF CACHE BOOL "" FORCE)
    set(CSERIALPORT_BUILD_TEST OFF CACHE BOOL "" FORCE)
    set(CSERIALPORT_ENABLE_DEBUG OFF CACHE BOOL "" FORCE)
    set(CSERIALPORT_ENABLE_UTF8 OFF CACHE BOOL "" FORCE)

    add_subdirectory(
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/CSerialPort-master
        ${CMAKE_BINARY_DIR}/third_party/cserialport
        EXCLUDE_FROM_ALL
    )

    target_include_directories(libcserialport
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/CSerialPort-master/include
    )
endfunction()
