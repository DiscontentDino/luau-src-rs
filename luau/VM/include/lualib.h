// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#pragma once

#include "lua.h"

#define luaL_raiseerror(L, fmt, ...) luaL_raiseerrorL(L, fmt, ##__VA_ARGS__)
#define luaL_raisetypeerror(L, narg, tname) luaL_raisetypeerrorL(L, narg, tname)
#define luaL_raiseargerror(L, narg, extramsg) luaL_raiseargerrorL(L, narg, extramsg)

#if !LUAU_ERRORS_AS_RETURNS
#define luaL_error(L, fmt, ...) luaL_errorL(L, fmt, ##__VA_ARGS__)
#define luaL_typeerror(L, narg, tname) luaL_typeerrorL(L, narg, tname)
#define luaL_argerror(L, narg, extramsg) luaL_argerrorL(L, narg, extramsg)
#endif

struct luaL_Reg
{
    const char* name;
    lua_CFunction func;
};
typedef struct luaL_Reg luaL_Reg;

LUALIB_API void luaL_register(lua_State* L, const char* libname, const luaL_Reg* l);
LUALIB_API int luaL_getmetafield(lua_State* L, int obj, const char* e);
LUALIB_API int luaL_callmeta(lua_State* L, int obj, const char* e);
#if LUAU_ERRORS_AS_RETURNS
LUALIB_API int luaL_raisetypeerrorL(lua_State* L, int narg, const char* tname);
LUALIB_API int luaL_raiseargerrorL(lua_State* L, int narg, const char* extramsg);
#else
LUALIB_API int LUA_NORETURN luaL_raisetypeerrorL(lua_State* L, int narg, const char* tname);
LUALIB_API int LUA_NORETURN luaL_raiseargerrorL(lua_State* L, int narg, const char* extramsg);
LUALIB_API l_noret luaL_typeerrorL(lua_State* L, int narg, const char* tname);
LUALIB_API l_noret luaL_argerrorL(lua_State* L, int narg, const char* extramsg);
#endif

LUALIB_API LUA_NODISCARD int luaL_requirelstring(lua_State* L, int narg, const char** out, size_t* len);
LUALIB_API LUA_NODISCARD int luaL_requireoptlstring(lua_State* L, int narg, const char* def, const char** out, size_t* len);
LUALIB_API LUA_NODISCARD int luaL_requirenumber(lua_State* L, int narg, double* out);
LUALIB_API LUA_NODISCARD int luaL_requireoptnumber(lua_State* L, int narg, double def, double* out);

LUALIB_API LUA_NODISCARD int luaL_requireboolean(lua_State* L, int narg, int* out);
LUALIB_API LUA_NODISCARD int luaL_requireoptboolean(lua_State* L, int narg, int def, int* out);

LUALIB_API LUA_NODISCARD int luaL_requireinteger(lua_State* L, int narg, int* out);
LUALIB_API LUA_NODISCARD int luaL_requireinteger64(lua_State* L, int narg, int64_t* out);
LUALIB_API LUA_NODISCARD int luaL_requireoptinteger(lua_State* L, int narg, int def, int* out);
LUALIB_API LUA_NODISCARD int luaL_requireoptinteger64(lua_State* L, int narg, int64_t def, int64_t* out);
LUALIB_API LUA_NODISCARD int luaL_requireunsigned(lua_State* L, int narg, unsigned* out);
LUALIB_API LUA_NODISCARD int luaL_requireoptunsigned(lua_State* L, int narg, unsigned def, unsigned* out);

LUALIB_API LUA_NODISCARD int luaL_requirevector(lua_State* L, int narg, const LUA_VECTOR_TYPE** out);
LUALIB_API LUA_NODISCARD int luaL_requireoptvector(lua_State* L, int narg, const LUA_VECTOR_TYPE* def, const LUA_VECTOR_TYPE** out);

LUALIB_API LUA_NODISCARD int luaL_requirestack(lua_State* L, int sz, const char* msg);
LUALIB_API LUA_NODISCARD int luaL_requiretype(lua_State* L, int narg, int t);
LUALIB_API LUA_NODISCARD int luaL_requireany(lua_State* L, int narg);

LUALIB_API LUA_NODISCARD int luaL_requireudata(lua_State* L, int ud, const char* tname, void** out);
LUALIB_API LUA_NODISCARD int luaL_requireudatatagged(lua_State* L, int ud, int tag, void** out);

LUALIB_API LUA_NODISCARD int luaL_requirebuffer(lua_State* L, int narg, void** out, size_t* len);

