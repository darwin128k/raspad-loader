#pragma once

#include <string>
#include <string.h>

// Quote one argument when reconstructing a Windows/engine command line.
static std::string Vellum_QuoteArgument(const char *arg)
{
    if (*arg && strpbrk(arg, " \t\n\v\"") == NULL) return arg;
    std::string out = "\"";
    size_t slashes = 0;
    for (const char *p = arg; ; ++p) {
        if (*p == '\\') { ++slashes; continue; }
        if (*p == '"' || *p == '\0') out.append(slashes * 2, '\\');
        else out.append(slashes, '\\');
        slashes = 0;
        if (*p == '\0') break;
        if (*p == '"') out += '\\';
        out += *p;
    }
    return out + '"';
}

// Linux wrappers execute through /bin/sh: keep each original argv token literal.
static std::string Vellum_QuoteShellArgument(const char *arg)
{
    std::string out = "'";
    for (const char *p = arg; *p; ++p) {
        if (*p == '\'') out += "'\\''";
        else out += *p;
    }
    return out + '\'';
}

static bool Vellum_AppendArgument(char *cmd, size_t size, const char *arg, bool shell = false)
{
    std::string quoted = shell ? Vellum_QuoteShellArgument(arg) : Vellum_QuoteArgument(arg);
    size_t n = strlen(cmd);
    size_t separator = n ? 1 : 0;
    if (n + separator + quoted.size() >= size) return false;
    if (separator) cmd[n++] = ' ';
    memcpy(cmd + n, quoted.c_str(), quoted.size() + 1);
    return true;
}
