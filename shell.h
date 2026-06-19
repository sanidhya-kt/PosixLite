#ifndef SHELL_H
#define SHELL_H

#include <string>
#include <vector>
#include <map>
#include <sys/types.h>

// ─────────────────────────────────────────────
//  Data Structures
// ─────────────────────────────────────────────

struct Command {
    std::vector<std::string> args;   // argv[0] is the program name
    std::string input_file;          // < file
    std::string output_file;         // > file
    bool append_output = false;      // >> file
    bool background = false;         // trailing &
};

struct Job {
    int job_id;
    pid_t pgid;
    std::string cmd_line;
    bool running;   // true = running, false = stopped
};

// ─────────────────────────────────────────────
//  Globals (extern declarations)
// ─────────────────────────────────────────────

extern std::vector<std::string>  history;
extern std::map<int, Job>        jobs;       // job_id -> Job
extern int                       next_job_id;
extern pid_t                     shell_pgid;
extern int                       shell_terminal;

// ─────────────────────────────────────────────
//  Function Prototypes
// ─────────────────────────────────────────────

std::vector<std::string> tokenise(const std::string& line);
std::vector<Command> parse(const std::vector<std::string>& tokens);

void register_job(pid_t pgid, const std::string& cmdline, bool bg);
void reap_jobs();
void wait_for_fg(pid_t pgid);

bool run_builtin(const Command& cmd);
void exec_child(const Command& cmd, int in_fd, int out_fd, pid_t pgid);
void run_pipeline(const std::vector<Command>& pipeline, const std::string& raw);

std::string get_prompt();

#endif // SHELL_H
