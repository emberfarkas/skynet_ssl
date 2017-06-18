#include "lssock.h"
#include "ssock.h"

#include <lua.h>
#include <lauxlib.h>
#include <assert.h>

const char *gkey = "ssockaux";
const char *gkey_data = "data";
const char *gkey_write = "write";
const char *gkey_shutdown = "shutdown";
const char *gkey_close = "close";


struct ssockaux {
	lua_State *L;
	struct ssock *fd;
};

/*
 * @breif 把所有数据都交出去，至于是否能接受，
*/
static void
ssockaux_data(const char *data, int dlen, void *ud) {
	struct ssockaux *aux = ud;
	lua_State *L = aux->L;
	lua_getglobal(L, gkey);
	lua_getfield(L, -1, gkey_data);
	if (lua_isfunction(L, -1)) {
		lua_pushlstring(L, data, dlen);
		int status = lua_pcall(L, 1, 0, 0);
		if (status == LUA_OK) {  // 当前线程OK
		}
	}
}

/*
 * @breif 把所有数据都写出去
 */
static int
ssockaux_write(const char *data, int dlen, void *ud) {
	struct ssockaux *aux = ud;
	lua_State *L = aux->L;
	if (dlen <= 0) {
		return 0;
	}
	assert(dlen > 0 && data != NULL);
	lua_getglobal(L, gkey);
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_getfield(L, -1, "write");
	if (lua_isfunction(L, -1)) {
		lua_pushlstring(L, data, dlen);
		int status = lua_pcall(L, 1, 1, 0);
		if (status == LUA_OK) {
			int r = (int)luaL_checknumber(L, -1);
			return r;
		}
	}
	return 0;
}

static int
ssockaux_shutdown(int how, void *ud) {
	struct ssockaux *aux = ud;
	lua_State *L = aux->L;
	lua_getglobal(L, gkey);
	lua_getfield(L, -1, "shutdown");
	if (lua_isfunction(L, -1)) {
		lua_pushinteger(L, how);
		lua_pcall(L, 1, 0, 0);
	}
	return 0;
}

static int
ssockaux_close(void *ud) {
	struct ssockaux *aux = ud;
	lua_State *L = aux->L;
	lua_getglobal(L, gkey);
	lua_getfield(L, -1, "close");
	if (lua_isfunction(L, -1)) {
		int status = lua_pcall(L, 0, 0, 0);
		if (status == LUA_OK) {
		}
	}
	return 0;
}

static int
lssockaux_alloc(lua_State *L) {

	struct ssockaux *aux = lua_newuserdata(L, sizeof(struct ssockaux));
	aux->L = L;

	struct ssock_cb cb;
	cb.write_callback = ssockaux_write;
	cb.data_callback = ssockaux_data;
	cb.shutdown_callback = ssockaux_shutdown;
	cb.close_callback = ssockaux_close;
	cb.ud = aux;
	aux->fd = ssock_alloc(&cb);

	if (lua_gettop(L) >= 1) {
		luaL_checktype(L, 1, LUA_TTABLE);
		lua_pushvalue(L, 1);
		lua_setglobal(L, gkey);
	}
	lua_pushvalue(L, lua_upvalueindex(1));
	lua_setmetatable(L, -2);
	return 1;
}

static int
lssockaux_free(lua_State *L) {
	struct ssockaux *aux = lua_touserdata(L, 1);
	ssock_free(aux->fd);
	return 0;
}

static int
lssockaux_connected(lua_State *L) {
	struct ssockaux *aux = lua_touserdata(L, 1);
	int b = ssock_connected(aux->fd);
	lua_pushboolean(L, b);
	return 1;
}

static int
lssockaux_connect(lua_State *L) {
	struct ssockaux *aux = lua_touserdata(L, 1);
	const char *addr = luaL_checkstring(L, 2);
	int port = luaL_checkinteger(L, 3);
	printf("%s:%d\n", addr, port);
	int r = ssock_connect(aux->fd, addr, port);
	lua_pushinteger(L, r);
	return 1;
}

static int
lssockaux_update(lua_State *L) {
	struct ssockaux *aux = lua_touserdata(L, 1);
	int r = ssock_update(aux->fd);
	lua_pushinteger(L, r);
	return 1;
}

static int
lssockaux_poll(lua_State *L) {
	struct ssockaux *aux = lua_touserdata(L, 1);
	size_t l = 0;
	const char *buf = luaL_checklstring(L, 2, &l);
	if (lua_isfunction(L, 3)) {
		lua_getglobal(L, gkey);
		if (!lua_istable(L, -1)) {
			lua_createtable(L, 0, 4);
			lua_setglobal(L, gkey);
		}
		lua_pushvalue(L, 3);
		lua_setfield(L, -2, gkey_data);
		lua_pop(L, 1);   // 还原
	}
	if (l > 0) {
		int r = ssock_poll(aux->fd, buf, l);
		lua_pushinteger(L, r);
	} else {
		lua_pushinteger(L, 0);
	}
	return 1;
}

static int
lssockaux_send(lua_State *L) {
	struct ssockaux *aux = lua_touserdata(L, 1);
	size_t l = 0;
	const char *buf = luaL_checklstring(L, 2, &l);
	if (lua_gettop(L) > 2) {
		luaL_checktype(L, 3, LUA_TFUNCTION);
		lua_getglobal(L, gkey);
		if (lua_istable(L, -1)) {
		} else {
			lua_newtable(L);
			lua_setglobal(L, gkey);
		}
		lua_pushvalue(L, 3);
		lua_setfield(L, -2, "write");
	}
	int r = ssock_send(aux->fd, buf, l);
	lua_pushinteger(L, r);
	return 1;
}

static int
lssockaux_shutdown(lua_State *L) {
	struct ssockaux *aux = lua_touserdata(L, 1);
	int how = luaL_checkinteger(L, 2);
	if (lua_gettop(L) > 2) {
		luaL_checktype(L, 3, LUA_TFUNCTION);
		lua_getglobal(L, gkey);
		if (lua_istable(L, -1)) {
		} else {
			lua_newtable(L);
			lua_setglobal(L, gkey);
		}
		lua_pushvalue(L, 3);
		lua_setfield(L, -2, gkey_shutdown);
	}
	int r = ssock_shutdown(aux->fd, how);
	lua_pushinteger(L, r);
	return 1;
}

static int
lssockaux_close(lua_State *L) {
	struct ssockaux *aux = lua_touserdata(L, 1);
	int r = ssock_close(aux->fd);
	lua_pushinteger(L, r);
	return 1;
}

LUAMOD_API int
luaopen_ssock(lua_State *L) {
	luaL_checkversion(L);
	lua_createtable(L, 0, 1);
	luaL_Reg l[] = {
		{ "connected", lssockaux_connected },
		{ "connect", lssockaux_connect },
		{ "poll", lssockaux_poll },
		{ "send", lssockaux_send },
		{ "shutdown", lssockaux_shutdown },
		{ "close", lssockaux_close },
		{ NULL, NULL },
	};
	luaL_newlib(L, l); // met
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, lssockaux_free);
	lua_setfield(L, -2, "__gc");

	lua_pushcclosure(L, lssockaux_alloc, 1);
	return 1;
}

