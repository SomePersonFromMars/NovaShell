### **SO-shell – Stage 5**

*Specification for the fifth stage of the project carried out as part of Operating Systems exercises.*

***

## **Shell Implementation – Stage 5 (Background Process Execution and Signal Handling)**

1.  All commands in a line where the last non-whitespace character is `&` should be executed in the background. The shell process should start them but not wait for them to finish. You can assume that if commands in a pipeline are to be run in the background, none of them are shell built-ins (i.e., all commands can be treated as programs). To check whether a pipeline should be executed in the background, use the `LINBACKGROUND` flag in the `flags` field of the `pipeline` structure.

2.  The shell process should check the status of terminated child processes.
    *   In non-prompt mode, these statuses can be ignored.
    *   In prompt mode, information about terminated background processes should be printed **before the next prompt** in the following format:

    <!---->

        Background process (pid) terminated. (exited with status (status))
        Background process (pid) terminated. (killed by signal (signo))

3.  Make sure not to leave zombie processes. In prompt mode, consider the case when the prompt is not printed for a long time (e.g., `sleep 10 | sleep 10 & sleep 1000000`).  
    It is acceptable to store only a fixed number of termination statuses (a compile-time constant) until the prompt is printed, and forget the rest.

4.  **CTRL-C** sends the `SIGINT` signal to all processes in the foreground process group. Ensure that this signal does not reach background processes. Background processes should have the default handler for this signal registered.

**Syscall checklist:** `sigaction`, `sigprocmask`, `sigsuspend`, `setsid`

***

### **Example Session**

    $ sleep 20 | sleep 21 | sleep 22 &
    $ ps ax | grep sleep
     580 ? 0:00 sleep TERM=xterm
     581 ? 0:00 sleep TERM=xterm
     582 ? 0:00 sleep TERM=xterm
     586 p1 0:00 grep sleep
    $
    Background process 580 terminated. (exited with status 0)
    Background process 581 terminated. (exited with status 0)
    Background process 582 terminated. (exited with status 0)
    $ sleep 10 | sleep 10 | sleep 10 &
    $ sleep 30    # (this sleep should not be interrupted earlier than after 30s)
    Background process 587 terminated. (exited with status 0)
    Background process 588 terminated. (exited with status 0)
    Background process 589 terminated. (exited with status 0)
    $ sleep 10 | sleep 10 | sleep 10 &
    $ sleep 1000  # (from another terminal, check that there are no zombie processes after at least 10s but before `sleep 1000` ends)
    (CTRL-C) (should interrupt sleep 1000)
    Background process 591 terminated. (exited with status 0)
    Background process 592 terminated. (exited with status 0)
    Background process 593 terminated. (exited with status 0)
    $ sleep 1000 &
    $ sleep 10
    (CTRL-C) (should interrupt sleep 10 but not sleep 1000)
    $ ps ax | grep sleep
    595 ? 0:00 sleep TERM=xterm
    598 p1 0:00 grep sleep
    $ /bin/kill -SIGINT 595
    Background process 595 terminated. (killed by signal 2)
    $ exit

***
