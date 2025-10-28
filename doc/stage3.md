# SO-shell-stage3  
*Specification for the third stage of the project carried out as part of the Operating Systems course exercises.*

## Shell Implementation – Stage 3 (Shell Built-in Commands)

Before executing a command as an external program, we check whether it is a shell built-in command. Built-in commands are listed in the `builtins_table` in the `builtins.c` file. Each entry consists of the command name and a pointer to the function that implements the command. The function takes an argument array similar to `argv`. The following commands must be implemented:

- `exit` – terminates the shell process.  
- `lcd [path]` – changes the current working directory to the specified path. If the command is called without an argument, the directory should be changed to the value of the `HOME` environment variable.  
- `lkill [ -signal_number ] pid` – sends the signal `signal_number` to the process/group with the given `pid`. The default signal number is `SIGTERM`.  
- `lls` – prints the names of files in the current directory (similar to `ls -1` without any options). The directory contents should be printed in the order returned by the system. Names starting with `.` should be ignored.

If there are any problems executing a command (incorrect number or format of arguments, syscall failure, etc.), the following message should be printed to STDERR:
```
Builtin 'command_name' error.
```

### Example session:
```
sleep 10000 &
sleep 10001 &
./mshell
$ pwd
/home/dude
$ lcd /etc
$ pwd
/etc
$ lcd ../home
$ pwd
/home
$ lls
bin
ast
dude
$ lcd /
$ lls
usr
boot.cfg
bin
boot
boot_monitor
dev
etc
home
lib
libexec
mnt
proc
root
sbin
sys
tmp
var
$ lcd
$ pwd
/home/dude
$ cd ../../usr/src/etc
$ pwd
/usr/src/etc
$ ps
 PID TTY  TIME CMD
...
 2194  p1  0:00 sleep TERM=xterm
 2195  p1  0:00 sleep TERM=xterm
...
$ lkill -15 2194
$ ps
  PID TTY  TIME CMD
...
 2195  p1  0:00 sleep TERM=xterm
...
$ lkill 2195
$ ps
  PID TTY  TIME CMD
  ...
$ lcd /CraneJacksonsFountainStreetTheatre
Builtin lcd error.
$ lkill
Builtin lkill error.
$ exit
```

### Syscall checklist:
- `exit`
- `chdir`
- `kill`
- `readdir`
- `opendir` / `fdopendir`
- `closedir`

The test suite has been extended with set 2, which includes shell built-in command execution.
