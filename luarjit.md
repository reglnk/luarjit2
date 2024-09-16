# LuarJIT

A LuaJIT derivative with 2 frontends.
This document assumes Lua is well-known to the reader and aims only to highlight differences of the second frontend.

## Definitions and general information

Variable identifier, or simply - identifier, or `vID`: a name that can be used for Lua variable. The token TK_name stores that stuff.\
Syntax mode: a new global state variable that affects the behaviour of lexer and parser. It is stored in `global_State::pars::mode` field.

The modes are the following:
- Mode 0: Classical backwards-compatible Lua syntax mode. All code that is valid in plain LuaJIT should work in the same way in LuarJIT running the Mode 0.
- Mode 1: The mode that is most related to all these changes (the second frontend).

A language of the second frontend tends to
1. have more syntactic sugar than Lua;
2. be fully compatible with Lua, in bounds of LuaJIT VM.
\
Thus, being like Kotlin compared to Java.

2 syntax switching directives are accepted by the parser:
- `@[lua]` tells to switch to Mode 0.
- `@[luar]` tells to switch to Mode 1.
\
These affect the mode immediately when the statement is read, and to work so, they must be a beginning of statement
(the end is right there - no semicolon is needed after the directive).

2 new API functions have been added for manipulation of syntax mode:
```c
LUA_API int lua_getsyntaxmode(lua_State *L)
```
```c
LUA_API int lua_setsyntaxmode(lua_State *L, int mode)
```
The syntax mode affects luaL_loadfile, luaL_loadstring and all functions that parse code.\
However, luaL_loadstring also affects the syntax mode: if the file has extension ".luar", it switches to Mode 1 before loading, and switches back after.
Let this feature be called 'format detection'.
Builtin global function `__syntax_mode` is made ontop of 2 above functions to manipulate the mode in runtime, for example, preceding `load`.
Call it without arguments to get the current mode, or pass in the mode to set.
Some feature is planned to override the format detection behaviour for setting any syntax mode for `dofile` and `require` without limitations.

### For syntax description, the following 'description language' will be used:

Required part is written inside the `</ >` block.\
Optional part is written inside the `[/ ]` block.\
Whitespace doesn't matter by default, except that at least one whitespace symbol may be required to separate tokens.\
To escape the `>`, `/`, ` `, or `]`, `\` may be used.\
Any other whitespace is escaped like in C: `\n`, `\r`, `\t`, `\v`, `\f`.
Words written inside `[/ ]` have the direct meaning. For `</ >`, the meaning is descriptive.\
If 2 or more `/` slashes are in brackets, then the text after each slash (and before next slash or closing bracket) means a *variant*.
This structure selects 1 of them.
Thus,\
	`[\//]` describes an optional sequence of symbols: `[//]`,\
    `[/\]]`: optional symbol ']',\
    `[/foo/bar]`: token `foo`, `bar` or nothing,\
    `</foo/bar>`: required either `foo` or `bar`,\
    `[/function]`: optional `function` token, and\
    `</function>`: "some function applicable in given context".\
The `...` operator inside brackets means that everything within these brackets preceding `...` is repeated. Incase of *variants* inside, only one of them is repeated.\
To escape `...` operator, a single backslash should be used.\
Thus,\
    `</string...>` describes 1 or more strings,\
    `[/\...]`: optional operator `...` of target language,\
    `[/\... ...]`: sequence of 0 or more operators `...` of target language,\
    `[/</string>...]`: 0 or more strings, and\
    `{[/</string>: </string> [/, </string>: </string> ...]]}`: something like a single-level primitive JSON.

## Reserved words

_(changes only)_
| mode 0     | mode 1   |
| ---------- | -------- |
| function   | fn       |
| end        |          |
|            | operator |
|            | nameof   |
|            | using    |

Keyword `function` is replaced with `fn`, meanwhile the `function` becomes not reserved,
and vice versa for when switching to classical Lua mode.

## Comments

single-line: `//` \
multiline: `[//` + `=` * N + `[` ... `]//` + `=` * N + `]`

