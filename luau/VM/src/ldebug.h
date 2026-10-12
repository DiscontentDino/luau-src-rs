// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#pragma once

#include "lstate.h"

#define pcRel(pc, p) ((pc) && (pc) != (p)->code ? cast_to(int, (pc) - (p)->code) - 1 : 0)

#define LUA_MEMERRMSG "not enough memory"
#define LUA_ERRERRMSG "error in error handling"

#if LUAU_ERRORS_AS_RETURNS
#define luaG_raisewith_(ret, call) \
    do \
    { \
        call; \
        return ret; \
    } while (0)

#define luaG_raisetypeerror(L, ret, o, opname) luaG_raisewith_(ret, luaG_settypeerrorL(L, o, opname))
#define luaG_raiseforerror(L, ret, o, what) luaG_raisewith_(ret, luaG_setforerrorL(L, o, what))
#define luaG_raiseconcaterror(L, ret, p1, p2) luaG_raisewith_(ret, luaG_setconcaterror(L, p1, p2))
#define luaG_raisearitherror(L, ret, p1, p2, op) luaG_raisewith_(ret, luaG_setaritherror(L, p1, p2, op))
#define luaG_raiseordererror(L, ret, p1, p2, op) luaG_raisewith_(ret, luaG_setordererror(L, p1, p2, op))
#define luaG_raiseindexerror(L, ret, p1, p2) luaG_raisewith_(ret, luaG_setindexerror(L, p1, p2))
#define luaG_raisemethoderror(L, ret, p1, p2) luaG_raisewith_(ret, luaG_setmethoderror(L, p1, p2))
#define luaG_raisemissingmembererror(L, ret, p1, p2) luaG_raisewith_(ret, luaG_setmissingmembererror(L, p1, p2))
#define luaG_raisereadonlyerror(L, ret) luaG_raisewith_(ret, luaG_setreadonlyerror(L))
#define luaG_raiserunerror(L, ret, fmt, ...) luaG_raisewith_(ret, luaG_setrunerrorL(L, fmt, ##__VA_ARGS__))

LUAI_FUNC void luaG_settypeerrorL(lua_State* L, const TValue* o, const char* opname);
LUAI_FUNC void luaG_setforerrorL(lua_State* L, const TValue* o, const char* what);
LUAI_FUNC void luaG_setconcaterror(lua_State* L, StkId p1, StkId p2);
LUAI_FUNC void luaG_setaritherror(lua_State* L, const TValue* p1, const TValue* p2, TMS op);
LUAI_FUNC void luaG_setordererror(lua_State* L, const TValue* p1, const TValue* p2, TMS op);
LUAI_FUNC void luaG_setindexerror(lua_State* L, const TValue* p1, const TValue* p2);
LUAI_FUNC void luaG_setmethoderror(lua_State* L, const TValue* p1, const TString* p2);
LUAI_FUNC void luaG_setmissingmembererror(lua_State* L, const TValue* p1, const TValue* p2);
LUAI_FUNC void luaG_setreadonlyerror(lua_State* L);

LUAI_FUNC LUA_PRINTF_ATTR(2, 3) void luaG_setrunerrorL(lua_State* L, const char* fmt, ...);
#else
#define luaG_typeerror(L, o, opname) luaG_typeerrorL(L, o, opname)
#define luaG_forerror(L, o, what) luaG_forerrorL(L, o, what)
#define luaG_runerror(L, fmt, ...) luaG_runerrorL(L, fmt, ##__VA_ARGS__)

#define luaG_raisetypeerror(L, ret, o, opname) luaG_typeerrorL(L, o, opname)
#define luaG_raiseforerror(L, ret, o, what) luaG_forerrorL(L, o, what)
#define luaG_raiseconcaterror(L, ret, p1, p2) luaG_concaterror(L, p1, p2)
#define luaG_raisearitherror(L, ret, p1, p2, op) luaG_aritherror(L, p1, p2, op)
#define luaG_raiseordererror(L, ret, p1, p2, op) luaG_ordererror(L, p1, p2, op)
#define luaG_raiseindexerror(L, ret, p1, p2) luaG_indexerror(L, p1, p2)
#define luaG_raisemethoderror(L, ret, p1, p2) luaG_methoderror(L, p1, p2)
#define luaG_raisemissingmembererror(L, ret, p1, p2) luaG_missingmembererror(L, p1, p2)
#define luaG_raisereadonlyerror(L, ret) luaG_readonlyerror(L)
#define luaG_raiserunerror(L, ret, fmt, ...) luaG_runerrorL(L, fmt, ##__VA_ARGS__)

LUAI_FUNC l_noret luaG_typeerrorL(lua_State* L, const TValue* o, const char* opname);
LUAI_FUNC l_noret luaG_forerrorL(lua_State* L, const TValue* o, const char* what);
LUAI_FUNC l_noret luaG_concaterror(lua_State* L, StkId p1, StkId p2);
LUAI_FUNC l_noret luaG_aritherror(lua_State* L, const TValue* p1, const TValue* p2, TMS op);
LUAI_FUNC l_noret luaG_ordererror(lua_State* L, const TValue* p1, const TValue* p2, TMS op);
LUAI_FUNC l_noret luaG_indexerror(lua_State* L, const TValue* p1, const TValue* p2);
LUAI_FUNC l_noret luaG_methoderror(lua_State* L, const TValue* p1, const TString* p2);
LUAI_FUNC l_noret luaG_missingmembererror(lua_State* L, const TValue* p1, const TValue* p2);
LUAI_FUNC l_noret luaG_readonlyerror(lua_State* L);

LUAI_FUNC LUA_PRINTF_ATTR(2, 3) l_noret luaG_runerrorL(lua_State* L, const char* fmt, ...);
#endif

LUAI_FUNC void luaG_pusherror(lua_State* L, const char* error);

LUAI_FUNC void luaG_breakpoint(lua_State* L, Proto* p, int line, bool enable);
LUAI_FUNC bool luaG_onbreak(lua_State* L);

LUAI_FUNC int luaG_getline(Proto* p, int pc);

LUAI_FUNC int luaG_isnative(lua_State* L, int level);
LUAI_FUNC int luaG_hasnative(lua_State* L, int level);
