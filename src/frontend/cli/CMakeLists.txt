add_executable(vault-cli)

target_sources(vault-cli PUBLIC
        commands/add.h
        commands/add.cpp
        commands/init.h
        commands/init.cpp
        commands/exitcodes.h
        commands/list.h
        commands/list.cpp
        commands/remove.h
        commands/remove.cpp
        commands/search.h
        commands/search.cpp
        commands/show.h
        commands/show.cpp
        commands/edit.h
        commands/edit.cpp
        utils/colors.h
        utils/env.h
        utils/env.cpp
        utils/prompt.h
        utils/prompt.cpp
        utils/vaults.h
        utils/vaults.cpp
        main.cpp
)

target_include_directories(vault-cli PUBLIC .)

target_link_libraries(vault-cli PRIVATE vault args)

set_target_properties(vault-cli PROPERTIES OUTPUT_NAME vault)
