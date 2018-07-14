# Cog

> Cog is under active development. Not all documented language features have been implemented.

Cog is a C-like systems programming language intented to maximize modular design with simple features for enforcing and testing correctness.

1. [Examples](#Examples)
2. [Benchmarks](#Benchmarks)
   1. [Compile Time](#Compile-Time)
   2. [Run Time](#Run-Time)
4. [Syntax](#Syntax)
   1. [Program Structure](#Program-Structure)
   2. [Program Specification](#Program-Specification)
      * [Comments](#Comments)
      * [Variables](#Variables)
      * [Expressions](#Expressions)
      * [Implicit Casting Rules](#Implicit-Casting-Rules)
      * [If Statements](#If-Statements)
      * [While Loops](#While-Loops)
      * [Functions](#Functions)
      * [Inline Assembly](#Inline-Assembly)
      * [Constraints](#Constraints)
   3. [Source Files](#Source-Files)
      * [Structures](#Structures)
   4. [Interface Files](#Interface-Files)
      * [Interfaces](#Interfaces)
   5. [Test Files](#Test-Files)
      * [Mocks](#Mocks)
      * [Tests](#Tests)

## Examples

```

```

## Benchmarks

### Compile Time

> No compile time benchmarks have been run.

### Run Time

> No run time benchmarks have been run.

## Syntax

### Program Structure

> This feature is not yet implemented.

Cog programs are divided up into three file types that serve different purposes. `.ifc` interface files specify limited interfaces designed for specific purposes. These represent the only way that programs may be linked. This ensures that any or every piece of a code base may be swapped out for something else while guaranteeing correct functionality. `.cog` source files specify implementations with for interfaces and represent the primary content of a program. `.tst` test files specify an array of tests on interfaces, structures, and functions that check their implementation.

Dependencies are limited by file type.
* `.cog` sources my only import `.ifc` interfaces
* `.ifc` interfaces may only import other `.ifc` interfaces
* `.tst` tests may import any number of any file type.

Each source file implements exactly one structure and its associated member functions. Each interface file may implement any number of interfaces and non-member functions. Each interface file is given its own namespace with the same name as the file. Import directives targeted at interface files may either import the whole namespace or specific interfaces and/or functions. Import directives targeted at source files may only import the whole file.

```
import path/to/myinterfacesfile
import myinterface as ifc from path/to/myinterfacesfile
import path/to/mysourcefile
```

### Program Specification

These represent language constructs that may be found in any of the file types.

#### Comments

Cog implements C-like comments.
```
// This is a line comment
/*
This is
a block
comment
*/
```

#### Variables

> Primitives except for character encodings have been implemented.
> Variable declaration currently limited to one variable with no inline assignment.
> Pointers and Arrays have not yet been implemented.

The usual primitive types are supported with added support for fixed point numbers and arbitrary character encodings. Here, <n> represents the bitwidth of the type and <m> represents the power of 2 exponent. For example, int32 is a 32 bit signed integer. fixed32e-5 is a 32 bit fixed point decimal with a five bit fractional precision. int<n>, uint<n>, fixed<n>e<m>, and ufixed<n>e<m> all support arbitrary values for <n> and <m>. However, float<n> is limited to bitwidths of 16, 32, 64, or 128.

```
bool
int<n>
uint<n>
fixed<n>e<m>
ufixed<n>e<m>
float<n>
```
Both integer and decimal constants are encoded as fixed point numbers with arbitrary precision. This means that all constant expressions will evaluate without any rounding errors. Once all constant expressions are evaluated, the results are implicitly cast to and stored as the type that they are assigned in the program.

Variables are declared and assigned using a C-like syntax.
```
int32 myInt;
float32 myFloat = 3.2e5;
fixed32 v0 = 3.8e10, v1 = 2.4, v2 = 5;
```

#### Expressions

#### Implicit Casting Rules

#### If Statements

If statements have the usual C syntax and behave as one might expect. Spacing doesn't matter, but the conditions must be wrapped in parenthesis. While single line statements in the body of an if don't need curly brackets, multiline statement blocks do.
```
if (x < 5) {
  // Do things
} else if (x > 20) {
  // Do other things
} else
  // Do one statement
```

#### While loops

While loops also have the usual C syntax. Same rules as if statements apply.
```
while (i < 100) {
  // Do things
}

while (j < 20)
  // Do one statement
```

#### Functions

#### Inline Assembly

> This feature is unstable.

```
asm {
}
```

#### Constraints

> This feature is not yet implemented.

### Source Files

#### Structures

> This feature is not yet implemented.

### Interface Files

#### Interfaces

> This feature is not yet implemented.

### Test Files

#### Mocks

> This feature is not yet implemented.

#### Tests

> This feature is not yet implemented.
