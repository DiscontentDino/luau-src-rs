// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#include "lualib.h"

#include "lcommon.h"

#include <string.h>
#include <time.h>

#define LUA_STRFTIMEOPTIONS "aAbBcdHIjmMpSUwWxXyYzZ%"

#if defined(_WIN32)
static tm* gmtime_r(const time_t* timep, tm* result)
{
    return gmtime_s(result, timep) == 0 ? result : NULL;
}

static tm* localtime_r(const time_t* timep, tm* result)
{
    return localtime_s(result, timep) == 0 ? result : NULL;
}
#endif

static time_t os_timegm(struct tm* timep)
{
    // Julian day number calculation
    int day = timep->tm_mday;
    int month = timep->tm_mon + 1;
    int year = timep->tm_year + 1900;

    // year adjustment, pretend that it starts in March
    int a = timep->tm_mon % 12 < 2 ? 1 : 0;

    // also adjust for out-of-range month numbers in input
    a -= timep->tm_mon / 12;

    int y = year + 4800 - a;
    int m = month + (12 * a) - 3;

    int julianday = day + ((153 * m + 2) / 5) + (365 * y) + (y / 4) - (y / 100) + (y / 400) - 32045;

    const int utcstartasjulianday = 2440588;                              // Jan 1st 1970 offset in Julian calendar
    const int64_t utcstartasjuliansecond = utcstartasjulianday * 86400ll; // same in seconds

    // fail the dates before UTC start
    if (julianday < utcstartasjulianday)
        return time_t(-1);

    int64_t daysecond = timep->tm_hour * 3600ll + timep->tm_min * 60ll + timep->tm_sec;
    int64_t julianseconds = int64_t(julianday) * 86400ull + daysecond;

    if (julianseconds < utcstartasjuliansecond)
        return time_t(-1);

    int64_t utc = julianseconds - utcstartasjuliansecond;
    return time_t(utc);
}

static int os_clock(lua_State* L)
{
    lua_pushnumber(L, lua_clock());
    return 1;
}

/*
** {======================================================
** Time/Date operations
** { year=%Y, month=%m, day=%d, hour=%H, min=%M, sec=%S,
**   wday=%w+1, yday=%j, isdst=? }
** =======================================================
*/

static void setfield(lua_State* L, const char* key, int value)
{
    lua_pushinteger(L, value);
    lua_setfield(L, -2, key);
}

static void setboolfield(lua_State* L, const char* key, int value)
{
    if (value < 0) // undefined?
        return;    // does not set field
    lua_pushboolean(L, value);
    lua_setfield(L, -2, key);
}

static bool getboolfield(lua_State* L, const char* key, int* out)
{
    int res;
    lua_rawgetfield(L, -1, key);
    if (lua_haserror(L))
        return false;
    res = lua_isnil(L, -1) ? -1 : lua_toboolean(L, -1);
    lua_pop(L, 1);
    *out = res;
    return true;
}

static bool getfield(lua_State* L, const char* key, int d, int* out)
{
    int res;
    lua_rawgetfield(L, -1, key);
    if (lua_haserror(L))
        return false;
    if (lua_isnumber(L, -1))
        res = (int)lua_tointeger(L, -1);
    else
    {
        if (d < 0)
        {
            luaL_raiseerror(L, "field '%s' missing in date table", key);
            return false;
        }
        res = d;
    }
    lua_pop(L, 1);
    *out = res;
    return true;
}

