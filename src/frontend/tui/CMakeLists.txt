add_executable(vault-tui)

target_sources(vault-tui PUBLIC
        commands/add.h
        commands/add.cpp
        commands/create.h
        commands/create.cpp
        commands/destroy.h
        commands/destroy.cpp
        commands/exitcodes.h
        commands/list.h
        commands/list.cpp
        commands/remove.h
        commands/remove.cpp
        commands/search.h
        commands/search.cpp
        commands/show.h
        commands/show.cpp
        commands/update.h
        commands/update.cpp
        ui/colors.h
        ui/prompt.h
        ui/prompt.cpp
        utils/display.h
        utils/display.cpp
        utils/env.h
        utils/env.cpp
        utils/prompt.h
        utils/prompt.cpp
        utils/vaults.h
        utils/vaults.cpp
        main.cpp
)

target_include_directories(vault-tui PUBLIC .)

target_link_libraries(vault-tui PRIVATE vault args)

set_target_properties(vault-tui PROPERTIES OUTPUT_NAME vault)
