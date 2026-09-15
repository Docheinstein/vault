add_executable(vault-cli)

target_sources(vault-cli PUBLIC
        commands/add.h
        commands/add.cpp
        commands/init.h
        commands/init.cpp
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
        utils/cli.h
        utils/cli.cpp
        utils/colors.h
        utils/strings.h
        utils/strings.cpp
        utils/vault.h
        utils/vault.cpp
        main.cpp
        retcodes.h
)

if (ENABLE_GIT)
    target_sources(vault-cli PUBLIC
            git/add.h
            git/add.cpp
            git/commit.h
            git/commit.cpp
            git/init.h
            git/init.cpp
            git/remove.h
            git/remove.cpp
            utils/git.h
            utils/git.cpp
    )
endif()

target_include_directories(vault-cli PUBLIC .)

target_link_libraries(vault-cli PRIVATE vault args)

set_target_properties(vault-cli PROPERTIES OUTPUT_NAME vault)
