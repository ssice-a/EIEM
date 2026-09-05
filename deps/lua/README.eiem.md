# Lua 5.4.9

Unmodified upstream `src` from https://www.lua.org/ftp/lua-5.4.9.tar.gz.
SHA256: `2335b6c582a52654f94612bf10d2f4672805d05329aa6568b1d8cd9e5c6fb8e6`.
The archive hash was checked before extraction. The MIT license is included at
the end of `lua.h`. The DLL statically links the library, excluding the `lua.c`
and `luac.c` command-line programs. Build as C++ with exception unwinding;
bindings include `lua.h` directly, not the extern-C `lua.hpp` wrapper.
