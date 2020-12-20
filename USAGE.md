Saucer usage flags:

-n / --new                  Create new saucer preset
-l / --load [ PRESET ]      Load existing preset 
--help                      Show all available usage flags and sets


Saucer command sets:

list                        List source tree
task                        Task editor command
    new [ NAME ]                Create new task with name [ NAME ]
    type [ TYPE ]               Specify the task type
        EXECUTABLE                  Create executable as a task
        DYNAMIC_LIB                 Create shared/dynamic library as a task
        STATIC_LIB                  Create static libaryr as a task
    add                         Add new instance of:
        lib-path [ PATH ]           library path
        include-path [ PATH ]       include path
        lib-link [ LIB_NAME ]       library for linking
        src [ FILE_NAME ]           source file
    rm                          Remove instance of:
        lib-path [ ID ]             library path
        include-path [ ID ]         include path
        lib-link [ ID ]             linked library
        src [ ID ]                  source file
    list                        List all instances of:
        lib-paths                   library paths
        include-paths               include path
        lib-links                   linked libraries
        src                         source files
    switch                      Switch priorities of:
        lib-links [ ID1 ] [ ID2 ]   linking priority
        src [ ID1 ] [ ID2 ]         source file compilation priority
    exit                        Exit task view


Variables:

CC                          Specify compiler used for compiling C programs
CXX                         Specify compiler used for compiling C++ programs
CC_FLAGS                    Specify universal compiler flags