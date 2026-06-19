#include "shell.h"
#include <iostream>
#include <array>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>

void exec_child(const Command& cmd, int in_fd, int out_fd, pid_t pgid) {
    // Set process group
    if (pgid == 0) pgid = getpid();
    setpgid(0, pgid);

    // Restore default signal handlers
    signal(SIGINT,  SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);

    // Redirect stdin
    if (in_fd != STDIN_FILENO) {
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
    }
    // Redirect stdout
    if (out_fd != STDOUT_FILENO) {
        dup2(out_fd, STDOUT_FILENO);
        close(out_fd);
    }

    // File redirections
    if (!cmd.input_file.empty()) {
        int fd = open(cmd.input_file.c_str(), O_RDONLY);
        if (fd < 0) { perror(cmd.input_file.c_str()); exit(1); }
        dup2(fd, STDIN_FILENO); close(fd);
    }
    if (!cmd.output_file.empty()) {
        int flags = O_WRONLY | O_CREAT | (cmd.append_output ? O_APPEND : O_TRUNC);
        int fd = open(cmd.output_file.c_str(), flags, 0644);
        if (fd < 0) { perror(cmd.output_file.c_str()); exit(1); }
        dup2(fd, STDOUT_FILENO); close(fd);
    }

    // Build argv
    std::vector<char*> argv;
    for (auto& a : cmd.args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);

    execvp(argv[0], argv.data());
    // Only reached on error
    std::cerr << argv[0] << ": " << strerror(errno) << "\n";
    exit(127);
}

void run_pipeline(const std::vector<Command>& pipeline, const std::string& raw) {
    if (pipeline.empty()) return;

    // Handle pure built-in (single command, no pipes)
    if (pipeline.size() == 1 && run_builtin(pipeline[0])) return;

    bool background = pipeline.back().background;
    size_t n = pipeline.size();

    // Pipe array: pipes[i] connects stage i to stage i+1
    // pipes[i][0] = read end, pipes[i][1] = write end
    std::vector<std::array<int,2>> pipes(n - 1);
    for (size_t i = 0; i < n - 1; ++i) {
        if (pipe(pipes[i].data()) < 0) { perror("pipe"); return; }
    }

    pid_t pgid = 0; // process group for this pipeline

    for (size_t i = 0; i < n; ++i) {
        int in_fd  = (i == 0)     ? STDIN_FILENO  : pipes[i-1][0];
        int out_fd = (i == n - 1) ? STDOUT_FILENO : pipes[i][1];

        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return; }

        if (pid == 0) {
            // Child: close all pipe ends we don't use
            for (size_t j = 0; j < pipes.size(); ++j) {
                if ((int)j != (int)(i-1)) close(pipes[j][0]);
                if (j != i)              close(pipes[j][1]);
            }
            exec_child(pipeline[i], in_fd, out_fd, pgid);
        }

        // Parent: assign first child's pid as pgid
        if (pgid == 0) pgid = pid;
        setpgid(pid, pgid);
    }

    // Close all pipe ends in parent
    for (auto& p : pipes) { close(p[0]); close(p[1]); }

    if (!background) {
        // Give terminal to foreground process group
        tcsetpgrp(shell_terminal, pgid);
        wait_for_fg(pgid);
    } else {
        register_job(pgid, raw, true);
    }
}
