This is the PARTEE runtime, to be presented in IPDPS 2016.
Nikolaos Papakonstantinou's MSc [thesis](https://dl.dropboxusercontent.com/u/6873550/thesis.pdf)

Nikolaos Papakonstantinou, Foivos S. Zakkak, and Polyvios Pratikakis.
*Hierarchical parallel dynamic dependence analysis for recursively
Task-Parallel programs*.  In 30th IEEE International Parallel &
Distributed Processing Symposium (IEEE IPDPS 2016), Chicago, USA, May
2016.

# Build

```
make lib                Build the basic library.
make bechs              Build all benchmarks in ./bechmark_suite directory.
make all                Build the previous targets (app and Apps).
make clean_lib          Remove object file of the basic Library.
make distclean          Remove the basic Library.
make clean_bechs        Clear all benchmarks in ./bechmark_suitedirectory.
make clean_all          Clear both the basic library and benchmarks.
make clear              Clear both the basic library and benchmarks and
                        temporery files.
make help               Prints this message.
```
