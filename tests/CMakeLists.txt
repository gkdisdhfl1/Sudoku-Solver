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
add_test(NAME SudokuSolverTest COMMAND tst_sudoku_solver)

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
add_test(NAME SudokuBackendTest COMMAND tst_sudoku_backend)