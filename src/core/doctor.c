/* doctor.c — voir doctor.h. */
#include "doctor.h"

#include <stdlib.h>
#include <string.h>

#include "fs.h"
#include "process.h"
#include "strbuf.h"
#include "text.h"
#include "xalloc.h"

typedef enum { DISTRO_OTHER, DISTRO_ARCH, DISTRO_DEBIAN, DISTRO_FEDORA } distro_family;

typedef struct {
    const char *command, *purpose;
    int essential;
    const char *hint[4]; /* indexé par distro_family */
} tool_def;

static const tool_def tools[] = {
    { "g++", "exercices C++", 1,
      { NULL, "sudo pacman -S gcc", "sudo apt install g++", "sudo dnf install gcc-c++" } },
    { "git", "exercices Git", 1,
      { NULL, "sudo pacman -S git", "sudo apt install git", "sudo dnf install git" } },
    { "dotnet", "exercices C# (étape 6)", 0,
      { "voir https://learn.microsoft.com/dotnet/core/install", "sudo pacman -S dotnet-sdk",
        "voir https://learn.microsoft.com/dotnet/core/install/linux",
        "voir https://learn.microsoft.com/dotnet/core/install/linux" } },
    { "cargo", "exercices Rust (étape 6)", 0,
      { "voir https://rustup.rs", "sudo pacman -S rust", "voir https://rustup.rs",
        "sudo dnf install cargo" } },
    { "code", "éditeur VS Code (trainer start --open)", 0,
      { "voir https://code.visualstudio.com", "yay -S visual-studio-code-bin (AUR)",
        "voir https://code.visualstudio.com", "voir https://code.visualstudio.com" } },
    { "clangd", "autocomplétion C++ dans l'éditeur", 0,
      { NULL, "sudo pacman -S clang", "sudo apt install clangd",
        "sudo dnf install clang-tools-extra" } },
    { "valgrind", "recherche de fuites mémoire (développement)", 0,
      { NULL, "sudo pacman -S valgrind", "sudo apt install valgrind",
        "sudo dnf install valgrind" } },
};
#define NTOOLS (sizeof tools / sizeof tools[0])

/* Lit ID= et ID_LIKE= dans /etc/os-release (format standard des distributions). */
static distro_family detect_distro(char **id_out)
{
    strbuf text = STRBUF_INIT, ids = STRBUF_INIT;
    *id_out = xstrdup("");
    if (fs_read_file("/etc/os-release", &text) != 0) {
        sb_free(&text);
        return DISTRO_OTHER;
    }
    const char *cursor = sb_cstr(&text), *line;
    size_t len;
    while (text_next_line(&cursor, &line, &len)) {
        int is_id = span_starts_with(line, len, "ID=");
        int is_like = span_starts_with(line, len, "ID_LIKE=");
        if (!is_id && !is_like)
            continue;
        const char *v = (const char *)memchr(line, '=', len) + 1;
        size_t vn = len - (size_t)(v - line);
        if (vn >= 2 && v[0] == '"' && v[vn - 1] == '"') { /* ID="arch" */
            v++;
            vn -= 2;
        }
        if (is_id) {
            free(*id_out);
            *id_out = xstrndup(v, vn);
        }
        sb_append_char(&ids, ' ');
        sb_append_n(&ids, v, vn);
        sb_append_char(&ids, ' ');
    }
    const char *s = sb_cstr(&ids);
    distro_family f = DISTRO_OTHER;
    if (strstr(s, " arch "))
        f = DISTRO_ARCH; /* Arch, Manjaro, EndeavourOS... */
    else if (strstr(s, " debian ") || strstr(s, " ubuntu "))
        f = DISTRO_DEBIAN;
    else if (strstr(s, " fedora ") || strstr(s, " rhel "))
        f = DISTRO_FEDORA;
    sb_free(&text);
    sb_free(&ids);
    return f;
}

static char *probe_version(const char *command, int *found)
{
    const char *argv[] = { command, "--version", NULL };
    proc_opts opts = { 0 };
    opts.timeout_ms = 10000;
    opts.max_output = 64 * 1024;
    proc_result res;
    char *version = NULL;
    *found = proc_run(argv, &opts, &res, NULL) == 0;
    if (*found) {
        /* Première ligne non vide, sur stdout ou à défaut stderr. */
        const char *src = res.out.len ? sb_cstr(&res.out) : sb_cstr(&res.err);
        const char *cursor = src, *line;
        size_t len;
        while (text_next_line(&cursor, &line, &len)) {
            span_trim(&line, &len);
            if (len > 0) {
                version = xstrndup(line, len);
                break;
            }
        }
    }
    proc_result_free(&res);
    return version;
}

void doctor_run(doctor_report *rep)
{
    memset(rep, 0, sizeof *rep);
    distro_family family = detect_distro(&rep->distro);
    rep->tools = xcalloc(NTOOLS, sizeof *rep->tools);
    rep->len = NTOOLS;
    for (size_t i = 0; i < NTOOLS; i++) {
        tool_status *t = &rep->tools[i];
        t->command = tools[i].command;
        t->purpose = tools[i].purpose;
        t->essential = tools[i].essential;
        t->version = probe_version(t->command, &t->found);
        const char *hint = tools[i].hint[family];
        if (!hint)
            hint = "installez-le avec le gestionnaire de paquets de votre système";
        t->install_hint = xstrdup(hint);
    }
}

void doctor_free(doctor_report *rep)
{
    for (size_t i = 0; i < rep->len; i++) {
        free(rep->tools[i].version);
        free(rep->tools[i].install_hint);
    }
    free(rep->tools);
    free(rep->distro);
    memset(rep, 0, sizeof *rep);
}
