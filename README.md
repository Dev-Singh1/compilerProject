### README for Lisp-Inspired Compiler Project
This project is a custom compiler for a simple programming language inspired by the syntax and principles of ANSI Common Lisp. The language is designed to be expression-oriented and uses a formulaic syntax for clarity and simplicity. This repository contains the source code for the compiler, written in C, using the **mpc** (Micro Parser Combinator) library for parsing.

## Features

- **Lisp-Like Syntax**: The language supports arithmetic expressions enclosed in parentheses, such as `(+ 1 2)` or `(* 3 (- 4 2))`.
- **Recursive Evaluation**: Expressions are evaluated recursively, enabling the use of nested operations.
- **Arithmetic Operators**: Basic arithmetic operators (`+`, `-`, `*`, `/`) are supported.
- **Custom Grammar**: The grammar is defined using `mpc`, making it easy to expand the language in the future.

## Getting Started

### Prerequisites

- GCC or any C compiler
- The `mpc` library: [https://github.com/orangeduck/mpc](https://github.com/orangeduck/mpc)

### Installation

1. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/lisp-inspired-compiler.git
   cd lisp-inspired-compiler
   ```

2. Install the `mpc` library:
   Follow the installation instructions in the [mpc repository](https://github.com/orangeduck/mpc).

3. Compile the program:
   ```bash
   gcc -std=c99 -Wall -o compiler compiler.c mpc.c
   ```

### Usage

1. Run the compiled program:
   ```bash
   ./compiler
   ```

2. Enter expressions in the REPL:
   ```
   > (+ 1 2)
   output: 3
   > (* 3 (- 4 2))
   output: 6
   ```

### Example Code

#### Input
```lisp
(+ 2 (* 3 4))
```

#### Output
```
output: 14
```

## Code Overview

### `recursiveEvalHelper`

A recursive function that traverses the abstract syntax tree (AST) to evaluate expressions.

### `arithmaticHelper`

Performs arithmetic operations based on the operator provided (`+`, `-`, `*`, `/`).

### `main`

Handles user input and parses it into an AST using `mpc`. If the input is valid, it evaluates the AST and prints the result.

## Roadmap

- [ ] Add support for additional operators (e.g., modulus, power).
- [ ] Implement error handling for division by zero and invalid syntax.
- [ ] Introduce variables and assignment expressions.
- [ ] Support functions and lambda expressions.
- [ ] Create a proper standard library for common utilities.
---
