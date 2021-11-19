set(UTILITY_SRC
        command_parser.cpp
        command_parser.h)

add_library(utility SHARED
        ${UTILITY_SRC})