configure_file("common/Constants.h.in" "common/Constants.h" IMMEDIATE @ONLY)

add_subdirectory(daemon)
add_subdirectory(greeter)
add_subdirectory(helper)

if(BUILD_NEO_GREETER)
    add_subdirectory(greeter-neo)
endif()
