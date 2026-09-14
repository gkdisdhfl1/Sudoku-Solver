function(qtest_register_tests target source_file suite_name)
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}" test_lines
        REGEX "^[ \t]*void[ \t]+test[A-Za-z0-9_]+[ \t]*\\(")
    foreach(line IN LISTS test_lines)
        string(REGEX MATCH "test[A-Za-z0-9_]+" test_name "${line}")
        # testFoo_data()는 Qt Test 데이터 공급 함수이므로 제외
        if(test_name AND NOT test_name MATCHES "_data$")
            set(test_id "${suite_name}.${test_name}")
            add_test(NAME "${test_id}"
                COMMAND $<TARGET_FILE:${target}> "${test_name}" -v2)
            # Windows에서 Ctest 단독 실행 시 Qt DLL 경로를 자동으로 PATH 맨 앞에 추가
            if(WIN32)
                set_tests_properties("${test_id}" PROPERTIES
                    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"                
                )
            endif()
        endif()
    endforeach()
endfunction()

find_package(Qt6 REQUIRED COMPONENTS Test)

# ==============================================================================
# 1. SudokuSolver 핵심 알고리즘 단위 테스트
# ==============================================================================
qt_add_executable(tst_sudoku_solver
    tst_sudoku_solver.cpp
    ${CMAKE_SOURCE_DIR}/sudoku_solver.cpp
    ${CMAKE_SOURCE_DIR}/sudoku_solver.h
    ${CMAKE_SOURCE_DIR}/sudoku_constants.h
)

target_include_directories(tst_sudoku_solver PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(tst_sudoku_solver PRIVATE Qt6::Test Qt6::Core)
qtest_register_tests(tst_sudoku_solver tst_sudoku_solver.cpp SudokuSolver)

# ==============================================================================
# 2. SudokuBackend 모델 및 캐시 통합 테스트
# ==============================================================================
qt_add_executable(tst_sudoku_backend
    tst_sudoku_backend.cpp
    ${CMAKE_SOURCE_DIR}/sudoku_backend.cpp
    ${CMAKE_SOURCE_DIR}/sudoku_backend.h
    ${CMAKE_SOURCE_DIR}/sudoku_solver.cpp
    ${CMAKE_SOURCE_DIR}/sudoku_solver.h
    ${CMAKE_SOURCE_DIR}/solver_worker.cpp
    ${CMAKE_SOURCE_DIR}/solver_worker.h
    ${CMAKE_SOURCE_DIR}/sudoku_constants.h
)
target_include_directories(tst_sudoku_backend PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(tst_sudoku_backend PRIVATE Qt6::Test Qt6::Quick)
qtest_register_tests(tst_sudoku_backend tst_sudoku_backend.cpp SudokuBackend)