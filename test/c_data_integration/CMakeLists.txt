include(compile_options)

if(NOT FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON")
    find_package(nlohmann_json ${FIND_PACKAGE_OPTIONS})
endif()

if(FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON" OR FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "MISSING") 
    include(FetchContent)
    if(NOT nlohmann_json_FOUND)
        set(NLOHMANN_JSON_VERSION "v3.12.0")
        message(STATUS "📦 Fetching nlohmann_json ${NLOHMANN_JSON_VERSION}")
        FetchContent_Declare(
            nlohmann_json
            GIT_SHALLOW TRUE
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG ${NLOHMANN_JSON_VERSION}
            GIT_PROGRESS TRUE
            SYSTEM
            EXCLUDE_FROM_ALL)
        FetchContent_MakeAvailable(nlohmann_json)
        message(STATUS "\t✅ Fetched nlohmann_json ${NLOHMANN_JSON_VERSION}")
    endif()
endif()

find_package(doctest REQUIRED)

add_library(c_data_integration SHARED
    src/binary_parser.cpp
    src/bool_parser.cpp
    src/c_data_integration.cpp
    src/comparison.cpp
    src/fixedsizebinary_parser.cpp
    src/json_parser.cpp
    src/primitive_parser.cpp
    src/string_parser.cpp
    src/stringview_parser.cpp
    src/utils.cpp
    include/sparrow/c_data_integration/binary_parser.hpp
    include/sparrow/c_data_integration/bool_parser.hpp
    include/sparrow/c_data_integration/c_data_integration.hpp
    include/sparrow/c_data_integration/comparison.hpp
    include/sparrow/c_data_integration/config.hpp
    include/sparrow/c_data_integration/constant.hpp
    include/sparrow/c_data_integration/fixedsizebinary_parser.hpp
    include/sparrow/c_data_integration/primitive_parser.hpp
    include/sparrow/c_data_integration/string_parser.hpp
    include/sparrow/c_data_integration/stringview_parser.hpp
    include/sparrow/c_data_integration/utils.hpp
    )

set_property(TARGET c_data_integration PROPERTY POSITION_INDEPENDENT_CODE ON)

target_compile_options(c_data_integration
    PRIVATE
        ${compile_options}
)

target_compile_definitions(c_data_integration
    PRIVATE
        SPARROW_C_DATA_INTEGRATION_EXPORTS)

set_target_properties(c_data_integration
    PROPERTIES
        VERSION ${SPARROW_BINARY_VERSION}
        SOVERSION ${SPARROW_BINARY_CURRENT}
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        FOLDER c_data_integration
)

target_link_libraries(c_data_integration
    PUBLIC
        sparrow
        nlohmann_json::nlohmann_json)

target_include_directories(c_data_integration
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

add_subdirectory(test_c_data_integration)
        