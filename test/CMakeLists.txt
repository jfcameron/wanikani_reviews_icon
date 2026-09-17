# © 2020 Joseph Cameron - All Rights Reserved
jfc_add_tests(
    C++_STANDARD 20

    TEST_SOURCE_FILES
        "${CMAKE_CURRENT_LIST_DIR}/review_count_test.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/../src/review_count.cpp"

    INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_LIST_DIR}/../include"
        "${CMAKE_CURRENT_LIST_DIR}/../src/include"
)
