# SO-shell-stage2  
*Specification for the second stage of the project carried out as part of the Operating Systems course exercises.*

## Shell Implementation – Stage 2 (input from file)

Changes compared to Stage 1:

1. The prompt is printed to `STDOUT` **only if** `STDIN` corresponds to a special character device (see `man fstat`).
2. When reading a line from `STDIN`, you must account for the possibility that **multiple lines** may be read at once, and that the **last line may be incomplete**. You must handle the case where only part of a line is read in a single read operation.
3. You still only need to execute the **first command** from each of the read lines.
4. Remember that the **last input line may end with end of file** rather than a newline character.

Scripts and the first set of tests are located in the `test` directory. Instructions for running the tests are in the `test/README.md` file.

Each test from the first set is run in two modes:
- In the first mode, the entire file is provided as input at once.
- In the second mode, the input is passed through the `splitter` program and piped to the shell. `splitter` rewrites the input to output, introducing breaks at pseudo-random moments.
