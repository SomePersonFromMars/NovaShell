# SO-shell-stage4

*Specification for the fourth stage of the project carried out as part of the Operating Systems course exercises.*

## Shell Implementation – Stage 4 (I/O redirection and piping commands)

1.  **Each program launched from the shell may have its input and output redirected.**  
    Redirections for built-in shell commands can be ignored.  
    The `redirs` list in the `command` structure describes the redirections defined for a given command.

    *   **Input redirection**: If the `flags` field in the `redir` structure satisfies the macro `IS_RIN(x)`, the newly launched program should have the file named `filename` opened as its standard input (descriptor `0`).

    *   **Output redirection**: If the `flags` field satisfies the macro `IS_ROUT(x)` or `IS_RAPPEND(x)`, the newly launched program should have the file named `filename` opened as its standard output (descriptor `1`).  
        Additionally:
        *   If the flags satisfy `IS_RAPPEND(x)`, the file should be opened in append mode (`O_APPEND`).
        *   Otherwise, the file content should be truncated (`O_TRUNC`).
        *   If the output file does not exist, it should be created.

    **Error handling**:

    *   File does not exist → print to stderr: `(filename): no such file or directory\n`
    *   Insufficient permissions → print to stderr: `(filename): permission denied\n`

    You may assume that each command’s redirection list contains **at most one input and one output redirection**.

2.  **Commands in a single line may be connected using pipes (`|`).**  
    A sequence of such commands is called a **pipeline**.  
    If the pipeline contains more than one command, you may assume none of them are built-in shell commands.  
    All commands in the pipeline should be executed, each in a separate child process.  
    The commands should be connected via pipes so that the output of command `k` becomes the input of command `k+1`.  
    The shell should suspend its operation until **all** processes in the pipeline have finished.  
    If a command has defined I/O redirection(s), they take precedence over pipes.

3.  **Multiple pipelines may be defined in a single line**, separated by `;` (or `&`).  
    These should be executed sequentially: execute the first, wait for all its processes to finish, then execute the next, and so on.

**Note**:  
The parser accepts lines containing empty commands.  
For example, the line `ls | sort ; ls |  | wc` will be parsed correctly.  
If a pipeline of length ≥ 2 contains an empty command, the entire line should be ignored and a **syntax error** should be reported.

***

**Example session** (compare results with Bash):

    $ ls > a
    $ ls >> a
    $ ls >> b
    $ cat < a
    $ cat < a > b
    $ cat < a >> b
    $ ls | sort
    ...
    $ ls | cat | sort | wc
    ...
    $ ls > /dev/null | wc
          0       0       0
    $ ls | wc < /dev/null
          0       0       0
    $ ls | sleep 10 | ls | sort # shell should pause for 10s
    ...
    $ ls /etc | grep "a" > a ; ls /home | sort > b
    ...
    $ sleep 5 ; sleep 5; sleep 5; echo yawn # should take 15s to complete
    yawn

***

**Syscall checklist**: `open`, `close`, `pipe`, `dup/dup2/fcntl`

**Tests** have been extended with **suite 3**, covering redirections and piping.
