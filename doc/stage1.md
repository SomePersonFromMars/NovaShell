## **OS Shell – Stage 1**

*Specification for the first stage of the project carried out as part of the Operating Systems course exercises.*

### **Shell Implementation – Stage 1**

In the first stage, for each input line, only the **first** command provided needs to be executed. Input is assumed to come from the console/terminal.

Values of the macros used below are defined in the file `include/config.h`.

#### **Main shell loop:**

1. Print the prompt to STDOUT.
2. Read a line from STDIN.
3. Parse commands from the line (it is recommended to use the provided parser).
4. If parsing is successful, execute the **first** parsed command as a program in a child process, if the command is not empty. The shell process should wait for the child process to finish.

#### **Error handling:**

- If the program specified in the command does not exist, the child process should print to STDERR:
  ```
  [program name]: no such file or directory\n
  ```
- If the program exists but cannot be executed due to lack of execution permissions for the current user, print:
  ```
  [program name]: permission denied\n
  ```
- In other cases where the program fails to execute, print:
  ```
  [program name]: exec error\n
  ```
- In all the above failure cases, the child process should exit with the value of the macro `EXEC_FAILURE` (e.g., `exit(EXEC_FAILURE)`).

5. If parsing fails, print the error message (value of the macro `SYNTAX_ERROR_STR`) to STDERR, ending with a newline `\n`.

The main loop should terminate when end of file is observed on STDIN.

#### **Temporary assumptions:**

- Lines arrive in full (one `read` from STDIN returns one line).

#### **Requirements:**

- Line length must not exceed the value of the macro `MAX_LINE_LENGTH`. Lines longer than this should be treated as syntax errors and not passed to the parser.
- Only `read` may be used to read from standard input.
- When searching for the program to execute, directories from the `PATH` environment variable must be considered (see `execvp`).
- The default prompt is defined by the macro `PROMPT_STR`.

#### **Syscall checklist:**

The following syscalls should appear in the code:
- `read`, `fork`, `exec`, `wait/waitpid`

#### **Additional notes:**

- The file `utils.c` contains simple functions that may be useful for debugging.
- Header and source files (except `mshell.c`) may be extended or modified in future stages. Therefore, it is best to place custom code in `mshell.c` or in newly created files.

#### **Example session** (session ends with CTRL-D, which causes "end of file" on STDIN):

```
> ./mshell
$ ls /home
ast bin dude
$ sleep 3
$ ls -las /home
total 6
total 40
8 drwxrwxrwx 5 bin bin 320 Oct 5 15:13 .
8 drwxr-xr-x 17 root operator 1408 Oct 5 13:39 ..
8 drwxr-xr-x 2 ast operator 320 Feb 15 2013 ast
8 drwxr-xr-x 2 bin operator 320 Feb 15 2013 bin
8 drwxr-xr-x 4 dude dude 1024 Oct 12 19:37 dude
$ 
$ cat /etc/version
3.2.1, SVN revision , generated Fri Feb 15 11:34:15 GMT 2013
$ cat /etc/version /etc/version
3.2.1, SVN revision , generated Fri Feb 15 11:34:15 GMT 2013
3.2.1, SVN revision , generated Fri Feb 15 11:34:15 GMT 2013
$ iamthewalrus
iamthewalrus: no such file or directory
$ /etc/passwd
/etc/passwd: permission denied
$ CTRL-D
```
