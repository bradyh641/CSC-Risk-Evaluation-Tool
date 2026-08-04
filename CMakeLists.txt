cmake_minimum_required(VERSION 3.20)

project(CSCRiskEvalTool)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CSCRiskEvalTool
        main.cpp
        gui/MainWindow.cpp
        gui/MainWindow.hpp
        gui/FilePicker.cpp
        gui/FilePicker.hpp
        scanner/FileScanner.cpp
        scanner/FileScanner.hpp
        scanner/SheetScanner.cpp
        scanner/SheetScanner.hpp
        parser/PDFParser.cpp
        parser/PDFParser.hpp
        extraction/LLMCaller.cpp
        extraction/LLMCaller.hpp
        matching/CombineResults.cpp
        matching/CombineResults.hpp
        gui/LinkWindow.cpp
        gui/LinkWindow.hpp
        gui/OrganizationCard.hpp
        excel/GenerateSpreadsheet.cpp
        excel/GenerateSpreadsheet.hpp
)

# OpenXLSX
find_package(OpenXLSX CONFIG REQUIRED)

# Poppler
find_package(unofficial-poppler CONFIG REQUIRED)

# SFML
set(SFML_DIR "C:/msys2/mingw64/lib/cmake/SFML")
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# JSON
find_package(nlohmann_json CONFIG REQUIRED)

# OCR
find_package(Tesseract CONFIG REQUIRED)

# CSV Parser
find_package(unofficial-vincentlaucsb-csv-parser CONFIG REQUIRED)

target_link_libraries(CSCRiskEvalTool PRIVATE
        OpenXLSX::OpenXLSX

        unofficial::poppler::poppler-cpp

        sfml-graphics
        sfml-window
        sfml-system

        ole32
        shell32

        unofficial::vincentlaucsb-csv-parser::csv

        winhttp

        nlohmann_json::nlohmann_json

        Tesseract::libtesseract
)

#
# Install executable
#
install(TARGETS CSCRiskEvalTool
        RUNTIME DESTINATION .
)

#
# Install runtime DLLs
#
install(CODE [[
    file(GET_RUNTIME_DEPENDENCIES
        EXECUTABLES
            "$<TARGET_FILE:CSCRiskEvalTool>"

        DIRECTORIES
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin"

        RESOLVED_DEPENDENCIES_VAR RESOLVED_DLLS
        UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED_DLLS
    )

    foreach(DLL ${RESOLVED_DLLS})
        file(INSTALL
            DESTINATION "${CMAKE_INSTALL_PREFIX}"
            TYPE FILE
            FILES "${DLL}"
        )
    endforeach()

    if(UNRESOLVED_DLLS)
        message(WARNING "Unresolved DLLs: ${UNRESOLVED_DLLS}")
    endif()
]])

install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/resources
        DESTINATION .
)

install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/tessdata
        DESTINATION .
)

install(FILES
        README.md
        DESTINATION .
)

install(FILES
        config.json
        DESTINATION .
)