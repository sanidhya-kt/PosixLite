#include "shell.h"
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <termios.h>

bool run_builtin(const Command& cmd) {
    if (cmd.args.empty()) return true;
    const std::string& name = cmd.args[0];

    // cd
    if (name == "cd") {
        const char* dir = (cmd.args.size() > 1) ? cmd.args[1].c_str() : getenv("HOME");
        if (!dir) dir = "/";
        if (chdir(dir) != 0)
            perror("cd");
        return true;
    }

    // exit
    if (name == "exit") {
        std::cout << "Goodbye!\n";
        exit(0);
    }

    // history
    if (name == "history") {
        for (size_t i = 0; i < history.size(); ++i)
            std::cout << "  " << i + 1 << "  " << history[i] << "\n";
        return true;
    }

    // jobs
    if (name == "jobs") {
        reap_jobs();
        for (auto& [id, j] : jobs)
            std::cout << "[" << id << "] " << (j.running ? "Running" : "Stopped")
                      << "\t\t" << j.cmd_line << "\n";
        return true;
    }

    // fg [job_id]
    if (name == "fg") {
        int jid = -1;
        if (cmd.args.size() > 1) jid = std::stoi(cmd.args[1]);
        else if (!jobs.empty())  jid = jobs.rbegin()->first;

        if (jid == -1 || jobs.find(jid) == jobs.end()) {
            std::cerr << "fg: no such job\n"; return true;
        }
        Job& j = jobs[jid];
        std::cout << j.cmd_line << "\n";
        tcsetpgrp(shell_terminal, j.pgid);
        kill(-j.pgid, SIGCONT);
        wait_for_fg(j.pgid);
        jobs.erase(jid);
        return true;
    }

    // bg [job_id]
    if (name == "bg") {
        int jid = -1;
        if (cmd.args.size() > 1) jid = std::stoi(cmd.args[1]);
        else if (!jobs.empty())  jid = jobs.rbegin()->first;

        if (jid == -1 || jobs.find(jid) == jobs.end()) {
            std::cerr << "bg: no such job\n"; return true;
        }
        Job& j = jobs[jid];
        j.running = true;
        kill(-j.pgid, SIGCONT);
        std::cout << "[" << jid << "] " << j.cmd_line << " &\n";
        return true;
    }

    return false; // not a built-in
}
