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

void ssockaux_data(struct ssockaux *self, void *data, int dlen) {
	struct ssockaux *aux = self;
	lua_State *L = aux->L;
	lua_getglobal(L, gkey);
	lua_getfield(L, -1, "data");
	if (lua_isfunction(L, -1)) {
		lua_pushlstring(L, data, dlen);
		int status = lua_pcall(L, 1, 0, 0);
		if (status == LUA_OK) {
		}
	}
}

int ssockaux_write(struct ssockaux *self, void *data, int dlen) {
	struct ssockaux *aux = self;
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

int ssockaux_shutdown(struct ssockaux *self, int how) {
	struct ssockaux *aux = self;
	lua_State *L = aux->L;
	lua_getglobal(L, gkey);
	lua_getfield(L, -1, "shutdown");
	if (lua_isfunction(L, -1)) {
		lua_pushinteger(L, how);
		lua_pcall(L, 1, 0, 0);
	}
	return 0;
}

int ssockaux_close(struct ssockaux *self) {
	struct ssockaux *aux = self;
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

int onrecv(void *data, int dlen, void *ud) {
	//char *test = "hello world.";
	return 0;
}

static int
lssockaux_alloc(lua_State *L) {

	struct ssockaux *aux = lua_newuserdata(L, sizeof(struct ssockaux));
	aux->L = L;
	aux->fd = ssock_alloc(onrecv, aux);

	if (lua_gettop(L) > 1) {
		luaL_checktype(L, 1, LUA_TTABLE);
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
	char *buf = luaL_checklstring(L, 2, &l);
	if (lua_gettop(L) > 2) {
		luaL_checktype(L, 3, LUA_TFUNCTION);
		lua_getglobal(L, gkey);
		if (lua_istable(L, -1)) {
		} else {
			lua_newtable(L);
			lua_setglobal(L, gkey);
		}
		lua_pushvalue(L, 3);
		lua_setfield(L, -2, gkey_data);
	}

	int r = ssock_poll(aux->fd, buf, l);
	lua_pushinteger(L, r);
	return 1;
}

static int
lssockaux_send(lua_State *L) {
	struct ssockaux *aux = lua_touserdata(L, 1);
	size_t l = 0;
	char *buf = luaL_checklstring(L, 2, &l);
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
		{ "update", lssockaux_update },
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

