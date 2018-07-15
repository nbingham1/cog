# Cog

> Cog is under active development. Not all documented language features have been implemented.

Cog is a C-like systems programming language intented to maximize modular design with simple features for enforcing and testing correctness.

1. [Examples](#examples)
2. [Benchmarks](#benchmarks)
3. [Syntax](#syntax)
   1. [Program Logic](#program-logic)
      * [Comments](#comments)
      * [Variables](#variables)
      * [Expressions](#expressions)
      * [Static Arrays](#static-arrays)
      * [Dynamic Arrays](#dynamic-arrays)
      * [Pointers](#pointers)
      * [Implicit Casting Rules](#implicit-casting-rules)
      * [If Statements](#if-statements)
      * [While Loops](#while-loops)
      * [Functions](#functions)
      * [Inline Assembly](#inline-assembly)
   2. [Program Structure](#program-structure)
   3. [Source Files](#source-files)
      * [Structures](#structures)
   4. [Interface Files](#interface-files)
      * [Interfaces](#interfaces)
      * [Protocols](#protocols)
      * [Encodings](#encodings)
   5. [Test Files](#test-files)
      * [Mocks](#mocks)
      * [Tests](#tests)
   7. [Program Files](#program-files)
      * [Linking](#linking)
      * [Processes](#processes)
   6. [Metaprogramming](#metaprogramming)
      * [Templates](#templates)
      * [Dependencies](#dependencies)
      * [Constraints](#constraints)
4. [Compiler](#compiler)
5. [Debugger](#debugger)
6. [Documenter](#documenter)

## Examples

## Benchmarks

> No benchmarks have been run at this time.

## Syntax

A formal specification of the grammar follows:
* [Lex Specificiation](src/Lexer.l)
* [Yacc Grammar](src/Parser.y)

### Program Logic

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

Variables are declared and assigned using a C-like syntax.

```
type name;
type name = value;
type name0, name1, name2;
type name0 = value0, name1 = value1, name2 = value2;
```

The usual primitive types are supported with added support for fixed point numbers. Here, <n> represents the bitwidth of the type and <m> represents it's power of 2 exponent. For example, `int32` is a 32 bit signed integer. `fixed32e-5` is a 32 bit fixed point decimal with a five bit fractional precision. int<n>, uint<n>, fixed<n>e<m>, and ufixed<n>e<m> all support arbitrary values for <n> and <m>. However, float<n> is limited to bitwidths of 16, 32, 64, or 128.

```
bool
int<n>
uint<n>
fixed<n>e<m>
ufixed<n>e<m>
float<n>
```

Both integer and decimal constants are encoded as fixed point numbers with arbitrary precision. This means that all constant expressions will evaluate at compile time without any rounding errors. Once all constant expressions are evaluated, the results are implicitly cast to and stored as the type that they are assigned in the program. Constants may be specified in base 10 as decimal values with a base 10 exponent or in base 16 or base 2 as integers.

```
578
63e-4
3.2e6
0x88af
0b1001
```

#### Expressions

The following table lists the precedence and associativity of all supported operators in descending precedence.

Precedence | Operator | Description | Associativity | Implemented
---
**1** | :: | scope resolution | left to right | no
---
**2** | a() | function call | left to right | yes
 | a[] | subscript | | no
 | a\* a&amp; | pointer dereference, address-of | | no
 | a{type} | typecast | | no
 | . | member access | | no
---
**3** | ~ | bitwise not | right to left | yes
 | not | boolean not | | yes
 | - | negative | | yes
 | new | allocate memory | | no
 | delete | deallocate memory | | no
---
**4** | a\*b a/b a%b | multiplication, division, remainder | left to right | yes
---
**5** | a+b a-b | addition, subtraction | left to right | yes
---
**6** | &lt;&lt; &gt;&gt; &gt;&gt;&gt; | left shift, arithmetic right shift, logical right shift | left to right | yes
 | &lt;&lt;&gt; &gt;&gt;&lt; | rotate left, rotate right | | yes
---
**7** | a&amp;b | bitwise AND | left to right | yes
---
**8** | a^b | bitwise XOR | left to right | yes
---
**9** | a\|b | bitwise OR | left to right | yes
---
**10** | a&lt;b | less than | left to right | yes
 | a&gt;b | greater than |  | yes
 | a&lt;=b | less or equal |  | yes
 | a&gt;=b | greater or equal |  | yes
 | a==b | equal to |  | yes
 | a!=b | not equal to |  | yes
---
**11** | a and b | boolean AND | left to right | yes
---
**12** | a xor b | boolean XOR | left to right | yes
---
**13** | a or b | boolean OR | left to right | yes

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

### Program Structure

> This feature is not yet implemented.

Cog programs are divided up into four file types that serve different purposes. 

#### Interface Files

`.ifc` files specify limited interfaces designed for specific purposes. These represent the only way that programs may be linked, ensuring that any structure in a code base may be swapped out for something else while guaranteeing correct functionality.

Below shows an example of an interface file for a simple stack. The first line declares a templated type `ValueType` and two interfaces that use that type, `Node` and `Stack`. Finally, it specifies a function `pop()` that is only dependent upon the functionality provided by the interface and therefore automatically implemented for all structures that implement the `Stack` interface.

```
typename ValueType;

interface Node<ValueType>
{
	construct(ValueType value, Node<ValueType> prev*);

	void push(Node<ValueType> list*);
	Node<ValueType>* pop();
	ValueType get();
}

interface Stack<ValueType>
{
	void push(ValueType value);
	void drop();
	ValueType get();

	ValueType pop()
	{
		ValueType result = get();
		drop();
		return result;
	}
}
```

#### Cog Source Files

`.cog` files each contain a structure that implements a set of interfaces. These files may import `.ifc` files and create dependent types that are eventually linked in the `.prg` file.

Interface files may be imported in two ways. The `import` directive imports everything from the file and puts it all in the file's namespace while the 'as' directive sets the space's name. Alternatively, elements from may be imported directly via the `from` directive. 

```
import "Stack.ifc" as StackIfc;

typename ValueType;

struct Stack<ValueType>
{
	depend Node implements StackIfc::Node<ValueType>;
	suggest Node = Node<ValueType> from "Node.cog";

	Node *last;

	construct()
	{
	}

	destruct()
	{
		if (last)
			delete last;
	}

	void push(ValueType value)
	{
		Node *node = new Node(value, last);
		last = node;
	}

	void drop()
	{
		Node *node = last;
		last = last->pop();
		delete node;
	}

	ValueType get()
	{
		return last->get();
	}
}

keep Stack<ValueType> implements StackIfc::Stack<ValueType>;

```

`.tst` test files specify an array of tests on interfaces, structures, and functions that check their implementation.

`.prg` programs specify a the final import and link configurations, and the main behavior of a binary.

### Source Files

#### Structures

> This feature is not yet implemented.

### Interface Files

#### Interfaces

> This feature is not yet implemented.

### Test Files

#### Mocks

> This feature is not yet implemented.

Mocks are like structures in that they implement functionality of a given set of interfaces, except that they implement only the functionality necessary to execute the tests.

```
mock myMock
{
};
```

#### Tests

> This feature is not yet implemented.

Tests specify a contained set of logic that exercises specific functionality of structures or interfaces.

```
test("My Test")
{
	// Do things.
}
```

### Metaprogramming

#### Templates

> This feature is not yet implemented.

#### Dependencies

> This feature is not yet implemented.

#### Constraints

> This feature is not yet implemented.

## Compiler

## Debugger

## Documenter
