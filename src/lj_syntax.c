/*
** Language frontends manipulation.
*/

#define lj_syntax_c
#define LUA_CORE

#include "lj_obj.h"
#include "lj_lex.h"
#include "lj_parse.h"
#include "lj_syntax.h"

LUA_API int32_t lua_getsyntaxmode(lua_State *L)
{
  global_State *g = G(L);
  ParserState *ps = &g->pars;
  return ps->mode;
}

LUA_API int lua_setsyntaxmode(lua_State *L, int32_t mode)
{
  if (mode < 0 || mode > 1) {
    setstrV(L, L->top++, lj_err_str(L, LJ_ERR_NUMRNG));
    lj_err_throw(L, LUA_ERRRUN);
    return LUA_ERRRUN;
  }
  global_State *g = G(L);
  ParserState *ps = &g->pars;
  if (ps->mode == mode) return LUA_OK;
  ps->mode = mode;
  if (mode == 1) {
    ps->funcstr->reserved = 0;
    ps->end_str->reserved = 0;
    ps->fnstr->reserved = lj_lex_token2reserved(TK_function); /* so that TK_fn is unused in parser */
    ps->operstr->reserved = lj_lex_token2reserved(TK_operator);
    ps->usingstr->reserved = lj_lex_token2reserved(TK_using);
    ps->nameof_str->reserved = lj_lex_token2reserved(TK_nameof);
  } else {
    ps->funcstr->reserved = lj_lex_token2reserved(TK_function);
    ps->end_str->reserved = lj_lex_token2reserved(TK_end);
    ps->fnstr->reserved = 0;
    ps->operstr->reserved = 0;
    ps->usingstr->reserved = 0;
    ps->nameof_str->reserved = 0;
  }
  return LUA_OK;
}

LUA_API int32_t lua_getsyntaxautosel(lua_State *L)
{
  global_State *g = G(L);
  ParserState *ps = &g->pars;
  return ps->autoselect;
}

LUA_API void lua_setsyntaxautosel(lua_State *L, int32_t autoselect)
{
  global_State *g = G(L);
  ParserState *ps = &g->pars;
  ps->autoselect = autoselect != 0;
}