LUALIB_API LUA_NODISCARD int luaL_requireoption(lua_State* L, int narg, const char* def, const char* const lst[], int* out);

#if !LUAU_ERRORS_AS_RETURNS
LUALIB_API const char* luaL_checklstring(lua_State* L, int numArg, size_t* l);
LUALIB_API const char* luaL_optlstring(lua_State* L, int numArg, const char* def, size_t* l);
LUALIB_API double luaL_checknumber(lua_State* L, int numArg);
LUALIB_API double luaL_optnumber(lua_State* L, int nArg, double def);

LUALIB_API int luaL_checkboolean(lua_State* L, int narg);
LUALIB_API int luaL_optboolean(lua_State* L, int narg, int def);

LUALIB_API int luaL_checkinteger(lua_State* L, int numArg);
LUALIB_API int64_t luaL_checkinteger64(lua_State* L, int numArg);
LUALIB_API int luaL_optinteger(lua_State* L, int nArg, int def);
LUALIB_API int64_t luaL_optinteger64(lua_State* L, int nArg, int64_t def);
LUALIB_API unsigned luaL_checkunsigned(lua_State* L, int numArg);
LUALIB_API unsigned luaL_optunsigned(lua_State* L, int numArg, unsigned def);

LUALIB_API const LUA_VECTOR_TYPE* luaL_checkvector(lua_State* L, int narg);
LUALIB_API const LUA_VECTOR_TYPE* luaL_optvector(lua_State* L, int narg, const LUA_VECTOR_TYPE* def);

LUALIB_API void luaL_checkstack(lua_State* L, int sz, const char* msg);
LUALIB_API void luaL_checktype(lua_State* L, int narg, int t);
LUALIB_API void luaL_checkany(lua_State* L, int narg);

LUALIB_API void* luaL_checkudata(lua_State* L, int ud, const char* tname);
LUALIB_API void* luaL_checkudatatagged(lua_State* L, int ud, int tag);

LUALIB_API void* luaL_checkbuffer(lua_State* L, int narg, size_t* len);

LUALIB_API int luaL_checkoption(lua_State* L, int narg, const char* def, const char* const lst[]);
#endif

LUALIB_API int luaL_newmetatable(lua_State* L, const char* tname);

LUALIB_API void luaL_where(lua_State* L, int lvl);
#if LUAU_ERRORS_AS_RETURNS
LUALIB_API LUA_PRINTF_ATTR(2, 3) int luaL_raiseerrorL(lua_State* L, const char* fmt, ...);
#else
LUALIB_API LUA_PRINTF_ATTR(2, 3) int LUA_NORETURN luaL_raiseerrorL(lua_State* L, const char* fmt, ...);
LUALIB_API LUA_PRINTF_ATTR(2, 3) l_noret luaL_errorL(lua_State* L, const char* fmt, ...);
#endif

LUALIB_API const char* luaL_tolstring(lua_State* L, int idx, size_t* len);

LUALIB_API lua_State* luaL_newstate(void);

LUALIB_API const char* luaL_findtable(lua_State* L, int idx, const char* fname, int szhint);

LUALIB_API const char* luaL_typename(lua_State* L, int idx);

LUALIB_API void luaL_traceback(lua_State* L, lua_State* L1, const char* msg, int level);

/*
** ===============================================================
** some useful macros
** ===============================================================
*/

#define luaL_argrequire(L, cond, arg, extramsg) \
    do \
    { \
        if (!(cond)) \
            return luaL_raiseargerror(L, arg, extramsg); \
    } while (0)
#define luaL_argrequiretype(L, cond, arg, tname) \
    do \
    { \
        if (!(cond)) \
            return luaL_raisetypeerror(L, arg, tname); \
    } while (0)

#define luaL_requirestring(L, n, out) (luaL_requirelstring(L, (n), (out), NULL))
#define luaL_requireoptstring(L, n, d, out) (luaL_requireoptlstring(L, (n), (d), (out), NULL))

#if LUAU_ERRORS_AS_RETURNS
#define luaL_propagate(L) \
    do \
    { \
        if (lua_haserror(L)) \
            return LUA_RAISED; \
    } while (0)
#else
#define luaL_propagate(L) ((void)0)

#define luaL_argcheck(L, cond, arg, extramsg) ((void)((cond) ? (void)0 : luaL_argerror(L, arg, extramsg)))
#define luaL_argexpected(L, cond, arg, tname) ((void)((cond) ? (void)0 : luaL_typeerror(L, arg, tname)))

