// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details

#include "Luau/Require.h"

#include "RequireImpl.h"

#include "lua.h"
#include "lualib.h"

#include <new>

static int validateConfig(lua_State* L, const luarequire_Configuration& config)
{
    if (!config.is_require_allowed)
        return luaL_raiseerror(L, "require configuration is missing required function pointer: is_require_allowed");
    if (!config.reset)
        return luaL_raiseerror(L, "require configuration is missing required function pointer: reset");
    if (!config.jump_to_alias)
        return luaL_raiseerror(L, "require configuration is missing required function pointer: jump_to_alias");
    if (!config.to_parent)
        return luaL_raiseerror(L, "require configuration is missing required function pointer: to_parent");
    if (!config.to_child)
        return luaL_raiseerror(L, "require configuration is missing required function pointer: to_child");
    if (!config.is_module_present)
        return luaL_raiseerror(L, "require configuration is missing required function pointer: is_module_present");
    if (!config.get_chunkname)
        return luaL_raiseerror(L, "require configuration is missing required function pointer: get_chunkname");
    if (!config.get_loadname)
        return luaL_raiseerror(L, "require configuration is missing required function pointer: get_loadname");
    if (!config.get_cache_key)
        return luaL_raiseerror(L, "require configuration is missing required function pointer: get_cache_key");
    if (!config.get_config_status)
        return luaL_raiseerror(L, "require configuration is missing required function pointer: get_config_status");
    if (config.get_alias && config.get_config)
        return luaL_raiseerror(L, "require configuration cannot define both get_alias and get_config");
    if (!config.get_alias && !config.get_config)
        return luaL_raiseerror(L, "require configuration is missing required function pointer: either get_alias or get_config (not both)");
    if (!config.load)
        return luaL_raiseerror(L, "require configuration is missing required function pointer: load");
    return 0;
}

static int pushrequireclosureinternal(
    lua_State* L,
    luarequire_Configuration_init config_init,
    void* ctx,
    lua_CFunction requirelikefunc,
    const char* debugname
)
{
    void* ud = (lua_newuserdata(L, sizeof(luarequire_Configuration)));
    luaL_propagate(L);
    if (!ud)
        return luaL_raiseerror(L, "failed to allocate memory for require configuration");

    luarequire_Configuration* config = new (ud) luarequire_Configuration{};
    config_init(config);
    luaL_propagate(L);
    if (validateConfig(L, *config) == LUA_RAISED)
        return LUA_RAISED;

    lua_pushlightuserdata(L, ctx);
    luaL_propagate(L);

    // require-like closure captures config and ctx as upvalues
    lua_pushcclosurek(L, requirelikefunc, debugname, 2, Luau::Require::lua_requirecont);
    luaL_propagate(L);
    return 1;
}

int luarequire_pushrequire(lua_State* L, luarequire_Configuration_init config_init, void* ctx)
{
    return pushrequireclosureinternal(L, config_init, ctx, Luau::Require::lua_require, "require");
}

void luaopen_require(lua_State* L, luarequire_Configuration_init config_init, void* ctx)
{
    if (luarequire_pushrequire(L, config_init, ctx) == LUA_RAISED)
        return;
    lua_setglobal(L, "require");
}

int luarequire_pushproxyrequire(lua_State* L, luarequire_Configuration_init config_init, void* ctx)
{
    return pushrequireclosureinternal(L, config_init, ctx, Luau::Require::lua_proxyrequire, "proxyrequire");
}

int luarequire_registermodule(lua_State* L)
{
    return Luau::Require::registerModuleImpl(L);
}

int luarequire_clearcacheentry(lua_State* L)
{
    return Luau::Require::clearCacheEntry(L);
}

int luarequire_clearcache(lua_State* L)
{
    return Luau::Require::clearCache(L);
}

void luarequire_lockplaceholder(lua_State* L, int idx)
{
    Luau::Require::lockPlaceholder(L, idx);
}

void luarequire_populateplaceholder(lua_State* L, int placeholderIdx, int resultIdx)
{
    Luau::Require::populatePlaceholder(L, placeholderIdx, resultIdx);
}

void luarequire_createplaceholder(lua_State* L)
{
    Luau::Require::createPlaceholder(L);
}
