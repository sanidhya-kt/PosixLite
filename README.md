# PosixLite

<div align="center">

### A Unix-like Interactive Shell Built in C++ Using POSIX System Calls

Lightweight command-line shell supporting process management, pipelines, I/O redirection, background execution, and built-in commands.

![Language](https://img.shields.io/badge/Language-C%2B%2B-blue)
![Platform](https://img.shields.io/badge/Platform-Linux-green)
![POSIX](https://img.shields.io/badge/Standard-POSIX-orange)
![License](https://img.shields.io/badge/License-MIT-yellow)

</div>

---

## Overview

PosixLite is a Unix-like interactive shell developed in C++ that demonstrates core operating system and systems programming concepts through direct use of POSIX system calls.

The project replicates essential shell functionalities found in traditional Unix shells, including command execution, process creation, inter-process communication, input/output redirection, and background job management.

It serves as both a practical systems programming project and an educational tool for understanding how modern command-line interpreters interact with the operating system.

---

## Features

### Command Execution

* Execute external Linux commands
* Support for command arguments
* Process creation using `fork()`
* Program execution using `execvp()`

### Pipelines

* Multi-stage command pipelines using `pipe()`
* Inter-process communication between commands

Example:

```bash
ls -la | grep ".cpp" | wc -l
```

### I/O Redirection

* Input redirection (`<`)
* Output redirection (`>`)
* Append output redirection (`>>`)

Examples:

```bash
cat < input.txt
ls > output.txt
echo "Hello" >> log.txt
```

### Background Job Execution

* Run commands asynchronously using `&`
* Non-blocking process execution

Example:

```bash
sleep 10 &
```

### Built-in Commands

* `cd`
* `pwd`
* `exit`
* Additional shell utilities

### Process Management

* Child process creation
* Parent-child synchronization
* Process termination handling
* Exit status tracking

---

## System Calls Used

The shell is implemented using fundamental POSIX system calls:

| System Call            | Purpose                     |
| ---------------------- | --------------------------- |
| `fork()`               | Create child processes      |
| `execvp()`             | Execute programs            |
| `wait()` / `waitpid()` | Process synchronization     |
| `pipe()`               | Inter-process communication |
| `dup2()`               | File descriptor redirection |
| `open()`               | File handling               |
| `close()`              | File descriptor management  |
| `chdir()`              | Directory navigation        |

---

## Project Structure

```text
PosixLite/
│
├── src/                 # Source files
├── include/             # Header files
├── README.md
├── LICENSE
└── Makefile
```

---

## Installation

### Prerequisites

* Linux Operating System
* GCC/G++ Compiler
* POSIX-compliant environment

### Clone Repository

```bash
git clone https://github.com/sanidhya-kt/PosixLite.git
cd PosixLite
```

### Build

```bash
g++ *.cpp -o posixlite
```

or

```bash
make
```

### Run

```bash
./posixlite
```

---

## Example Usage

### Basic Command

```bash
posixlite> ls
```

### Pipeline

```bash
posixlite> ps aux | grep chrome
```

### Output Redirection

```bash
posixlite> ls > files.txt
```

### Background Process

```bash
posixlite> sleep 30 &
```

---

## Learning Outcomes

This project demonstrates practical understanding of:

* Operating Systems
* Linux Internals
* Process Management
* POSIX APIs
* Inter-Process Communication (IPC)
* File Descriptors
* Concurrent Execution
* Shell Design and Implementation

---

## Future Improvements

* Command history support
* Signal handling (`Ctrl+C`, `Ctrl+Z`)
* Job control commands (`jobs`, `fg`, `bg`)
* Auto-completion
* Environment variable expansion
* Shell scripting support
* Command aliases

---

## License

This project is licensed under the MIT License.

See the LICENSE file for more information.

---

## Author

**Sanidhya Kumar Tiwari**

M.Tech – Computer Science & Engineering

GitHub: https://github.com/sanidhya-kt

---

## Acknowledgments

Inspired by Unix/Linux shell architecture and POSIX standards. Developed as a systems programming project to explore process management, operating system concepts, and command-line interpreter design.