Example:
```luar
[//===[
This part is commented.
]//]
This is too.
]//====]
This too.
]//===]
[//[ end of previous multiline comment ]//]
```

## Loops, branches and blocks

Syntax of loops `if`, `for`, `repeat until`, `while` is modified.
Now, instead of many statements from header to the `end` token, there is one 'C-like statement',
which may be either a statement like in Lua, with optional `;` for separation from further code, or a block
enclosed in `{` `}` like in C. Such a block contains 0 or more C-like statements.
As a side effect, `elseif` now can be replaced with `else if`.

### Examples
```luar
local i = 0;
while (i < 100) {
	i = i + 1
}

repeat {
	i = i - 1;
} until (i == 0);

local k = 2;
if (i == 0)
	print(0);
else if (k == 2)
	print(1);
else if (k == 8)
	print(8);
else print(874);

for k, v in pairs({bar = 12, foo = 37}) do {
	print(k, v);
}

do {
	print(12);
	print(24);
	print(37)
}
```

If condition of `if` started without brackets, `then` is required.
It's still possible to write almost as in Lua, the main difference is unneeded `end` token:
```luar
local i = 0
while i < 100 do i = i + 1

repeat i = i - 1 until i == 0

local k = 2
if i == 0 then
	print(0)
elseif k == 2 then
	print(1)
else print(2)
```
However, such a syntax is not advised to use in Mode 1 and was only left in place for such cases to simplify reading brackets:
```luar
if myobj:func((loc[0].property + 78) * loc[1].property, loc.select) then {
	myobj:signal(loc);
	myobj:accept();
}
```

C-like syntax is added because of minor convenient features, here's one of them: simple branch conditions
of such a look: (k == 2) - might be faster to seek in code and read if they're enclosed in single brackets.

The point is that there are always many ways to make the code awful and unreadable,
so it's better to give a small opportunity to improve the code and more ways to make it look like
a random sequence of symbols, but the first is more valuable because you choose that. Or not `¯\_(ツ)_/¯`

## Functions

The full definition syntax of functions, including operators:
```luar
[/local] [/operator [/<index>/<newindex>]] fn </name>([/</arg>...]) { [/</statement>...] } // 1
[/local] [/operator [/<index>/<newindex>]] fn </name>([/</arg>...]) </expression> // 2
```
1. A complete definition statement.
2. Syntactic sugar over (1), equivalent to
```luar
fn [/operator [/<index>/<newindex>]] </name>([/arg...]) { return </expression> }
```

A lambda function, or anonymous function, can be expressed as follows:
```luar
fn ([/</arg>...]) { [/</statement>...] } // 1
fn ([/</arg>...]) </expression> // 2
```

Explanation of blocks used:
- `[/operator [/<index>/<newindex>]]`: for mangling symbols, thus allowing to reference different symbols with the same name in different contexts.
  All index operators preceded with `operator<newindex>` have different mangling than ones preceded with (or having implied) `operator<index>`.
  `operator<index>` has the same meaning for all index operators as just `operator`.
