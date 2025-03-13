### README 

# WORK IN PROGRESS

# Lisp-Inspired Compiler (Systems Programming Project)

This project is a custom compiler for a simple programming language inspired by the syntax and principles of ANSI Common Lisp. The language is designed to be expression-oriented and supports a rich set of features, including functional programming constructs, dynamic scoping, and various primitive operations.

## Features

- **Extended Syntax**:
  - Added support for floating-point numbers, strings, and comments.
  - Included quoted expressions (`qexpr`) for evaluating blocks without immediate execution.
- **Functional Programming**:
  - Support for anonymous functions (lambdas) and user-defined functions.
  - Functional primitives like `head`, `tail`, `join`, and `cons`.
- **Arithmetic and Logical Operations**:
  - Basic arithmetic (`+`, `-`, `*`, `/`, `mod`).
  - Comparison operators (`>`, `>=`, `<`, `<=`, `==`, `!=`).
  - Logical operators (`and`, `or`, `not`).
- **Environment and Scope**:
  - Local (`let`) and global (`static`) variable declaration.
  - Recursive evaluation with dynamic scoping.
- **Control Flow**:
  - Conditional execution with the `if` operator.
- **Error Handling**:
  - Graceful error reporting for invalid input or runtime issues.
- **Interactivity**:
  - REPL (Read-Eval-Print Loop) with history support.
  - Ability to load and execute scripts from files.

## Getting Started

### Prerequisites

- GCC or any C compiler supporting the C99 standard.
- The `mpc` library: [https://github.com/orangeduck/mpc](https://github.com/orangeduck/mpc).
- `editline` library for command-line editing and history (optional, for UNIX systems).

### Installation

1. Clone this repository:
   ```bash
   git clone https://github.com/Dev-Singh1/compilerProject.git
   cd compilerProject
   ```

2. Install dependencies:
   Follow the installation instructions for the [mpc library](https://github.com/orangeduck/mpc) and `editline`.

3. Compile the program:
   ```bash
   gcc -std=c99 -Wall -o compiler main.c mpc.c -ledit -lm
   ```

### Usage

1. Run the compiler interactively:
   ```bash
   ./compiler
   ```
   Use the REPL to enter expressions and view results.

2. Load and execute a script file:
   ```bash
   ./compiler script_file.lsp
   ```

### Example

#### Input (REPL):
```lisp
> (+ 2 3)
> (if (> 5 2) { (* 3 4) } { (/ 10 2) })
> (\\ {x} {+ x 1}) 5
```

#### Output:
```lisp
5
12
6
```

## Code Structure

- **Core Types**:
  - `customType`: Represents all expressions, numbers, strings, and functions.
  - `env`: Represents the dynamic environment for scope management.
- **Built-in Functions**:
  - Arithmetic: `+`, `-`, `*`, `/`, `%`.
  - Logical: `and`, `or`, `not`.
  - Structural: `head`, `tail`, `join`, `cons`.
  - Special: `if`, `def`, `\\` (lambda), `let`, `static`.
- **Parser and AST**:
  - Tokenizes and parses input using the `mpc` library.
  - Converts input into Abstract Syntax Trees (ASTs).

## Roadmap

- [ ] Implement loops and iteration constructs.
- [ ] Add file I/O operations.
- [ ] Improve error handling
- [ ] Enhance REPL with debugging tools.