static int os_date(lua_State* L)
{
    const char* s;
    if (!luaL_requireoptstring(L, 1, "%c", &s))
        return LUA_RAISED;
    time_t t = time(NULL);
    if (!lua_isnoneornil(L, 2))
    {
        double n;
        if (!luaL_requirenumber(L, 2, &n))
            return LUA_RAISED;
        t = (time_t)n;
    }

    struct tm tm;
    struct tm* stm;
    if (*s == '!')
    { // UTC?
        stm = gmtime_r(&t, &tm);
        s++; // skip `!'
    }
    else
    {
        // on Windows, localtime() fails with dates before epoch start so we disallow that
        stm = t < 0 ? NULL : localtime_r(&t, &tm);
    }

    if (stm == NULL) // invalid date?
    {
        lua_pushnil(L);
    }
    else if (strcmp(s, "*t") == 0)
    {
        lua_createtable(L, 0, 9); // 9 = number of fields
        luaL_propagate(L);
        setfield(L, "sec", stm->tm_sec);
        luaL_propagate(L);
        setfield(L, "min", stm->tm_min);
        luaL_propagate(L);
        setfield(L, "hour", stm->tm_hour);
        luaL_propagate(L);
        setfield(L, "day", stm->tm_mday);
        luaL_propagate(L);
        setfield(L, "month", stm->tm_mon + 1);
        luaL_propagate(L);
        setfield(L, "year", stm->tm_year + 1900);
        luaL_propagate(L);
        setfield(L, "wday", stm->tm_wday + 1);
        luaL_propagate(L);
        setfield(L, "yday", stm->tm_yday + 1);
        luaL_propagate(L);
        setboolfield(L, "isdst", stm->tm_isdst);
    }
    else
    {
        char cc[3];
        cc[0] = '%';
        cc[2] = '\0';

        luaL_Strbuf b;
        luaL_buffinit(L, &b);
        for (; *s; s++)
        {
            if (*s != '%' || *(s + 1) == '\0') // no conversion specifier?
            {
                luaL_addchar(&b, *s);
            }
            else if (strchr(LUA_STRFTIMEOPTIONS, *(s + 1)) == 0)
            {
                return luaL_raiseargerror(L, 1, "invalid conversion specifier");
            }
            else
            {
                size_t reslen;
                char buff[200]; // should be big enough for any conversion result
                cc[1] = *(++s);
                reslen = strftime(buff, sizeof(buff), cc, stm);
                luaL_addlstring(&b, buff, reslen);
            }
            luaL_propagate(L);
        }
        luaL_pushresult(&b);
    }
    return 1;
}

static int os_time(lua_State* L)
{
    time_t t;
    if (lua_isnoneornil(L, 1)) // called without args?
        t = time(NULL);        // get current time
    else
    {
        struct tm ts;
        if (!luaL_requiretype(L, 1, LUA_TTABLE))
            return LUA_RAISED;
        lua_settop(L, 1); // make sure table is at the top
        int month, year;
        if (!getfield(L, "sec", 0, &ts.tm_sec) || !getfield(L, "min", 0, &ts.tm_min) || !getfield(L, "hour", 12, &ts.tm_hour) ||
            !getfield(L, "day", -1, &ts.tm_mday) || !getfield(L, "month", -1, &month) || !getfield(L, "year", -1, &year) ||
            !getboolfield(L, "isdst", &ts.tm_isdst))
            return LUA_RAISED;
        ts.tm_mon = month - 1;
        ts.tm_year = year - 1900;

        // Note: upstream Lua uses mktime() here which assumes input is local time, but we prefer UTC for consistency
        t = os_timegm(&ts);
    }
    if (t == (time_t)(-1))
        lua_pushnil(L);
    else
        lua_pushnumber(L, (double)t);
    return 1;
}

static int os_difftime(lua_State* L)
{
    double t1, t2;
    if (!luaL_requirenumber(L, 1, &t1) || !luaL_requireoptnumber(L, 2, 0, &t2))
        return LUA_RAISED;
    lua_pushnumber(L, difftime((time_t)(t1), (time_t)(t2)));
    return 1;
}

static const luaL_Reg syslib[] = {
    {"clock", os_clock},
    {"date", os_date},
    {"difftime", os_difftime},
    {"time", os_time},
    {NULL, NULL},
};

int luaopen_os(lua_State* L)
{
    luaL_register(L, LUA_OSLIBNAME, syslib);
    return 1;
}
