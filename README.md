8086 Simulator in C89
=====================


Based on Performance-Aware Programming series by Casey Muratori and Intel 8086 Family User's Manual.

Some of the architecture of this software is inspired from [Performance-Aware Programming's source code](https://github.com/cmuratori/computer_enhance/tree/main/perfaware/sim86) such as metaprogramming for the instruction encodings.

This software is not intended to be Intel 8086 emulator!


Build and run
-------------

```bash
# Run GNU Make on Linux or just run any C compiler on `8086sim.c`
make

# Decode a binary
./build/8086sim decode samples/05_completionist_decode

# Execute a binary
./build/8086sim exec samples/08_challenge_register_movs
```


Dependency
----------

The decoder has no dependency.

The decoder testing script requires `nasm` assembler to work.


"No AI" Policy
--------------

Uses of generative AI or LLM are forbidden for this project. The purpose of this project is for the human to learn, not AI.

[Reasons to reject generative AI and LLMs](https://codeberg.org/small-hack/open-slopware#why-not-llms)
