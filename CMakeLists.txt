cmake_minimum_required(VERSION 3.25)

project(CryptoResearchFramework VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CRYPTO_RESEARCH_BUILD_TESTS "Build the unit-test suite" ON)
option(CRYPTO_RESEARCH_ENABLE_CUDA "Enable CUDA image-analysis acceleration when a CUDA compiler is available" OFF)

find_package(OpenSSL REQUIRED)
find_package(libsecp256k1 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED COMPONENTS core imgcodecs)

set(CRYPTO_RESEARCH_GPU_SOURCE gpu/bitplane.cpp)
if(CRYPTO_RESEARCH_ENABLE_CUDA)
    include(CheckLanguage)
    check_language(CUDA)
    if(NOT CMAKE_CUDA_COMPILER)
        message(FATAL_ERROR "CRYPTO_RESEARCH_ENABLE_CUDA requires the CUDA Toolkit and an NVIDIA CUDA compiler.")
    endif()
    enable_language(CUDA)
    set(CRYPTO_RESEARCH_GPU_SOURCE gpu/bitplane_cuda.cu)
endif()

add_library(crypto_research_core
    core/hashing/sha256.cpp
    core/hashing/pbkdf2.cpp
    core/hashing/hmac.cpp
    core/hashing/ripemd160.cpp
    core/encoding/base58.cpp
    core/utils/hex.cpp
    core/wordlists/local_wordlist.cpp
    core/utils/thread_pool.cpp
    bitcoin/private_key.cpp
    bitcoin/p2pkh.cpp
    bitcoin/bip39/mnemonic.cpp
    bitcoin/bip39/wordlist.cpp
    bitcoin/electrum/old_wordlist.cpp
    bitcoin/bip32/xpub.cpp
    bitcoin/address/public_address_matcher.cpp
    bitcoin/address/format.cpp
    image_analysis/png_analysis.cpp
    steganography/bitplane.cpp
    ${CRYPTO_RESEARCH_GPU_SOURCE}
)

add_library(crypto_research::core ALIAS crypto_research_core)

target_include_directories(crypto_research_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(crypto_research_core
    PUBLIC
        OpenSSL::Crypto
        libsecp256k1::secp256k1
        opencv_core
        opencv_imgcodecs
)

if(CRYPTO_RESEARCH_ENABLE_CUDA)
    find_package(CUDAToolkit REQUIRED)
    target_link_libraries(crypto_research_core PRIVATE CUDA::cudart)
    target_compile_definitions(crypto_research_core PUBLIC CRYPTO_RESEARCH_WITH_CUDA=1)
    set_target_properties(crypto_research_core PROPERTIES CUDA_STANDARD 20 CUDA_STANDARD_REQUIRED ON)
endif()

target_compile_features(crypto_research_core PUBLIC cxx_std_20)

if(MSVC)
    # CUDA has its own option parser; keep MSVC-only warnings off nvcc's command line.
    target_compile_options(crypto_research_core PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/W4>
        $<$<COMPILE_LANGUAGE:CXX>:/permissive->
    )
else()
    target_compile_options(crypto_research_core PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall>
        $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
        $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
        $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
    )
endif()

add_executable(crypto_research app/main.cpp cli/address_command.cpp cli/image_command.cpp cli/batch_image_command.cpp)
target_link_libraries(crypto_research PRIVATE crypto_research::core spdlog::spdlog nlohmann_json::nlohmann_json)

add_executable(crypto_research_safe_benchmarks benchmarks/safe_benchmarks.cpp)
target_link_libraries(crypto_research_safe_benchmarks PRIVATE crypto_research::core)

add_executable(public_address_list_matcher app/public_address_list_matcher.cpp)
target_link_libraries(public_address_list_matcher PRIVATE crypto_research::core)

add_executable(png_report app/png_report.cpp)
target_link_libraries(png_report PRIVATE crypto_research::core nlohmann_json::nlohmann_json)
add_executable(address_format_report app/address_format_report.cpp)
target_link_libraries(address_format_report PRIVATE crypto_research::core nlohmann_json::nlohmann_json)
add_executable(address_intersection_report app/address_intersection_report.cpp)
target_link_libraries(address_intersection_report PRIVATE crypto_research::core nlohmann_json::nlohmann_json)
add_executable(evidence_text_report app/evidence_text_report.cpp)
target_link_libraries(evidence_text_report PRIVATE nlohmann_json::nlohmann_json)

install(TARGETS crypto_research_safe_benchmarks public_address_list_matcher png_report address_format_report address_intersection_report evidence_text_report
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(DIRECTORY examples/ DESTINATION examples)
install(FILES README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "CryptoResearchFramework-offline")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_BUILD_CONFIG "Debug")
include(CPack)

if(CRYPTO_RESEARCH_BUILD_TESTS)
    include(CTest)
    enable_testing()

    find_package(GTest CONFIG REQUIRED)

    add_executable(crypto_research_tests
        tests/hashing_tests.cpp
        tests/base58_tests.cpp
        tests/bitcoin_tests.cpp
        tests/bip39_tests.cpp
        tests/wordlist_tests.cpp
        tests/steganography_tests.cpp
        tests/xpub_tests.cpp
        tests/public_address_matcher_tests.cpp
        tests/address_format_tests.cpp
        tests/thread_pool_tests.cpp
    )
    target_link_libraries(crypto_research_tests PRIVATE crypto_research::core GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(crypto_research_tests)
endif()
