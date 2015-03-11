# bitonic_sort_mpi
bitonic_sort

This is a generic C++ implementation of bitonic sort. I have tested it with various numbers of processes and data sizes.
However, if you experience a bug, file an issue.

It takes a Range over a type T and a < comparator. It returns a sorted range. 

Unfortunately it does _not_ gauruntee that the size of the output and the size of the input are the same.
