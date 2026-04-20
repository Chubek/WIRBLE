# WIRBLE: Retagetable Compiler Infrastructure

WIRBLE, my latest AI slop, which is thoroughly tested, is now usable. I did not make it for a girl this time, don't worry.

What WIRBLE offers, at base, is a graphi-based IR which is an unholy mix of Sea of Nodes and Graphical SSA. However, SSA is not necessary or WIRBLE.

After you write your IR in WIL (WIRBLE Intermediate Language), the options are:
- Rewrite it usin Wirble Rewrite System;
- Lower to to MAL (Machine-Adjacent Language);
- Through instruction selection, conver it to MDL (Machine Description Language);
- Rewrite the target codde via peephole optization, allocate registers, and so on;
- There are many things you can do nw, but logically, we compile the chunk.

WIRBLE is mostly aimed at AOT/JIT, and its facilities reflect that. As of this moment, WIRBLE cannot write an assembly file, or link binaries.

You can serialize your generated code and use it for caching. A cache is just one of the utilities WIRBLE provides. We have trace recorder and race cache as well. As I said, WIRBLE is mostly aimed at AOT/JIT, not whole programs.

WIRBLE is retargetable. We define the targets in XML, YAML, or JSON files. Three targets have been provided by default, accessible in `machines` directory.

I will complete this README as I keep using WIRBLE for my projects.
