// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details

#include "RequireImpl.h"

#include "Navigation.h"

#include "Luau/RequireNavigator.h"
#include "Luau/Require.h"

#include "lua.h"
#include "lualib.h"

LUAU_FASTFLAGVARIABLE(LuauCyclicRequireShortCircuit)

#define REQUIRE_PROPAGATE(L, ...) \
    do \
    { \
        if (lua_haserror(L)) \
            return __VA_ARGS__; \
    } while (0)

namespace Luau::Require
{

// Stores explicitly registered modules.
static const char* registeredCacheTableKey = "_REGISTEREDMODULES";

// Stores the results of require calls.
static const char* requiredCacheTableKey = "_MODULES";

// Sentinel address used as a unique registry key for the shared placeholder metatable.
static char cyclicPlaceholderMetatableSentinel = 0;

// Tracks which placeholders were actually returned to a cyclic requirer.
static const char* cyclicPlaceholderProvidedKey = "_CYCLIC_PLACEHOLDER_PROVIDED";

struct ResolvedRequire
{
    static ResolvedRequire fromErrorHandler(const RuntimeErrorHandler& errorHandler)
    {
        return {ResolvedRequire::Status::ErrorReported, "", "", "", errorHandler.getReportedError()};
    }

    static ResolvedRequire fromErrorMessage(const char* message)
    {
        return {ResolvedRequire::Status::ErrorReported, "", "", "", message};
    }

    static ResolvedRequire fromRaisedError()
    {
        return {ResolvedRequire::Status::ErrorRaised};
    }

    enum class Status
    {
        Cached,
        ModuleRead,
        ErrorReported,
        ErrorRaised,
    };

    Status status;
    std::string chunkname;
    std::string loadname;
    std::string cacheKey;
    std::string error;
};

static int isCached(lua_State* L, const std::string& key)
{
    luaL_findtable(L, LUA_REGISTRYINDEX, requiredCacheTableKey, 1);
    REQUIRE_PROPAGATE(L, LUA_RAISED);
    lua_getfield(L, -1, key.c_str());
    REQUIRE_PROPAGATE(L, LUA_RAISED);
    bool cached = !lua_isnil(L, -1);

    if (FFlag::LuauCyclicRequireShortCircuit && cached && lua_istable(L, -1))
    {
        // Check if the cached value is a placeholder (has the shared placeholder metatable).
        if (lua_getmetatable(L, -1) == 1)
        {
            lua_rawgetp(L, LUA_REGISTRYINDEX, &cyclicPlaceholderMetatableSentinel);
            REQUIRE_PROPAGATE(L, LUA_RAISED);
            if (lua_rawequal(L, -1, -2) == 1)
            {
                // A cyclic require is accessing this placeholder — mark it as provided.
                luaL_findtable(L, LUA_REGISTRYINDEX, cyclicPlaceholderProvidedKey, 1);
                REQUIRE_PROPAGATE(L, LUA_RAISED);
                lua_pushboolean(L, 1);
                REQUIRE_PROPAGATE(L, LUA_RAISED);
                lua_setfield(L, -2, key.c_str());
                REQUIRE_PROPAGATE(L, LUA_RAISED);
                lua_pop(L, 1);
            }
            lua_pop(L, 2); // pop both metatables
        }
        REQUIRE_PROPAGATE(L, LUA_RAISED);
    }

    lua_pop(L, 2);

    return cached ? 1 : 0;
}

static ResolvedRequire resolveRequire(luarequire_Configuration* lrc, lua_State* L, void* ctx, const char* requirerChunkname, std::string path)
{
    bool allowed = lrc->is_require_allowed(L, ctx, requirerChunkname);
    REQUIRE_PROPAGATE(L, ResolvedRequire::fromRaisedError());
    if (!allowed)
        return ResolvedRequire::fromErrorMessage("require is not supported in this context");

    RuntimeNavigationContext navigationContext{lrc, L, ctx, requirerChunkname};
    RuntimeErrorHandler errorHandler{path};

    Navigator navigator(navigationContext, errorHandler);

    // Updates navigationContext while navigating through the given path.
    Navigator::Status status = navigator.navigate(std::move(path));
    REQUIRE_PROPAGATE(L, ResolvedRequire::fromRaisedError());
    if (status == Navigator::Status::ErrorReported)
        return ResolvedRequire::fromErrorHandler(errorHandler);

    bool present = navigationContext.isModulePresent();
    REQUIRE_PROPAGATE(L, ResolvedRequire::fromRaisedError());
    if (!present)
        return ResolvedRequire::fromErrorMessage("no module present at resolved path");

    std::optional<std::string> cacheKey = navigationContext.getCacheKey();
    REQUIRE_PROPAGATE(L, ResolvedRequire::fromRaisedError());
    if (!cacheKey)
        return ResolvedRequire::fromErrorMessage("could not get cache key for module");

    int cached = isCached(L, *cacheKey);
    if (cached == LUA_RAISED)
        return ResolvedRequire::fromRaisedError();

    if (cached)
    {
        // Put cached result on top of stack before returning.
        lua_getfield(L, LUA_REGISTRYINDEX, requiredCacheTableKey);
        REQUIRE_PROPAGATE(L, ResolvedRequire::fromRaisedError());
        lua_getfield(L, -1, cacheKey->c_str());
        REQUIRE_PROPAGATE(L, ResolvedRequire::fromRaisedError());
        lua_remove(L, -2);

        return ResolvedRequire{ResolvedRequire::Status::Cached};
    }

    std::optional<std::string> chunkname = navigationContext.getChunkname();
    REQUIRE_PROPAGATE(L, ResolvedRequire::fromRaisedError());
    if (!chunkname)
        return ResolvedRequire::fromErrorMessage("could not get chunkname for module");

    std::optional<std::string> loadname = navigationContext.getLoadname();
    REQUIRE_PROPAGATE(L, ResolvedRequire::fromRaisedError());
    if (!loadname)
        return ResolvedRequire::fromErrorMessage("could not get loadname for module");

    return ResolvedRequire{
        ResolvedRequire::Status::ModuleRead,
        std::move(*chunkname),
        std::move(*loadname),
        std::move(*cacheKey),
    };
}

static int checkRegisteredModules(lua_State* L, const char* path)
{
    luaL_findtable(L, LUA_REGISTRYINDEX, registeredCacheTableKey, 1);
    REQUIRE_PROPAGATE(L, LUA_RAISED);

    std::string pathLower = std::string(path);
    for (char& c : pathLower)
    {
        if (c >= 'A' && c <= 'Z')
            c -= ('A' - 'a');
    }

    lua_getfield(L, -1, pathLower.c_str());
    REQUIRE_PROPAGATE(L, LUA_RAISED);
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 2);
        return 0;
    }

    lua_remove(L, -2);
    return 1;
}