#define luaL_checkstring(L, n) (luaL_checklstring(L, (n), NULL))
#define luaL_optstring(L, n, d) (luaL_optlstring(L, (n), (d), NULL))
#endif

#define luaL_getmetatable(L, n) (lua_getfield(L, LUA_REGISTRYINDEX, (n)))

#define luaL_opt(L, f, n, d) (lua_isnoneornil(L, (n)) ? (d) : f(L, (n)))

// backwards compatibility
#define luaL_callyieldable(L, nargs, nresults) lua_callyieldable(L, nargs, nresults)
#define luaL_pcallyieldable(L, nargs, nresults, errfunc) lua_pcallyieldable(L, nargs, nresults, errfunc)

// generic buffer manipulation

struct luaL_Strbuf
{
    char* p;   // current position in buffer
    char* end; // end of the current buffer
    lua_State* L;
    struct TString* storage;
    char buffer[LUA_BUFFERSIZE];
};
typedef struct luaL_Strbuf luaL_Strbuf;

// compatibility typedef: this type is called luaL_Buffer in Lua headers
// renamed to luaL_Strbuf to reduce confusion with internal VM buffer type
typedef struct luaL_Strbuf luaL_Buffer;

// when internal buffer storage is exhausted, a mutable string value 'storage' will be placed on the stack
// in general, functions expect the mutable string buffer to be placed on top of the stack (top-1)
// with the exception of luaL_addvalue that expects the value at the top and string buffer further away (top-2)

#if LUAU_ERRORS_AS_RETURNS
#define luaL_addchar(B, c) \
    ((void)((B)->p < (B)->end || luaL_prepbuffsize(B, 1)), (B)->p < (B)->end ? (void)(*(B)->p++ = (char)(c)) : (void)0)
#else
#define luaL_addchar(B, c) ((void)((B)->p < (B)->end || luaL_prepbuffsize(B, 1)), (*(B)->p++ = (char)(c)))
#endif
#define luaL_addstring(B, s) luaL_addlstring(B, s, strlen(s))

LUALIB_API void luaL_buffinit(lua_State* L, luaL_Strbuf* B);
LUALIB_API char* luaL_buffinitsize(lua_State* L, luaL_Strbuf* B, size_t size);
LUALIB_API char* luaL_prepbuffsize(luaL_Buffer* B, size_t size);
LUALIB_API void luaL_addlstring(luaL_Strbuf* B, const char* s, size_t l);
LUALIB_API void luaL_addvalue(luaL_Strbuf* B);
LUALIB_API void luaL_addvalueany(luaL_Strbuf* B, int idx);
LUALIB_API void luaL_pushresult(luaL_Strbuf* B);
LUALIB_API void luaL_pushresultsize(luaL_Strbuf* B, size_t size);

// builtin libraries
LUALIB_API int luaopen_base(lua_State* L);

#define LUA_COLIBNAME "coroutine"
LUALIB_API int luaopen_coroutine(lua_State* L);

#define LUA_TABLIBNAME "table"
LUALIB_API int luaopen_table(lua_State* L);

#define LUA_OSLIBNAME "os"
LUALIB_API int luaopen_os(lua_State* L);

#define LUA_STRLIBNAME "string"
LUALIB_API int luaopen_string(lua_State* L);

#define LUA_BITLIBNAME "bit32"
LUALIB_API int luaopen_bit32(lua_State* L);

#define LUA_BUFFERLIBNAME "buffer"
LUALIB_API int luaopen_buffer(lua_State* L);

#define LUA_UTF8LIBNAME "utf8"
LUALIB_API int luaopen_utf8(lua_State* L);

#define LUA_CLASSLIBNAME "class"
LUALIB_API int luaopen_class(lua_State* L);

#define LUA_MATHLIBNAME "math"
LUALIB_API int luaopen_math(lua_State* L);

#define LUA_DBLIBNAME "debug"
LUALIB_API int luaopen_debug(lua_State* L);

#define LUA_VECLIBNAME "vector"
LUALIB_API int luaopen_vector(lua_State* L);

#define LUA_INTLIBNAME "integer"
LUALIB_API int luaopen_integer(lua_State* L);

// open all builtin libraries
LUALIB_API void luaL_openlibs(lua_State* L);

// sandbox libraries and globals
LUALIB_API void luaL_sandbox(lua_State* L);
LUALIB_API void luaL_sandboxthread(lua_State* L);
