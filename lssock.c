#include "lssock.h"
#include "ssock.h"

#include <lua.h>
#include <lauxlib.h>

struct ssockaux {
	lua_State *L;
	struct ssock *fd;
};

static void onrecv(void *data, int dlen, void *ud) {
	struct ssockaux *aux = ud;
	lua_State *L = aux->L;
	lua_getglobal(L, "ssockaux");
	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, "onrecv");
		lua_pushlstring(L, data, dlen);
		lua_pcall(L, 1, 0, 0);
	}
}

static int
lssockaux_alloc(lua_State *L) {

	struct ssockaux *aux = lua_newuserdata(L, sizeof(struct ssockaux));
	aux->L = L;
	aux->fd = ssock_alloc(onrecv, aux);

	if (lua_gettop(L) > 1 && lua_isfunction(L, 1)) {
		lua_createtable(L, 0, 1);
		lua_pushstring(L, "onrecv");
		lua_pushvalue(L, 1);
		lua_rawset(L, -3);

		lua_setglobal(L, "ssockaux");
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
lssockaux_poll(lua_State *L) {
	struct ssockaux *aux = lua_touserdata(L, 1);
	size_t l = 0;
	const char *buf = luaL_checklstring(L, 2, &l);
	int r = ssock_poll(aux->fd, buf, l);
	lua_pushinteger(L, r);
	return 1;
}

static int
lssockaux_send(lua_State *L) {
	struct ssockaux *aux = lua_touserdata(L, 1);
	size_t l = 0;
	const char *buf = luaL_checklstring(L, 2, &l);
	int r = ssock_send(aux->fd, buf, l);
	lua_pushinteger(L, r);
	return 1;
}

static int
lssockaux_shutdown(lua_State *L) {
	struct ssockaux *aux = lua_touserdata(L, 1);
	int how = luaL_checkinteger(L, 2);
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
	luaL_Reg l[] = {
		{ "free", lssockaux_free },
		{ "connected", lssockaux_connected },
		{ "poll", lssockaux_poll },
		{ "send", lssockaux_send },
		{ "shutdown", lssockaux_shutdown },
		{ "close", lssockaux_close },
		{ NULL, NULL },
	};
	luaL_newlib(L, l); // met
	lua_pushcclosure(L, lssockaux_alloc, 1);
	return 1;
}

