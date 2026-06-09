8086 Instruction Decoder in C89
===============================


Based on Performance-Aware Programming series by Casey Muratori and Intel 8086 Family User's Manual.

Fair amount of the architecture of this code was taken from [Performance-Aware Programming's source code](https://github.com/cmuratori/computer_enhance/tree/main/perfaware/sim86) such as metaprogramming for the instruction encodings.


Build and run
-------------

```bash
# Run GNU Make on Linux or just run any C compiler on `decoder8086.c`
make

# Decode a binary
./build/decode8086 samples/05_completionist_decode
```


Dependency
----------

The decoder has no dependency.

The testing script requires `nasm` assembler to work.


"No AI" Policy
--------------

Uses of generative AI or LLM are forbidden for this project. The purpose of this project is for the human to learn, not AI.

[Reasons to reject generative AI and LLMs](https://codeberg.org/small-hack/open-slopware#why-not-llms)