- `</name>`: Some `vID` or a sequence of operator symbols. The second case implies the `operator` keyword (for readability it's recommended to explicitly specify `operator`).
- `([/</arg>...])`: Arguments list as in Lua.

Previous definition style is also available but this one is recommended.

### Examples
```luar
// variant 1
local fn empty1() {}
local fn empty2(a, b) {}
local fn add(a, b) { return a + b }
local fn qwer(a, b, c) {
    local f = a * b * 0.283;
    return f / c
}

// variant 2
local fn add(a, b) a + b;
local fn qwer(a, b, c) (a * b * 0.283) / c;

fn function() { return 8; }
function = fn() { return 8; };

print(function()); // 8

@[lua]
fn = function() return 7 end
print(fn()); --> 7
print(_G["function"]()); --> 8

@[luar]
print(_G["fn"]()) // 7

// these 2 are equivalent
local fn square(x) x * x;
local fn square(x) {
    return x * x
}

// these are too
local fn length(x, y, z) math.sqrt (
    x * x + y * y + z * z
)
local fn length(x, y, z) {
    return math.sqrt(x * x + y * y + z * z);
}

print(square(5)); // 25
print(length(7, 7, 7)); // 12.124355652982
```

### Operators

There are now really custom postfix and infix operators.

1. Custom symbolic operators may consist of symbols `+-*/%~&|^!=<>.`, and can be either infix or postfix.
2. Ones that contain a dot (field operators) can only be infix.
3. Identifier operators consist of `vID` and can only be infix.
4. A dot in symbolic operator may only be the last symbol. This syntax is reserved for planned feature (for writing `obj.field =+ 20`).
5. Only one custom operator starting with `=` can be defined: `===`.
6. Custom operators have priority 1 (the lowest among operators) and are parsed left-to-right

For example, the statement `foo =<< bar;` will output a syntax error
(`<<` will be considered a variable, and `bar` an infix operator, and its right operand is missing, that's it).

Operator calling syntax:
```luar
</expr1> </symbolic_op> </expr2> // 1
</expr1> </symbolic_op> // 2
</expr1> </ident_op> </expr2> //  3
</expr1> </field_op> </TK_name1> = </expr2> // 4
```
1. An expression, equivalent to `operator </symbolic_op>(</expr1>, </expr2>)`
2. An expression, equivalent to `operator </symbolic_op>(</expr1>)`
3. An expression, equivalent to `operator </ident_op>(</expr1>, </expr2>)`
4. An expression, equivalent to `operator<newindex> </field_op>(</expr1>, </TK_name1 as string>, </expr2>)`

These were 4 parts of statement that produce a value from their functions' return values.
The complete statement now looks like
```luar
</expr1> [/</custom_op> </expression>...] [/custom_op]
```
Custom operators have priority lower than any binary value operator and are parsed to evaluate left-to-right.

#### Examples
```luar
local fn &&(a, b) a and b;
local fn ||(a, b) a or b;
print(nil || 15); // 15
print(nil && 20); // nil

local obj = {
    v = 0
};
local fn ++(o) {
    o.v = o.v + 1;
    print(o.v);
}
obj++; // 1
obj++; // 2
```

##### Field operators
```luar
local operator fn ~.(obj, field) {
    return obj.__data[field]
}
local operator<newindex> fn ~.(obj, field, value) {
    obj.__data[field] = value;
    return value
}

local foo = {__data = {bar = 8, baz = 9}};
print(foo~.bar); // 8
print(foo~.baz); // 9
local result = foo~.bar = 27;
foo~.baz = 290;
print(foo~.bar, foo~.baz); // 27 290
assert(result == foo~.bar);
```

### Mangling

The code in Luar is generally transcribable to Lua, including operators (which use identifier symbols for encoding).
Hence, all operators' symbol names are mangled, and any name starting with `__LR` is reserved.
And so, the code for definition of Luar operator would look in Lua like following
(use at your own risk - the mangling may change at any time):
```lua
local bit = require "bit"
function __LRop_mnxmnxor(a, b)
    if type(a) == "number" then
        return bit.bxor(a, b)
    end
    local h = ""
    for i = 1, b do
        h = h .. a
    end
    return h
end

-- Usage
@[luar]
print(7 ^&*^&* 7); // 0
print(8 ^&*^&* 7); // 15
print("qwer" ^&*^&* 5); // qwerqwerqwerqwerqwer
```

#### `nameof`

For convenient definition of custom operators as methods, the new keyword `nameof` is added.
When beginning a primary expression, it will make the symbol name parse as a constant string.

Some examples:
```luar
assert(nameof foo == "foo");
local foo = nameof operator foo;
local eadd = nameof +=;
local psub = nameof operator ->;
local tmul = nameof operator<newindex> *.;
```

Possible usage (full program):
```luar
local fn defineStdOP(name) {
	_G[name] = fn(self, a) getmetatable(self)[name](self, a);
}
defineStdOP(nameof +=);
defineStdOP(nameof -=);

local obj = setmetatable({v = 0}, {
	[nameof +=] = fn(self, val) {self.v = self.v + val},
	[nameof -=] = fn(self, val) {self.v = self.v - val}
});
obj += 2;
print(obj.v); // 2
obj -= 3;
print(obj.v); // -1
```

#### Operator table keys

The `operator` keyword may be used directly in the name of field, in the same fashion
as in definition and referencing of operators (with single-part names).
Function definitions with nested names including such fields are also supported: \
`</primary expr>.[/operator [/<index>/<newindex>]] </name>` \
`fn </nested name parts...>.[/operator [/<index>/<newindex>]] </name> ([/args...]) [/</body>]`

Example (full program):
```luar
local obj = {
    [nameof operator *.] = fn() {},
    [nameof operator<newindex> *.] = fn() {}
};
fn obj.operator +=(self, o) print(self, o);

local f = obj.operator +=;
local r = obj.operator *.;
local y = obj.operator<newindex> *.;

assert(f == obj.+=);
assert(r ~= y);
assert(obj[nameof *.] == r);
```

### Extra examples

My favourite usage:
```luar
local operator fn |>(a, b) b(a);

local result = 5
    |> (fn(x) x * x)
    |> (fn(x) x * 2)
    |> (fn(x) "result is " .. x);

print(result); // result is 50
```

Say you need an operator to determite if some object is an instance of some class.
Perhaps metatables are enough for demonstration:
```luar
local operator fn is(obj, t) (
    getmetatable(obj) == t
);

local Foo = {};
Foo.__index = Foo;
fn Foo: +=(val) {
    print(self.v);
    self.v = self.v + val;
}

local Bar = {};
Bar.__index = Bar;
fn Bar: +=(val) {
    self.v = self.v + val;
    print(self.v);
}

// define common += to work for both
fn +=(a, b) a: +=(b);

local obj1 = setmetatable({v = 0}, Foo);
local obj2 = setmetatable({v = 0}, Bar);

obj1 += 20; // 0
obj2 += 40; // 40

print(obj1 is Foo, obj2 is Bar); // true	true
print(obj1 is Bar, obj2 is Foo); // false	false
```

## Optional chaining operator

`?` operator is now added as part of language to be a part of primary expressions in pair with `.`, `:`, `[]`, `()` continuations.
The behaviour of `?.` and `?[]` combinations is similar to that ones in C#.
Generally, it checks if the value of an expression part before it is 'a false value', and if so, the result of primary expression is immediately set to false or nil.
Otherwise, the code of this primary expression is executed further, in the same way as if this operator was missing.
A primary statement with this operator cannot be a destination of assignment.

### Example

```luar
print(foo?.bar?.field); // nil
print(foo?["bar"]?["field"]); // nil

local foo = {};
print(foo?.bar?.field); // nil
print(foo?["bar"]?["field"] or "nil2"); // "nil2"

foo = {bar = {field = 358}};
print(foo?.bar?.field); // 358
print(foo?["bar"]?["field"]); // 358

foo.func?(); // nothing
foo.func = fn() { print("works"); };
foo.func?(); // "works"
```

## `using`

`using [/</fields...>.]</name>` is a syntactic sugar over `local </name> = [/</fields...>.]</name>`

### Examples

```luar
local std = {ns = {bar = {foo = fn() "works"}}};
using std.ns.bar.foo;
print(foo()); // works
```

## Substitution operator

The main syntactic sugar over here.
Its purpose is to allow something similar to operators `+=`, `-=`, `*=`, `/=` and others in other languages like C or C++.
Also, this feature not need extra methods and functions. The same things are called as if you wrote  `+`, `-`, `*`, `/`...

The token for it is `=~`. Consider the following:
```luar
local obj = {qwer = {bar = {foo = 5678}}};
obj.qwer.bar.foo =~ + 1;
```
The code is equivalent to
```luar
local obj = {qwer = {bar = {foo = 5678}}};
local temp = obj.qwer.bar;
temp.foo = temp.foo + 1;
```
It's compiled to bytecode in such way that minimizes table subscripting operators.
So, in this example, for the adding 1 to this field, we need only 4 subscriptions.
2 of them for .qwer.bar, one for getting the value of temp.foo, and the last one for setting it.

Substitution of values with square brackets subscription at the end is also supported.
The following code
```luar
fn calc(a) {
	print("'calc' called!", times);
	return a * a;
}
local obj = {qwer = {bar = {foo = {[16] = 5678}}}};
obj.qwer.bar.foo[calc(4)] =~ +1
```
will output `'calc' called!` once, since it is an equivalent for
```luar
fn calc(a) {
	print("'calc' called!");
	return a * a;
}
local obj = {qwer = {bar = {foo = {[16] = 5678}}}};
local base = obj.qwer.bar.foo;
local key = calc(4);
base[key] = base[key] +1;
```

### Extra example

This code was used for testing.

```luar
fn calc(a) {
	times =~ or 0;
	times =~ + 1;
	return a * a;
}
fn getb() nameof b;
local fn geta() nameof a;
fn getr() nameof r;

// test upvalues
do {
	local w = "w";
	fn getw() {
		return w;
	}
}

local bar = getb();
bar =~.. geta();

operator fn concat(l, r) l .. r;
do {
	local operator fn >.(obj, k) obj[k];
	local op = operator >.;
	operator fn -->.(obj, k) {
		return op(obj, k);
	}
}
operator fn ->(l, r) r(l);

pass = {};
fn pass.pass(obj) obj;
using pass.pass;

local calc1 = calc;

fn test(obj)
{
	local n = times or 0;
	obj["q" .. getw() .. "e" .. getr()][bar concat getr()]-->.foo[calc((fn() 2)()) * calc1(2)] =~+ 1;
	obj["q" .. getw() .. "e" .. getr()][bar concat getr()]-->.foo[calc(2) * calc1(2)] =~+ 4;
	obj["q" .. getw() .. "e" .. getr()][bar concat getr()]-->.foo[calc(2) * calc1?(2) * (calc1?(1 * 1) * calc(1 or 10))] =~+ 80;
	obj["q" .. getw() .. "e" .. getr()][bar concat getr()]-->.foo[calc(2) * calc1?(2) * (calc1?(1 * 1) * calc(1 or 10))] =~ +
		obj["q" .. getw() .. "e" .. getr()][bar concat getr()]-->.foo[calc(2) * calc1?(2) * (calc1?(1 * 1) * calc(1 or 10))] + (2 * 773 -> pass);
	local val = obj["q" .. getw() .. "e" .. getr()][bar concat getr()]-->.foo[calc(2) * calc1?(2) * (calc1?(1 * 1) * calc(1 or 10))];
	print(obj.qwer.bar.foo[16]); // 13072
	assert(val == obj.qwer.bar.foo[16]);
	assert(val == 13072);
	assert(times - n == 20);
}

local ob1 = {qwer = {bar = {foo = {[16] = 5678}}}};
test(ob1);
local ob2 = {qwer = {brr = {}, bar = {foo = {[16] = 5678}}}};
test(ob2);
```

## Miscellaneous

### More on semicolons

They're no more so optional as in Lua, mostly because of custom operators.
While the following won't generate an error (with applicable object) and will parse as intended
(because the reserved word cannot be a part of an operand):
```luar
local h = obj++
local j = obj--
```
The following will fail miserably (there's currently no way to define an operator strictly either as infix or postfix):
```luar
h = obj++
j = obj--
```

### More on syntax mode

Here's the reference implementation of function to correctly import plain Lua modules from Luar code.
It's deprecated and useless after the update with 'format detection' feature.
Some changes to __syntax_mode are planned to make it possible to override this behaviour.
```luar
local require = require; fn luaimport(pkg)
{
	local prevmode = __syntax_mode();
	if (prevmode == 1)
		__syntax_mode(0);
	local stuff = require(pkg);
	__syntax_mode(prevmode);
	return stuff;
}
```
