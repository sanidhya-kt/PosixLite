#include "shell.h"
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>

// Global history definition
std::vector<std::string> history;

std::string get_prompt() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) strcpy(cwd, "?");
    std::string path(cwd);
    // Shorten home directory to ~
    const char* home = getenv("HOME");
    if (home && path.find(home) == 0)
        path = "~" + path.substr(strlen(home));
    return "\033[1;32mmsh\033[0m:\033[1;34m" + path + "\033[0m$ ";
}

int main() {
    // Shell initialisation
    shell_terminal = STDIN_FILENO;
    shell_pgid     = getpid();
    setpgid(shell_pgid, shell_pgid);
    tcsetpgrp(shell_terminal, shell_pgid);

    // Ignore job-control signals in the shell itself
    signal(SIGINT,  SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    std::cout << "\033[1;36m╔══════════════════════════════╗\n"
                 "║   Mini Unix Shell  (msh)     ║\n"
                 "║   Type 'exit' to quit        ║\n"
                 "╚══════════════════════════════╝\033[0m\n\n";

    std::string line;
    while (true) {
        reap_jobs(); // collect any finished background jobs
        std::cout << get_prompt() << std::flush;

        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break; // EOF (Ctrl-D)
        }

        // Trim
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        size_t end = line.find_last_not_of(" \t");
        if (end != std::string::npos) line = line.substr(0, end + 1);
        if (line.empty()) continue;

        // History expansion: !! and !n
        if (line == "!!") {
            if (history.empty()) { std::cerr << "No history.\n"; continue; }
            line = history.back();
            std::cout << line << "\n";
        } else if (line.size() > 1 && line[0] == '!') {
            try {
                int n = std::stoi(line.substr(1));
                if (n < 1 || n > (int)history.size()) { std::cerr << "No such history entry.\n"; continue; }
                line = history[n - 1];
                std::cout << line << "\n";
            } catch (const std::exception& e) {
                std::cerr << "Invalid history format.\n";
                continue;
            }
        }

        history.push_back(line);

        auto tokens   = tokenise(line);
        auto pipeline = parse(tokens);
        run_pipeline(pipeline, line);
    }

    return 0;
}
