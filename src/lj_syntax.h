#ifndef lj_syntax_h
#define lj_syntax_h

#include <stddef.h>

#include "lua.h"

/* Switching language frontends. */
LUA_API int32_t lua_getsyntaxmode(lua_State *L);
LUA_API int lua_setsyntaxmode(lua_State *L, int32_t mode);

/* Frontend selection based on file extension. */
LUA_API int32_t lua_getsyntaxautosel(lua_State *L);
LUA_API void lua_setsyntaxautosel(lua_State *L, int32_t autoselect);

#endif
