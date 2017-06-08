# findPatterns
A small script that detect patterns in a list of hex numbers
This project was created in order to find patern is a bunch of inlined opcodes, and figure out the codeflow.

## How
The binary takes two arguments: the path to a list of numbers in hex form (0x12345678), 32 bits, less than 0xf0000000.
The second argument is the file to which the output will be written to.

## Output
Two sections, `Paterns:` where each patern found is written as `ID <patern ID, counter starting from 0xf0000000 (refCount: <number of occurencies of the pattern>)` followed by a list of either patterns or hex from the original file.
The second section is simply `Code:` followed by numbers/instructions not in any pattern.
If a pattern is big enough, we look for patterns inside the patern, in which case, if one is found, the output described earlier is repeated, inline indented one level deeper.

## Example

```
[...]
ID 0xf0000023(refCount: 3)
0x434fd4
0x43499f


Code:

0xf0000021
0x43445c
[...]
```
