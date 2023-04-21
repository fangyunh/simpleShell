# Report

Authors: Yunhua Fang, Zhuoheng Li

## Summary

This program, 'sshell', can perform some simple and basic functions of classical shell such as bash. It can achieve three built-in functions: cd, pwd, and exit. Also, it has ability to handle commands from $PATH by proper execvp(). When usrs input commands, they can use piping by character "|" and output redirection by character ">". If they want to collect error messages in a file, the shell support meta character to redirect output in file. The simple shell supports users to create their environmental variables by character "$".  

## Structure

The structure of the sshell is based on the reference provided by the Prof. Joel Porquet-Lupine. The reference create a infinite loop to make sure that the program will not exit after a command is completed. It inducts users to enter command and recieves input from them.

After recieves the command, the first thing is to check does user enters something useful and correct. The simple shell will check is command empty and is there exist mislocated output redirection sign. The goal of them is to make sure that the commands does not have obvious errors.

Then, the shell parse the command into a struct called "inputCmd". It is a linked-list structure to divide and store commands by spaces. Linked-list data structure helps other functions to interpret commands easily. The shell later concatenate linked-list strings to form a new command after replacement of environmental variables. The string form is efficient to some functions. We also have a function to parse commands in array because the data structure is proper to some functions. In conclusion, we try to use proper data structure to handle proper functions.

Since we have different kinds of function to implement, the shell create a struct called "cmdType" with an initialization function. It can determine the type of command and sent it to the correct place to execute.

Next, the program allpies a "if else" structure to determine the type of commands based on the "cmdType" struct. Commands will be execute in their functions.

Finally, we free allocated spaces and start a new turn of the infinite loop.

## Implementation

### Built-in Functions

The built-in functions part has four parts: cd, pwd, exit, and set. The exit is provided by reference so it will be checked at first. Then, the builtinFunctions() determine the commands according to their first element in the linked-list. Each of them will return the status about execution.

#### cd
The function simply called chdir() to access different categories.

#### pwd



#### set
The set built-in function supports users to store 26 different values in environmental varaibles. Firstly, it will check the name of environmental variables is correct. Then, it calculates the position of the variable in global array and store the value in the correct position.

### Output Redirection

Since the shell support a meta character '&' to redirect stderr in a file. The function will check does the command contains '&' at first and than divde components by '>' or '>&'. Because the shell only support one redirection sign, the first element after division is output and the second one is file.

After parsing the command, the function will create a new process by fork() and change the file description in FD table by dup2(). After running the execution function the command, the process will quit and return the status of execution.

### Piping


### Regular Functions and $PATH Commands

By considering many functions will call execvp() to execute commands, we build a function called execRegCmd() to recieve instructions and create a process for the execution of instructions. Hence, other parts can easily execute their commands.

### Extra Features

#### Combined Redirection

The combined redirection required to change the file description in FD table. It influence the standrd output of the program. Hence, the shell has three global varaibles to store the status of stanrd input, output, and error. After the file records the error messages from commands, the functions who replce the file description have responsibility to return the regular status based on three global variables.

#### Environmental Global Variables

Function repEnvirVarStat() will detect and replace the exist global variables in command by searching character '$'. Since the command is stored in linked-list, the function can easily replace the original content to the content stored in global array. The storing part is introduced in set built-in function.

### Test and Reference

We first use terster.sh script to test basic cases. After pass all of them, we start to manually typing commands provided by the instructions. Some of them may triggur problems in our code. Also, we self-design some edge cases to test the output. String handling is a complex and difficult part in C. So we design some strange commands to see if there exists segmentation fault inour codes.

By handling string, we check and refer many things from w3schools: https://www.w3schools.com/c/c_strings.php

