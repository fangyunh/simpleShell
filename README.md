# Simple Shell
This is a simple shell written in C.

## Shell Specification
### Built-in Commands:
  - exit: exit the program properly (with exit status 0). Before exiting, the shell must print the message 'Bye...' on stderr'
  - cd & 
  - set: set value to a simple variable (a~z). "Error: invalid variable name"
### Output Redirection >:
  The command located right before > is to write its output to the specified file instead of the shell’s standard output. 
### Piping |
  Allows multiple commands to be connected to one another in order to build more complex jobs. 
  We assume that there can be up to three pipe signs on the same command line to connect up to four commands to each other.
  The completion message must display the exit value of each process composing the job separately.
### Error Management
  1. Failure of library functions： 
    e.g malloc() and fork() fail, the shell is allowed to terminate its execution right away. Use perror() to report.
  2. Errors during the parsing of the command line:
    If an incorrect command line is supplied by the user, the shell should only display an error message on stderr, discard the invalid input and wait for a new input, but it should not die. If a command line contains more than one parsing error, the leftmost one should be detected first and reported.
    e.g "Error: too many process arguments", "Error: missing command", "Error: no output file", "Error: cannot open output file", "Error: mislocated output redirection"
  3. Errors during the launching of the job:
    when the shell actually tries to execute the parsed command line as a job. Like parsing errors, launching errors should not cause the shell to die. 
    e.g "Error: cannot cd into directory", "Error: command not found", 
### Combined Redirection >&, |&:
  The two sequences of meta-characters >& and |& indicate that the standard error should be redirected as well as the standard output, either to the specified file or to the command following the pipe.


