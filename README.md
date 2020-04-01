# green
Simple bytewise context-mixing coder

This is a C++ source of a simplest possible order16 bytewise mixing coder.
Its called "green" because it doesn't use any trees.
There're no complex data structures at all, so it should be easy to understand.
And it still compresses book1 to 212690 bytes, which is supposed to provide
a reasonable threshold for statistical text compression, as no dynamic mixing
or adaptive counters are used there.
