#include "command_line.h"
#include <assert.h>
#include <stdio.h>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <wordexp.h>
#endif

int main()
{
    const char *args[] = {"+connect", "127.0.0.1:27015", "+map", "de_dust2", "+exec",
        "file with spaces.cfg", "", "quote\"value", "trailing slash\\", "a'quote",
        "$HOME", "$(echo unwanted)", "a;b", "*", "line\nbreak"};
    char cmd[4096] = {};
#ifdef _WIN32
    assert(Vellum_AppendArgument(cmd, sizeof(cmd), "test.exe"));
#endif
#ifdef _WIN32
    const bool shell = false;
#else
    const bool shell = true;
#endif
    for (const char *arg : args)
        assert(Vellum_AppendArgument(cmd, sizeof(cmd), arg, shell));
#ifdef _WIN32
    wchar_t wide[4096];
    assert(MultiByteToWideChar(CP_UTF8, 0, cmd, -1, wide, 4096));
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(wide, &argc);
    assert(argv && argc == sizeof(args) / sizeof(args[0]) + 1);
    for (int i = 1; i < argc; ++i) {
        char value[4096];
        assert(WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, value, sizeof(value), NULL, NULL));
        assert(strcmp(value, args[i - 1]) == 0);
    }
    LocalFree(argv);
#else
    wordexp_t words = {};
    assert(wordexp(cmd, &words, WRDE_NOCMD) == 0);
    assert(words.we_wordc == sizeof(args) / sizeof(args[0]));
    for (size_t i = 0; i < words.we_wordc; ++i)
        assert(strcmp(words.we_wordv[i], args[i]) == 0);
    wordfree(&words);
#endif
    char limited[8] = "keep";
    assert(!Vellum_AppendArgument(limited, sizeof(limited), "too long"));
    assert(strcmp(limited, "keep") == 0);
    puts("loader argument smoke passed");
}

