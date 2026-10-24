set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# ==============================================================================
# 3. Sudoku GUI E2E 상호작용 테스트 (Semantic Action DSL)
# ==============================================================================
qt_add_executable(tst_sudoku_gui
    tst_sudoku_gui.cpp
    sudoku_driver.h
    ${CMAKE_SOURCE_DIR}/sudoku_backend.cpp
    ${CMAKE_SOURCE_DIR}/sudoku_backend.h
    ${CMAKE_SOURCE_DIR}/sudoku_solver.cpp
    ${CMAKE_SOURCE_DIR}/sudoku_solver.h
    ${CMAKE_SOURCE_DIR}/solver_worker.cpp
    ${CMAKE_SOURCE_DIR}/solver_worker.h
    ${CMAKE_SOURCE_DIR}/sudoku_constants.h
    # QML 모듈 리소스를 테스트 실행 파일에도 임베딩
    ${CMAKE_BINARY_DIR}/.qt/rcc/qrc_appSudokuSolver_raw_qml_0.cpp
    ${CMAKE_BINARY_DIR}/.qt/rcc/qrc_appSudokuSolver_raw_qml_0_extra_qmldirs.cpp
    ${CMAKE_BINARY_DIR}/.qt/rcc/qrc_qmake_SudokuSolver.cpp
    # SudokuBackend QML 타입 등록 코드 링크
    ${CMAKE_BINARY_DIR}/appsudokusolver_qmltyperegistrations.cpp
)
target_compile_definitions(tst_sudoku_gui PRIVATE
    TEST_QML_IMPORT_DIR="${CMAKE_BINARY_DIR}"
)
target_include_directories(tst_sudoku_gui PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(tst_sudoku_gui PRIVATE Qt6::Test Qt6::Quick Qt6::Qml)
qtest_register_tests(tst_sudoku_gui tst_sudoku_gui.cpp SudokuGui)

# 오프스크린 QPA 강제
set_tests_properties(SudokuGui.testCellClickAndTypeOverwrite SudokuGui.testArrowKeyNavigation PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
