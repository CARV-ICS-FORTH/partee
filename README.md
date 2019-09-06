This is the PARTEE runtime, to be presented in IPDPS 2016.
Nikolaos Papakonstantinou's MSc [thesis](https://elocus.lib.uoc.gr/dlib/9/3/8/metadata-dlib-1436171591-959869-8604.tkl?search_type=simple&search_help=&display_mode=overview&wf_step=init&show_hidden=0&number=10&keep_number=&cclterm1=&cclterm2=&cclterm3=&cclterm4=&cclterm5=&cclterm6=&cclterm7=&cclterm8=&cclterm9=&cclfield1=&cclfield2=&cclfield3=&cclfield4=&cclfield5=&cclfield6=&cclfield7=&cclfield8=&cclfield9=&cclop1=&cclop2=&cclop3=&cclop4=&cclop5=&cclop6=&cclop7=&cclop8=&display_help=0&offset=1&search_coll[metadata]=0&search_coll[dlib]=1&&stored_cclquery=&skin=&rss=0&show_form=&clone_file=&export_method=none&lang=en&offset=1)

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
