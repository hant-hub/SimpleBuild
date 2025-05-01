
> :warning: This Project is currently under active development and many
features have not been tested on all target platforms

C based build system inspired by `zig.build`, `nob.h`, and Thomas Borquez'
`mate.h`. It improves somewhat on `mate.h`'s feature set alongside dropping
the build.ninja dependency.

The build system can be used by including `sb.h` in 
your project. Then include the file in a build script which
can be given any name. You can then write your build script
and use it to compile your project.

Here is an example build script:

```c
#define SB_IMPL
#include "sb.h"


int main(int argc, char* argv[]) {

    sb_BUILD(argc, argv) {
        sb_EXEC() {
            sb_add_file("test.c");

            sb_link_library("vulkan");

            sb_set_out("testapp");
            sb_set_optmize(0);
            sb_export_command();
        }
    }
    return 0;
}
```


# Current Features:
- Automatic Build Script Rebuild
- Compile commands export
- Default parrellel Command Execution
- Compiler/Platform Detection

# Future Features:
- [ ] Platform independent Utilities
    - [ ] Mkdir
    - [ ] Chdir
    - [ ] Find File
    - [ ] Delete File
- [ ] Full documentation with multiple examples
- [ ] Toggleable incremental builds
- [ ] Conditional builds
- [ ] Multiple dependencies
- [ ] Windows Compatability
    - [ ] MSVC
    - [ ] GCC MinGW
    - [ ] Clang

# Tentative Future Features:
- [ ] Visual Studio SLN generation
- [ ] MacOS Compabatility
