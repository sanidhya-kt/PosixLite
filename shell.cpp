#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <sstream>
#include <map>
#include <algorithm>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

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
//  Globals
// ─────────────────────────────────────────────

static std::vector<std::string>  history;
static std::map<int, Job>        jobs;       // job_id -> Job
static int                       next_job_id = 1;
static pid_t                     shell_pgid;
static int                       shell_terminal;

// ─────────────────────────────────────────────
//  Tokeniser
// ─────────────────────────────────────────────

// Split raw input into tokens, honouring single and double quotes
static std::vector<std::string> tokenise(const std::string& line) {
    std::vector<std::string> tokens;
    std::string tok;
    bool in_single = false, in_double = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (c == '\'' && !in_double) { in_single = !in_single; continue; }
        if (c == '"'  && !in_single) { in_double = !in_double; continue; }

        if (!in_single && !in_double) {
            if (std::isspace(c)) {
                if (!tok.empty()) { tokens.push_back(tok); tok.clear(); }
                continue;
            }
            // Keep special chars as separate tokens
            if (c == '|' || c == '<' || c == '>' || c == '&') {
                if (!tok.empty()) { tokens.push_back(tok); tok.clear(); }
                // Check '>>'
                if (c == '>' && i + 1 < line.size() && line[i+1] == '>') {
                    tokens.push_back(">>"); ++i; continue;
                }
                tokens.push_back(std::string(1, c));
                continue;
            }
        }
        tok += c;
    }
    if (!tok.empty()) tokens.push_back(tok);
    return tokens;
}

// ─────────────────────────────────────────────
//  Parser  (tokens -> pipeline of Commands)
// ─────────────────────────────────────────────

// Returns list of commands separated by '|'.
// Sets background flag on last command if trailing '&' found.
static std::vector<Command> parse(const std::vector<std::string>& tokens) {
    std::vector<Command> pipeline;
    Command cur;

    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& t = tokens[i];

        if (t == "|") {
            pipeline.push_back(cur);
            cur = Command{};
        } else if (t == "<") {
            if (i + 1 < tokens.size()) cur.input_file  = tokens[++i];
        } else if (t == ">>") {
            if (i + 1 < tokens.size()) { cur.output_file = tokens[++i]; cur.append_output = true; }
        } else if (t == ">") {
            if (i + 1 < tokens.size()) cur.output_file = tokens[++i];
        } else if (t == "&") {
            cur.background = true;
        } else {
            cur.args.push_back(t);
        }
    }
    pipeline.push_back(cur);
    return pipeline;
}

// ─────────────────────────────────────────────
//  Job Control helpers
// ─────────────────────────────────────────────

static void register_job(pid_t pgid, const std::string& cmdline, bool bg) {
    Job j;
    j.job_id  = next_job_id++;
    j.pgid    = pgid;
    j.cmd_line = cmdline;
    j.running = true;
    jobs[j.job_id] = j;
    if (bg)
        std::cout << "[" << j.job_id << "] " << pgid << "\n";
}

static void reap_jobs() {
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
static void wait_for_fg(pid_t pgid) {
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

// ─────────────────────────────────────────────
//  Built-in commands
// ─────────────────────────────────────────────

static bool run_builtin(const Command& cmd) {
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

// ─────────────────────────────────────────────
//  Execute a single command in a child process
// ─────────────────────────────────────────────

static void exec_child(const Command& cmd, int in_fd, int out_fd, pid_t pgid) {
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

// ─────────────────────────────────────────────
//  Run a pipeline
// ─────────────────────────────────────────────

static void run_pipeline(const std::vector<Command>& pipeline, const std::string& raw) {
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

// ─────────────────────────────────────────────
//  Prompt helpers
// ─────────────────────────────────────────────

static std::string get_prompt() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) strcpy(cwd, "?");
    std::string path(cwd);
    // Shorten home directory to ~
    const char* home = getenv("HOME");
    if (home && path.find(home) == 0)
        path = "~" + path.substr(strlen(home));
    return "\033[1;32mmsh\033[0m:\033[1;34m" + path + "\033[0m$ ";
}

// ─────────────────────────────────────────────
//  Main REPL
// ─────────────────────────────────────────────

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
            int n = std::stoi(line.substr(1));
            if (n < 1 || n > (int)history.size()) { std::cerr << "No such history entry.\n"; continue; }
            line = history[n - 1];
            std::cout << line << "\n";
        }

        history.push_back(line);

        auto tokens   = tokenise(line);
        auto pipeline = parse(tokens);
        run_pipeline(pipeline, line);
    }

    return 0;
}
