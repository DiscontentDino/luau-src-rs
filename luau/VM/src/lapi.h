// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#pragma once

#include "lobject.h"

#if LUAU_ERRORS_AS_RETURNS
#define api_checknoerror(L) api_check(L, (L)->global->errcode == 0)
#else
#define api_checknoerror(L) ((void)0)
#endif

LUAI_FUNC const TValue* luaA_toobject(lua_State* L, int idx);
LUAI_FUNC void luaA_pushvalue(lua_State* L, const TValue* o);
LUAI_FUNC void luaA_pushclass(lua_State* L, LuauClass* lclass);
LuaTable* getcurrenv(lua_State* L);