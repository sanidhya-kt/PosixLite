# 🖥️ PosixLite

A lightweight, robust, and feature-rich Unix shell implemented in C++ (C++17). `msh` replicates key capabilities of standard POSIX shells like Bash, including command piping, input/output redirection, interactive job control (foreground/background execution), history management, and a clean, colorized path-shortening prompt.

---

## 🚀 Features

*   **Pipelining (`|`)**: Seamlessly chain multiple commands in a pipeline (e.g., `cat file.txt | grep "error" | wc -l`).
*   **I/O Redirection**:
    *   `<` : Redirect input from a file.
    *   `>` : Redirect output to a file (overwrite).
    *   `>>` : Append output to a file.
*   **Job Control**:
    *   Execute commands in the background using `&` at the end of the command.
    *   Suspend foreground jobs using `Ctrl + Z`.
    *   List current active jobs with `jobs`.
    *   Bring background or stopped jobs to the foreground with `fg [job_id]`.
    *   Resume stopped jobs in the background with `bg [job_id]`.
*   **Command History**:
    *   Type `history` to view previously run commands.
    *   Execute the last command using `!!`.
    *   Execute the $n$-th command from history using `!n`.
*   **Dynamic Prompt**:
    *   Colorful, responsive prompt showing `msh:<current_working_directory>$`.
    *   Automatically collapses the user's home directory path to `~` (e.g., `/Users/username/Projects` becomes `~/Projects`).
*   **Signal Handling**:
    *   Ignores interrupting and terminal control signals (`Ctrl+C`, `Ctrl+\`, `Ctrl+Z`) in the main shell session, but routes them correctly to active foreground child processes.

---

## 📁 Project Structure

| File | Description |
| :--- | :--- |
| **[shell.h](file:///Users/sanidhyakumartiwari/Desktop/Unix%20Shell%20/shell.h)** | Header file containing data structures (`Command`, `Job`), global states, and function prototypes. |
| **[main.cpp](file:///Users/sanidhyakumartiwari/Desktop/Unix%20Shell%20/main.cpp)** | Shell entry point, initialisation, REPL loop, prompt rendering, and history expansion. |
| **[tokenizer.cpp](file:///Users/sanidhyakumartiwari/Desktop/Unix%20Shell%20/tokenizer.cpp)** | Lexical analyzer that splits raw command input lines into tokens, handling whitespace and special operators. |
| **[parser.cpp](file:///Users/sanidhyakumartiwari/Desktop/Unix%20Shell%20/parser.cpp)** | Parses tokenized inputs into executable pipeline segments, recognizing redirection files and background commands. |
| **[executor.cpp](file:///Users/sanidhyakumartiwari/Desktop/Unix%20Shell%20/executor.cpp)** | Implements child process forking, environment setups, pipe chaining, file descriptor overrides, and executable routing via `execvp`. |
| **[builtins.cpp](file:///Users/sanidhyakumartiwari/Desktop/Unix%20Shell%20/builtins.cpp)** | Logic for shell built-ins: `cd`, `exit`, `history`, `jobs`, `fg`, and `bg`. |
| **[jobs.cpp](file:///Users/sanidhyakumartiwari/Desktop/Unix%20Shell%20/jobs.cpp)** | Job tracking table, background job registration, child status reaping (`waitpid`), and terminal foreground handovers. |
| **[Makefile](file:///Users/sanidhyakumartiwari/Desktop/Unix%20Shell%20/Makefile)** | Compilation directives supporting compiler optimization (`-O2`), warnings, and dependency checks. |

---

## 🛠️ Build & Run Instructions

### Prerequisites
*   A C++ compiler supporting C++17 (e.g., `g++` or `clang++`).
*   `make` build utility.

### Compile
To build the shell executable, run the following command in the project directory:
```bash
make
```
This compiles the source files and generates an executable named `msh`.

### Run
To launch the shell:
```bash
./msh
```

### Clean up
To remove compiled object files and the executable:
```bash
make clean
```

---

## 💡 Usage Examples

### 1. Basic Commands and Navigation
```bash
msh:~$ cd Desktop
msh:~/Desktop$ ls -la
```

### 2. Piping and Redirection
Write process lists to a file, filter it, and view it:
```bash
msh:~$ ps aux > processes.txt
msh:~$ grep "chrome" < processes.txt | wc -l >> chrome_count.txt
msh:~$ cat chrome_count.txt
```

### 3. Background Execution
Run a sleep command in the background:
```bash
msh:~$ sleep 100 &
[1] 84210
```

### 4. Job Control
Suspend a running command and resume it:
```bash
msh:~$ sleep 60
^Z
[1]+ Stopped    (fg job)
msh:~$ jobs
[1] Stopped    (fg job)
msh:~$ bg 1
[1] (fg job) &
msh:~$ jobs
[1] Running    (fg job)
msh:~$ fg 1
(fg job)
```

### 5. History Expansion
```bash
msh:~$ echo "Hello World"
Hello World
msh:~$ !!
echo "Hello World"
Hello World
msh:~$ history
  1  echo "Hello World"
  2  echo "Hello World"
msh:~$ !1
echo "Hello World"
Hello World
```

---

## 📄 License

This project is licensed under the terms of the [LICENSE](file:///Users/sanidhyakumartiwari/Desktop/Unix%20Shell%20/LICENSE) file included in this repository.

Created By - SANIDHYA KUMAR TIWARI
