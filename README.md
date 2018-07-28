# Cog

> Cog is under active development. Not all documented language features have been implemented.

Cog is a C-like systems programming language intented to maximize modular design with simple features for enforcing and testing correctness.

1. [Examples](#examples)
2. [Benchmarks](#benchmarks)
3. [Syntax](#syntax)
   1. [Program Logic](#program-logic)
      * [Comments](#comments)
      * [Variables](#variables)
      * [Static Arrays](#static-arrays)
      * [Dynamic Arrays](#dynamic-arrays)
      * [Pointers](#pointers)
      * [Expressions](#expressions)
      * [Implicit Casting Rules](#implicit-casting-rules)
      * [If Statements](#if-statements)
      * [While Loops](#while-loops)
      * [Functions](#functions)
      * [Inline Assembly](#inline-assembly)
   2. [Source Files](#source-files)
      * [Structures](#structures)
   3. [Interface Files](#interface-files)
      * [Interfaces](#interfaces)
      * [Protocols](#protocols)
      * [Encodings](#encodings)
   4. [Test Files](#test-files)
      * [Mocks](#mocks)
      * [Tests](#tests)
   5. [Program Files](#program-files)
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

However, variable assignment returns void and must be separate statements.

<table>
<tr><th>Operator</th><th>Description</th><th>Implemented</th></tr>
<tr><td><code>a=b;</code></td><td>direct assignment</td><td>yes</td></tr>
<tr><td><code>a+=b;</code></td><td>inplace add</td><td>yes</td></tr>
<tr><td><code>a-=b;</code></td><td>inplace subtract</td><td>yes</td></tr>
<tr><td><code>a&#42;=b;</code></td><td>inplace multiply</td><td>yes</td></tr>
<tr><td><code>a/=b;</code></td><td>inplace divide</td><td>yes</td></tr>
<tr><td><code>a%=b;</code></td><td>inplace remainder</td><td>yes</td></tr>
<tr><td><code>a&lt;&lt;=b;</code></td><td>inplace left shift</td><td>yes</td></tr>
<tr><td><code>a&gt;&gt;=b;</code></td><td>inplace logical right shift</td><td>yes</td></tr>
<tr><td><code>a&gt;&gt;&gt;=b;</code></td><td>inplace arithmetic right shift</td><td>yes</td></tr>
<tr><td><code>a&lt;&lt;&gt;=b;</code></td><td>inplace rotate left</td><td>yes</td></tr>
<tr><td><code>a&gt;&gt;&lt;=b;</code></td><td>inplace rotate right</td><td>yes</td></tr>
<tr><td><code>a&=b;</code></td><td>inplace bitwise AND</td><td>yes</td></tr>
<tr><td><code>a^=b;</code></td><td>inplace bitwise XOR</td><td>yes</td></tr>
<tr><td><code>a|=b;</code></td><td>inplace bitwise OR</td><td>yes</td></tr>
<tr><td><code>a and=b;</code></td><td>inplace boolean AND</td><td>yes</td></tr>
<tr><td><code>a xor=b;</code></td><td>inplace boolean XOR</td><td>yes</td></tr>
<tr><td><code>a or=b;</code></td><td>inplace boolean OR</td><td>yes</td></tr>
</table>

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

#### Static Arrays

> This feature is not yet implemented.

Static arrays are declared the same as in C++, the array dimensions are specified after the variable name in the declaration and may only be compile time constants. They are also indexed similary to C++. Static arrays also carry with them compile-time size attributes which may be accessed directly.

```
int32 myArr[32][6][3];

myArr[3][2][1] = 5;

int32 width = myArr.size[0];
int32 height = myArr.size[1];
int32 depth = myArr.size[2];
```

#### Dynamic Arrays

> This feature is not yet implemented.

Dynamically allocated arrays are specified separately from pointers in Cog with empty array brackets. They may then be allocated as multidimensional arrays then used as expected. Finally, they must be deleted after use. Dynamic arrays carry with them size attributes that are set upon allocation.

```
int32 myArr[][][];

myArr = new int32[32][6][3];

myArr[3][2][1] = 5;

int32 width = myArr.size[0];
int32 height = myArr.size[1];
int32 depth = myArr.size[2];

delete myArr;
```

#### Pointers

> This feature is not yet implemented.

Pointers may only point to a single value.

```
MyStruct myPtr*;

myPtr = new MyStruct();

int32 valuePtr*;
valuePtr = myPtr*.myMember&;
valuePtr* = 3;

int32 myValue = myPtr*.myMember;

delete myPtr;
```

#### Expressions

The following table lists the precedence and associativity of all supported operators in descending precedence.

<table>
<tr><th>Precedence</th><th>Operator</th><th>Description</th><th>Associativity</th><th>Implemented</th></tr>
<tr><th>1</th><td><code>(a)</code></td><td>parenthesis</td><td></td><td>yes</td></tr>
<tr><th>2</th><td><code>a::b</code></td><td>scope resolution</td><td>left to right</td><td>no</td></tr>
<tr>
<th>3</th>
<td><code>a()</code><br><code>a[]</code><br><code>a&#42;</code><br><code>a&amp;</code><br><code>type(b)</code><br><code>a.b</code></td>
<td>function call<br>subscript<br>pointer dereference<br>address-of<br>typecast<br>member access</td>
<td>left to right</td>
<td>yes<br>no<br>no<br>no<br>no<br>no</td>
</tr>
<tr>
<th>4</th>
<td><code>~a</code><br><code>not a</code><br><code>-a</code><br><code>new a</code><br><code>delete a</code></td>
<td>bitwise not<br>boolean not<br>negative<br>allocate memory<br>deallocate memory</td>
<td>right to left</td>
<td>yes<br>yes<br>yes<br>no<br>no</td>
</tr>
<tr>
<th>5</th>
<td><code>a&#42;b</code><br><code>a/b</code><br><code>a%b</code></td>
<td>multiplication<br>division<br>remainder</td>
<td>left to right</td>
<td>yes<br>yes<br>yes</td>
</tr>
<tr>
<th>6</th>
<td><code>a+b</code><br><code>a-b</code></td>
<td>addition<br>subtraction</td>
<td>left to right</td>
<td>yes<br>yes</td>
</tr>
<tr>
<th>7</th>
<td><code>a&lt;&lt;b</code><br><code>a&gt;&gt;b</code><br><code>a&gt;&gt;&gt;b</code><br><code>a&lt;&lt;&gt;b</code><br><code>a&gt;&gt;&lt;b</code></td>
<td>left shift<br>logical right shift<br>arithmetic right shift<br>rotate left<br>rotate right</td>
<td>left to right</td>
<td>yes<br>yes<br>yes<br>yes<br>yes</td>
</tr>
<tr><th>8</th><td><code>a&amp;b</code></td><td>bitwise AND</td><td>left to right</td><td>yes</td></tr>
<tr><th>9</th><td><code>a^b</code></td><td>bitwise XOR</td><td>left to right</td><td>yes</td></tr>
<tr><th>10</th><td><code>a|b</code></td><td>bitwise OR</td><td>left to right</td><td>yes</td></tr>
<tr>
<th>11</th>
<td><code>a&lt;b</code><br><code>a&gt;b</code><br><code>a&lt;=b</code><br><code>a&gt;=b</code><br><code>a==b</code><br><code>a!=b</code></td>
<td>less than<br>greater than<br>less or equal<br>greater or equal<br>equal to<br>not equal to</td>
<td>left to right</td>
<td>yes<br>yes<br>yes<br>yes<br>yes<br>yes</td>
</tr>
<tr><th>12</th><td><code>a and b</code></td><td>boolean AND</td><td>left to right</td><td>yes</td></tr>
<tr><th>13</th><td><code>a xor b</code></td><td>boolean XOR</td><td>left to right</td><td>yes</td></tr>
<tr><th>14</th><td><code>a or b</code></td><td>boolean OR</td><td>left to right</td><td>yes</td></tr>
</table>

#### Implicit Casting Rules

Primitive types are implicitly cast when doing so would not lose significant bits. This means that implicit casts can cause loss in precision.

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

> Member functions are not yet implemented.

Functions are declared using syntax similar to C++ but with behavior similar to Go. The receiver is specified with a scope resolution operator. However functions cannot be defined inside a structure. They must be explicitly defined outside. Functions without the receiver are defined like any other function in C++.

```
struct MyStruct
{
}

int32 MyStruct::myFunc(int32 myArg, int32 myArg2)
{
	// Do things.
}

int32 myFunc2(int32 myArg, int32 myArg2)
{
	// Do Things.
}
```

#### Inline Assembly

> This feature is unstable.

Inline assembly uses AT&T syntax. Each line specifies an instruction, and each instruction has a list of comma separated operands. Registers are denoted by `%name`, constants by `$value`, and program variables by their name.

```
int32 count = 5;
int32 value = 1;

asm {
	mov value, %eax
	mov count, %ebx
loop:
	mul $2, %eax
	dec %ebx
	jnz %ebx, loop
	mov %eax, value
}

fixed32 inv_value = 1/fixed32(value);
```

### Interface Files

`.ifc` files specify limited interfaces designed for specific purposes. These represent the only way that programs may be linked, ensuring that any structure in a code base may be swapped out for something else while guaranteeing correct functionality.

Below shows an example of an interface file for a simple stack. The first line declares a templated type `ValueType` and two interfaces that use that type, `Node` and `Stack`. Finally, it specifies a function `pop()` that is only dependent upon the functionality provided by the interface and therefore automatically implemented for all structures that implement the `Stack` interface.

**Stack.ifc**
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
}

ValueType Stack<ValueType>::pop()
{
	ValueType result = get();
	drop();
	return result;
}
```

#### Interfaces

> This feature is not yet implemented.

#### Protocols

> This feature is not yet implemented.

#### Encodings

> This feature is not yet implemented.

### Source Files

`.cog` files each contain a structure that implements a set of interfaces. These files may import `.ifc` files and create dependent types that are eventually linked in the `.prg` file.

Interface files may be imported in two ways. The `import` directive imports everything from the file and puts it all in the file's namespace while the 'as' directive sets the space's name. Alternatively, elements from may be imported directly via the `from` directive. 

**Node.cog**
```
typename ValueType;

struct Node<ValueType>
{
	ValueType value;
	Node<ValueType> *prev;
};

keep Node<ValueType> implements Node<ValueType> from "Stack.ifc";

Node<ValueType>::new(ValueType value, Node<ValueType> *prev)
{
	this->value = value;
	this->prev = prev;
}

Node<ValueType>::delete()
{
	if (prev)
		delete prev;
}

void Node<ValueType>::push(Node<ValueType> list*)
{
	assert not prev;
	prev = list;
}

Node<ValueType>* Node<ValueType>::pop()
{
	Node<ValueType> result* = prev;
	prev = null;
	return result;
}

ValueType Node<ValueType>::get()
{
	return value;
}
```

**Stack.cog**
```
import "Stack.ifc" as StackIfc;

typename ValueType;

struct Stack<ValueType>
{
	depend Node implements StackIfc::Node<ValueType>;
	suggest Node = Node<ValueType> from "Node.cog";

	Node *last;
}

keep Stack<ValueType> implements StackIfc::Stack<ValueType>;

Stack<ValueType>::new()
{
}

Stack<ValueType>::delete()
{
	if (last)
		delete last;
}

void Stack<ValueType>::push(ValueType value)
{
	Node *node = new Node(value, last);
	last = node;
}

void Stack<ValueType>::drop()
{
	Node *node = last;
	last = last->pop();
	delete node;
}

ValueType Stack<ValueType>::get()
{
	return last->get();
}

```

#### Structures

> This feature is not yet implemented.

### Test Files

`.tst` test files specify an array of tests on interfaces, structures, and functions that check their implementation.


**Stack.tst**
```
depend IntStack implements Stack<int32> from "Stack.ifc";

test("myStackTest")
{
	IntStack stack;
	stack.push(5);
	stack.push(3);
	stack.push(6);
	assert stack.get() == 6;
	assert stack.pop() == 6;
	stack.drop();
	assert stack.pop() == 5;
}
```

#### Mocks

> This feature is not yet implemented.

Mocks are like structures in that they implement functionality of a given set of interfaces, except that they implement only the functionality necessary to execute the tests.

```
mock MockNode
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

### Program Files

`.prg` programs specify a the final import and link configurations, and the main behavior of a binary.

```
typename ValueType;

import Stack<ValueType> from "Stack.cog" {
	// This dependency resolution isn't necessary
	// because of the suggest in Stack.cog
	Node = Node<ValueType> from "Node.cog";
}

void main()
{
	// Do something
}
```

### Metaprogramming

#### Templates

> This feature is not yet implemented.

Templated typenames may be specified at the global scope of a file, constrained for specific uses, and then used throughout the file. Interfaces, Protocols, Structures, Functions, Mocks, and Tests may all be templated. This is done with the templating operator `<types...>` after their names.

```
typename myType1;
keep MyType in [int32, float32];

typename myType2;
keep MyType2 implements MyInterface;

struct MyStruct<MyType, MyType2>
{
	MyType value0;
	MyType2 value1;
};

void myFunc<MyType, MyType2>(MyType a, MyType2 b)
{
}
```

#### Dependencies

> This feature is not yet implemented.

#### Constraints

> This feature is not yet implemented.

## Compiler

## Debugger

## Documenter
