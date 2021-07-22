set(SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/src/screen.c
)

set(SOURCE_TEST_FILES ${SOURCE_FILES} PARENT_SCOPE)

add_executable(golc main.c ${SOURCE_FILES})
target_link_libraries(golc ${CURSES_LIBRARIES})