static int CyclicDependencyIndexError(lua_State* L)
{
    const char* key = lua_tostring(L, 2);
    return luaL_raiseerror(
        L, "Cannot access the exported field '%s' because it has a cyclic dependency on its requiring module", key ? key : "unknown"
    );
}

static int CyclicDependencyNewIndexError(lua_State* L)
{
    const char* key = lua_tostring(L, 2);
    return luaL_raiseerror(
        L, "Cannot set the exported field '%s' because it has a cyclic dependency on its requiring module", key ? key : "unknown"
    );
}

// Returns the shared placeholder metatable, creating it on first use.
static void pushCyclicPlaceholderMetatable(lua_State* L)
{
    lua_rawgetp(L, LUA_REGISTRYINDEX, &cyclicPlaceholderMetatableSentinel);
    REQUIRE_PROPAGATE(L);
    if (!lua_isnil(L, -1))
        return;
    lua_pop(L, 1);

    lua_newtable(L);
    REQUIRE_PROPAGATE(L);
    lua_pushcfunction(L, CyclicDependencyIndexError, "CyclicDependencyIndexError");
    REQUIRE_PROPAGATE(L);
    lua_setfield(L, -2, "__index");
    REQUIRE_PROPAGATE(L);
    lua_pushcfunction(L, CyclicDependencyNewIndexError, "CyclicDependencyNewIndexError");
    REQUIRE_PROPAGATE(L);
    lua_setfield(L, -2, "__newindex");
    REQUIRE_PROPAGATE(L);
    lua_pushliteral(L, "The metatable is locked");
    REQUIRE_PROPAGATE(L);
    lua_setfield(L, -2, "__metatable");
    REQUIRE_PROPAGATE(L);

    lua_pushvalue(L, -1);
    REQUIRE_PROPAGATE(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &cyclicPlaceholderMetatableSentinel);
}

void lockPlaceholder(lua_State* L, int idx)
{
    idx = lua_absindex(L, idx);
    pushCyclicPlaceholderMetatable(L);
    REQUIRE_PROPAGATE(L);
    lua_setmetatable(L, idx);
    lua_setreadonly(L, idx, 1);
}

void createPlaceholder(lua_State* L)
{
    const char* cacheKey;
    if (!luaL_requirestring(L, 2, &cacheKey))
        return;

    lua_newtable(L);
    REQUIRE_PROPAGATE(L);
    lockPlaceholder(L, -1);
    REQUIRE_PROPAGATE(L);

    luaL_findtable(L, LUA_REGISTRYINDEX, requiredCacheTableKey, 1);
    REQUIRE_PROPAGATE(L);
    lua_pushvalue(L, -2);
    REQUIRE_PROPAGATE(L);
    lua_setfield(L, -2, cacheKey);
    REQUIRE_PROPAGATE(L);
    lua_pop(L, 2);
}

