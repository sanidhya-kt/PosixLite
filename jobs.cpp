#include "shell.h"
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <termios.h>

// Global definitions for job-related state
std::map<int, Job>        jobs;
int                       next_job_id = 1;
pid_t                     shell_pgid;
int                       shell_terminal;

void register_job(pid_t pgid, const std::string& cmdline, bool bg) {
    Job j;
    j.job_id  = next_job_id++;
    j.pgid    = pgid;
    j.cmd_line = cmdline;
    j.running = true;
    jobs[j.job_id] = j;
    if (bg)
        std::cout << "[" << j.job_id << "] " << pgid << "\n";
}

void reap_jobs() {
    for (auto it = jobs.begin(); it != jobs.end(); ) {
        int status;
        pid_t r = waitpid(-it->second.pgid, &status, WNOHANG | WUNTRACED);
        if (r > 0) {
            if (WIFSTOPPED(status)) {
                it->second.running = false;
                std::cout << "\n[" << it->first << "]+ Stopped\t" << it->second.cmd_line << "\n";
                ++it;
            } else {
                std::cout << "\n[" << it->first << "]+ Done\t\t" << it->second.cmd_line << "\n";
                it = jobs.erase(it);
            }
        } else {
            ++it;
        }
    }
}

// Wait for foreground process group
void wait_for_fg(pid_t pgid) {
    int status;
    pid_t r;
    do {
        r = waitpid(-pgid, &status, WUNTRACED);
    } while (r > 0 && !WIFSTOPPED(status) && !WIFEXITED(status) && !WIFSIGNALED(status));

    if (r > 0 && WIFSTOPPED(status)) {
        // Ctrl-Z: move to background stopped jobs list
        Job j;
        j.job_id  = next_job_id++;
        j.pgid    = pgid;
        j.cmd_line = "(fg job)";
        j.running = false;
        jobs[j.job_id] = j;
        std::cout << "\n[" << j.job_id << "]+ Stopped\t" << j.cmd_line << "\n";
    }

    // Give terminal back to shell
    tcsetpgrp(shell_terminal, shell_pgid);
}
