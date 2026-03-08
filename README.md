<p>
  
  https://github.com/user-attachments/assets/905a5c02-62a2-43d1-8a57-9d20ddc6a9e9
  
  <div align="center"> <em> Example NovaShell session. </em> </div>
</p>

# NovaShell
A custom shell implementation developed as part of the **Operating Systems** course at **Theoretical Computer Science, Jagiellonian University, Cracow**.

## Implemented Features

NovaShell is implemented incrementally across five stages. The final version includes all functionality listed below.

### 1. Core command execution
- Interactive read-eval loop with configurable prompt (`PROMPT_STR`).
- Line-oriented input handling with length guard (`MAX_LINE_LENGTH`) and syntax error reporting.
- Process spawning with `fork` + `execvp`, including `PATH` lookup.
- Parent waits for foreground child completion and reports execution failures using assignment-defined messages.

### 2. Robust input stream handling
- Prompt is shown only for terminal input (interactive mode).
- Correct handling of partial reads and multiple lines delivered in a single `read` call.
- Supports script/file input where the last line may end at EOF without a trailing newline.

### 3. Built-in commands
- Built-in dispatch table in `builtins.c` checked before external execution.
- Implemented built-ins:
  - `exit`
  - `lcd [path]` (falls back to `HOME`)
  - `lkill [ -signal ] pid` (default: `SIGTERM`)
  - `lls` (lists non-hidden entries from current directory)
- Consistent built-in error path for invalid arguments and syscall failures.

### 4. Redirections and pipelines
- Input/output redirections per command (`<`, `>`, `>>`) with open/create/truncate/append semantics.
- Redirection setup via descriptor duplication (`dup2`) before program execution.
- Multi-process pipeline execution with `pipe`, one child per command, shell waits for pipeline completion in foreground mode.
- Support for multiple pipelines in one line, executed sequentially.
- Parser edge-case handling: invalid/empty commands inside multi-command pipelines are rejected as syntax errors.

### 5. Background jobs and signal handling
- Pipelines ending with `&` execute in background without blocking the shell.
- Foreground and background execution paths are separated so interactive shell remains responsive.
- Child-reaping logic prevents zombie accumulation.
- In interactive mode, deferred notifications are printed before the next prompt for terminated background processes:
  - normal exit status
  - termination by signal
- `SIGINT` behavior aligns with shell expectations: foreground jobs are interruptible by `CTRL-C`, background jobs are isolated from terminal interrupts.

### Architecture notes
- Command parsing is provided by lexer/parser sources in `shell/input_parse/`.
- Execution logic is split into focused modules in `shell/src/` (argument parsing, builtin handling, execution preparation, executor, subprocess management, input handling, and utility/debug helpers).

## Getting Started

### Prerequisites
```txt
gcc make byacc lex
```

### Building NovaShell
```bash
cd shell
make bin/novash
```

<br/>

*Copyright (C) 2025, Kacper Orszulak*

*GNU General Public License v3.0+ (see LICENSE.txt or https://www.gnu.org/licenses/gpl-3.0.txt)*