void populatePlaceholder(lua_State* L, int placeholderIdx, int resultIdx)
{
    placeholderIdx = lua_absindex(L, placeholderIdx);
    resultIdx = lua_absindex(L, resultIdx);

    // Unfreeze so we can write to the placeholder
    lua_setreadonly(L, placeholderIdx, 0);

    // Copy all fields from the result table into the placeholder
    for (int iter = 0; (iter = lua_rawiter(L, resultIdx, iter)) != -1;)
    {
        REQUIRE_PROPAGATE(L);
        // key at -2, value at -1
        lua_rawset(L, placeholderIdx);
        REQUIRE_PROPAGATE(L);
    }
    REQUIRE_PROPAGATE(L);

    // Copy the metatable from the result (if any)
    if (lua_getmetatable(L, resultIdx) == 0)
        lua_pushnil(L);
    REQUIRE_PROPAGATE(L);
    lua_setmetatable(L, placeholderIdx);

    // Freeze the populated placeholder.
    lua_setreadonly(L, placeholderIdx, 1);
}

// Fixed stack slots below the load results:
//   (1) path, (2) cacheKey, (3) chunkname, (4) loadname
static const int kRequireStackValues = 4;

int lua_requirecont(lua_State* L, int status)
{
    LUAU_ASSERT(lua_gettop(L) >= kRequireStackValues);
    const int numResults = lua_gettop(L) - kRequireStackValues;
    const char* cacheKey;
    if (!luaL_requirestring(L, 2, &cacheKey))
        return LUA_RAISED;

    if (numResults > 1)
        return luaL_raiseerror(L, "module must return a single value");

    if (FFlag::LuauCyclicRequireShortCircuit && numResults == 1)
    {
        const int resultIdx = kRequireStackValues + 1;

        // Check if the placeholder was actually provided to a cyclic requirer.
        luaL_findtable(L, LUA_REGISTRYINDEX, cyclicPlaceholderProvidedKey, 1);
        luaL_propagate(L);
        lua_getfield(L, -1, cacheKey);
        luaL_propagate(L);
        bool wasProvided = lua_toboolean(L, -1) != 0;
        lua_pop(L, 1);

        // Clear the placeholder from the cache to reset the state.
        lua_pushnil(L);
        luaL_propagate(L);
        lua_setfield(L, -2, cacheKey);
        luaL_propagate(L);
        lua_pop(L, 1);

        if (wasProvided)
        {
            LUAU_ASSERT(lua_istable(L, resultIdx));

            // Retrieve the placeholder from the cache.
            luaL_findtable(L, LUA_REGISTRYINDEX, requiredCacheTableKey, 1);
            luaL_propagate(L);
            lua_getfield(L, -1, cacheKey);
            luaL_propagate(L);

            // Populate the placeholder with the module's result.
            // Cyclic importers already hold the placeholder, so they see the result.
            populatePlaceholder(L, -1, resultIdx);
            luaL_propagate(L);

            // Replace the result with the populated placeholder so the initial caller
            // gets the same object that cyclic importers and future cache hits receive.
            lua_replace(L, resultIdx);
            lua_pop(L, 1);
        }
        else
        {
            // Cache the result normally (no cycle occurred).
            luaL_findtable(L, LUA_REGISTRYINDEX, requiredCacheTableKey, 1);
            luaL_propagate(L);
            lua_pushvalue(L, resultIdx);
            luaL_propagate(L);
            lua_setfield(L, -2, cacheKey);
            luaL_propagate(L);
            lua_pop(L, 1);
        }
    }
    else if (numResults == 1)
    {
        // Initial stack state
        // (-1) result
        lua_getfield(L, LUA_REGISTRYINDEX, requiredCacheTableKey);
        luaL_propagate(L);
        // (-2) result, (-1) cache table

        lua_pushvalue(L, -2);
        luaL_propagate(L);
        // (-3) result, (-2) cache table, (-1) result

        lua_setfield(L, -2, cacheKey);
        luaL_propagate(L);
        // (-2) result, (-1) cache table

        lua_pop(L, 1);
        // (-1) result
    }

    return numResults;
}

