# Saucer

## Introduction
Saucer is a simple makefile generator that can be used to create different makefiles for
different platforms using simple yaml configuration.

## Usage
In order to create makefile for current platform  
`$ saucer build.yml`    
For specific platform `-p` flag can be used. Following examples show how to create makefile for different platform other than the one being detected right now.   
Using Macos configuration: `$ saucer build.yml -p apple`   
Using Windows configuration: `$ saucer build.yml -p windows`  
Using Linux configuration: `$ saucer build.yml -p linux`  

## YAML configuration
Yaml configuration file can be created easily. Two most important keys in yaml configuration file are `premake` and `tasks` keys. Keys that support platform specific values can be specified with following subkeys:  
`apple` for macos specific configuration value  
`linux` for linux specific configuration value    
`windows` for windows specific configuration value
### Import
Sometimes it might be necessary to use another projects in your project. For that purpose `import` flag can be used, which allows to use other projects' build configs with different source directories, while keeping the same build destination directory as your initial project.  
Import flag specification is following:  
* `import`        - path to other build config (optional / can be platform specific)  

### Premake
Premake key can contain following subkeys:  
* `cc`            - C compiler (semi-optional / non platfrom specific)
* `cxx`           - C++ compiler (semi-optional / non platform specific)
* `cc_flags`      - universal C compiler flags (optional / can be platform specific)
* `cxx_flags`     - universal C++ compiler flags (optional / can be platform specific)
* `include_path`  - include paths that are used by compiler (optional / can be platform specific)
* `library_path`  - library path used in linking (optional / non platform specific)   
* `src_dir`       - path for all the source files (mandatory / non platform specific)
* `build_dir`     - path for all the binary files (mandatory / non platform specific)
* `project_name`  - name of the project (mandatory / non platform specific)  
* `library_path`  - path to search for libraries


### Tasks
Tasks key must contain at least one subkey with it's name being used as a task target.  
Tasks subkey can contain following subkeys:  
* `type`          - target type (mandatory / non platform specific)  
* `src`           - target source files (mandatory / can be platform specific)
* `deps`          - target dependencies (optional exec only / can be platform specific)    

Task types can be following:  
* `exec`          - executable target
* `dynamic_lib`   - dynamic library target (.so for unix like systems / .dll for windows)
* `static_lib`    - static library target    

### Cpy and link keys
`cpy` key allows to copy target to build/[PROJECT_NAME] and `link` key creates a new symlink between  a target and build/[PROJECT_NAME]    
Cpy and link keys:
* `cpy`           - copy from subkey value to destination in [BUILD_DIR]/[SUBKEY_NAME]  
* `link`          - create symlink between subkey value and [BUILD_DIR]/[SUBKEY_NAME]  

## Building
To build and install Saucer use following make command:  
`$ sudo make clean install`