file (GLOB_RECURSE SRCS . *.cpp)

add_library(${PROJECT_NAME}
    ${SRCS}
)

add_dependencies(${PROJECT_NAME} ${EXTERNAL_PROJECTS} ${EXTERNAL_TEST_PROJECTS})

# target_link_libraries(netsim STATIC
#     # Add libraries here
#     ${PROJECT_NAME}
#     ${EXTERNAL_LIBRARIES}
# )