int lua_requireinternal(lua_State* L, const char* requirerChunkname)
{
    // Discard extra arguments, we only use path
    lua_settop(L, 1);

    luarequire_Configuration* lrc = static_cast<luarequire_Configuration*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!lrc)
        return luaL_raiseerror(L, "unable to find require configuration");

    void* ctx = lua_tolightuserdata(L, lua_upvalueindex(2));

    // (1) path
    const char* path;
    if (!luaL_requirestring(L, 1, &path))
        return LUA_RAISED;

    int registered = checkRegisteredModules(L, path);
    if (registered == LUA_RAISED)
        return LUA_RAISED;
    if (registered == 1)
        return 1;

    // ResolvedRequire will be destroyed and any string will be pinned to Luau stack, so that luaL_error doesn't need destructors
    bool resolveError = false;

    {
        ResolvedRequire resolvedRequire = resolveRequire(lrc, L, ctx, requirerChunkname, path);

        if (resolvedRequire.status == ResolvedRequire::Status::ErrorRaised)
            return LUA_RAISED;

        if (resolvedRequire.status == ResolvedRequire::Status::Cached)
            return 1;

        if (resolvedRequire.status == ResolvedRequire::Status::ErrorReported)
        {
            lua_pushstring(L, resolvedRequire.error.c_str());
            resolveError = true;
        }
        else
        {
            // (1) path, ..., cacheKey, chunkname, loadname
            lua_pushstring(L, resolvedRequire.cacheKey.c_str());
            luaL_propagate(L);
            lua_pushstring(L, resolvedRequire.chunkname.c_str());
            luaL_propagate(L);
            lua_pushstring(L, resolvedRequire.loadname.c_str());
        }
        luaL_propagate(L);
    }

    if (resolveError)
        return lua_raiseerror(L); // Error already on top of the stack

    const char* chunkname = lua_tostring(L, 3);
    const char* loadname = lua_tostring(L, 4);

    LUAU_ASSERT(lua_gettop(L) == kRequireStackValues);

    int numResults = lrc->load(L, ctx, path, chunkname, loadname);
    luaL_propagate(L);
    if (numResults == -1)
    {
        if (lua_gettop(L) != kRequireStackValues)
            return luaL_raiseerror(L, "stack cannot be modified when require yields");

        return lua_yield(L, 0);
    }

    return lua_requirecont(L, LUA_OK);
}

int lua_proxyrequire(lua_State* L)
{
    const char* requirerChunkname;
    if (!luaL_requirestring(L, 2, &requirerChunkname))
        return LUA_RAISED;
    return lua_requireinternal(L, requirerChunkname);
}

int lua_require(lua_State* L)
{
    lua_Debug ar;
    int level = 1;

    do
    {
        int found = lua_getinfo(L, level++, "s", &ar);
        luaL_propagate(L);
        if (!found)
            return luaL_raiseerror(L, "require is not supported in this context");
    } while (ar.what[0] == 'C');

    return lua_requireinternal(L, ar.source);
}

int registerModuleImpl(lua_State* L)
{
    if (lua_gettop(L) != 2)
        return luaL_raiseerror(L, "expected 2 arguments: aliased require path and desired result");

    size_t len;
    const char* path;
    if (!luaL_requirelstring(L, 1, &path, &len))
        return LUA_RAISED;
    std::string_view pathView(path, len);
    if (pathView.empty() || pathView[0] != '@')
        return luaL_raiseargerrorL(L, 1, "path must begin with '@'");

    // Make path lowercase to ensure case-insensitive matching.
    std::string pathLower = std::string(path, len);
    for (char& c : pathLower)
    {
        if (c >= 'A' && c <= 'Z')
            c -= ('A' - 'a');
    }
    lua_pushstring(L, pathLower.c_str());
    luaL_propagate(L);
    lua_replace(L, 1);

    luaL_findtable(L, LUA_REGISTRYINDEX, registeredCacheTableKey, 1);
    luaL_propagate(L);
    // (1) path, (2) result, (3) cache table

    lua_insert(L, 1);
    // (1) cache table, (2) path, (3) result

    lua_settable(L, 1);
    luaL_propagate(L);
    // (1) cache table

    lua_pop(L, 1);

    return 0;
}

int clearCacheEntry(lua_State* L)
{
    const char* cacheKey;
    if (!luaL_requirestring(L, 1, &cacheKey))
        return LUA_RAISED;
    luaL_findtable(L, LUA_REGISTRYINDEX, requiredCacheTableKey, 1);
    luaL_propagate(L);
    lua_pushnil(L);
    luaL_propagate(L);
    lua_setfield(L, -2, cacheKey);
    luaL_propagate(L);
    lua_pop(L, 1);
    return 0;
}

int clearCache(lua_State* L)
{
    lua_newtable(L);
    luaL_propagate(L);
    lua_setfield(L, LUA_REGISTRYINDEX, requiredCacheTableKey);
    luaL_propagate(L);
    return 0;
}

} // namespace Luau::Require
