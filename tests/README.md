# Tests for the *shell* project

1. Before starting the tests, in the unpacked `tests` directory, run:
   ```
   ./prepare.sh
   ```

2. After finishing the tests, run:
   ```
   ./clean.sh
   ```

3. To run all tests, use `run_all.sh`. Example usage:
   ```
   ./run_all.sh ~/shell/mshell
   ```

4. A single test suite can be run using `run_suite.sh`.

5. An individual test can be run using `run_one.sh`.

The name of the directory where the tests are executed is defined in the `setup` file.  
By default, it is:
```
TEST_DIR=/tmp/$USER.ShellTest
```

