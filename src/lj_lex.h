/*
** Lexical analyzer.
** Copyright (C) 2005-2023 Mike Pall. See Copyright Notice in luajit.h
*/

#ifndef _LJ_LEX_H
#define _LJ_LEX_H

#include <stdarg.h>
#include <stdint.h>

#include "lj_obj.h"
#include "lj_err.h"

/* Lua lexer tokens. */
#define TKDEF(_, __) \
  _(and) _(break) _(do) _(else) _(elseif) _(end) _(false) \
  _(for) _(function) _(fn) _(goto) _(if) _(in) _(local) _(nil) _(not) _(or) \
  _(repeat) _(return) _(then) _(true) _(until) _(while) _(operator) _(using) _(nameof) \
  __(pow, ^^) __(concat, ..) __(dots, ...) __(eq, ==) __(ge, >=) __(le, <=) __(ne, ~=) __(subs, =~)\
  __(label, ::) __(number, <number>) __(name, <name>) __(string, <string>) \
  __(subval, <substituted value>) __(oper, <operator>) __(fldoper, <field operator>) __(eof, <eof>)

enum {
  TK_OFS = 256,
#define TKENUM1(name)		TK_##name,
#define TKENUM2(name, sym)	TK_##name,
TKDEF(TKENUM1, TKENUM2)
#undef TKENUM1
#undef TKENUM2
  TK_RESERVED = TK_nameof - TK_OFS
};

#define lj_lex_token2reserved(tok) ((uint8_t)(tok - TK_OFS))

typedef int LexChar;	/* Lexical character. Unsigned ext. from char. */
typedef int LexToken;	/* Lexical token. */

/* Combined bytecode ins/line. Only used during bytecode generation. */
typedef struct BCInsLine {
  BCIns ins;		/* Bytecode instruction. */
  BCLine line;		/* Line number for this bytecode. */
} BCInsLine;

/* Info for local variables. Only used during bytecode generation. */
typedef struct VarInfo {
  GCRef name;		/* Local variable name or goto/label name. */
  BCPos startpc;	/* First point where the local variable is active. */
  BCPos endpc;		/* First point where the local variable is dead. */
  uint8_t slot;		/* Variable slot. */
  uint8_t info;		/* Variable/goto/label info. */
} VarInfo;

/* Kinds of symbol mangling. */
typedef enum LexMKind {
  MK_none = 0,
  MK_unknown,
  MK_oper,
  MK_index,
  MK_newindex,
  MK_vname
} LexMKind;

/* Lua lexer state. */
typedef struct LexState {
  struct FuncState *fs;	/* Current FuncState. Defined in lj_parse.c. */
  struct lua_State *L;	/* Lua state. */
  TValue tokval;	/* Current token value. */
  TValue lookaheadval;	/* Lookahead token value. */
  const char *p;	/* Current position in input buffer. */
  const char *pe;	/* End of input buffer. */
  LexChar c;		/* Current character. */
  LexToken tok;		/* Current token. */
  LexToken lookahead;	/* Lookahead token. */
  SBuf sb;		/* String buffer for tokens. */
  lua_Reader rfunc;	/* Reader callback. */
  void *rdata;		/* Reader callback data. */
  BCLine linenumber;	/* Input line counter. */
  BCLine lastline;	/* Line of last token. */
  GCstr *chunkname;	/* Current chunk name (interned string). */
  const char *chunkarg;	/* Chunk name argument. */
  const char *mode;	/* Allow loading bytecode (b) and/or source text (t). */
  VarInfo *vstack;	/* Stack for names and extents of local variables. */
  MSize sizevstack;	/* Size of variable stack. */
  MSize vtop;		/* Top of variable stack. */
  BCInsLine *bcstack;	/* Stack for bytecode instructions/line numbers. */
  MSize sizebcstack;	/* Size of bytecode stack. */
  uint32_t level;	/* Syntactical nesting level. */
  int endmark;		/* Trust bytecode end marker, even if not at EOF. */
  int fr2;		/* Generate bytecode for LJ_FR2 mode. */
  int xsub;		/* Non-zero if substitution operator is in attention. */
  void *xbase;		/* Base value for substitution. */
  void *xkey;		/* Key value for substitution. */
  LexMKind mkind;	/* Kind of mangling for next symbol name. */
} LexState;

LJ_FUNC int lj_lex_setup(lua_State *L, LexState *ls);
LJ_FUNC void lj_lex_cleanup(lua_State *L, LexState *ls);
LJ_FUNC void lj_lex_next(LexState *ls);
LJ_FUNC LexToken lj_lex_lookahead(LexState *ls);
LJ_FUNC const char *lj_lex_token2str(LexState *ls, LexToken tok);
LJ_FUNC_NORET void lj_lex_error(LexState *ls, LexToken tok, ErrMsg em, ...);
LJ_FUNC void lj_lex_init(lua_State *L);


#ifdef LUA_USE_ASSERT
#define lj_assertLS(c, ...)	(lj_assertG_(G(ls->L), (c), __VA_ARGS__))
#else
#define lj_assertLS(c, ...)	((void)ls)
#endif

/* Symbol mangling headers */

/* for infix and postfix symbolic operators, MK_oper
 * also for field operators, MK_index
 */
#define LUAR_OPHDR ("__LRop_")
#define LUAR_OPHDR_LEN (sizeof(LUAR_OPHDR)-1)

/* for field operators that handle assignment, MK_newindex */
#define LUAR_AOPHDR ("__LRaop_")
#define LUAR_AOPHDR_LEN (sizeof(LUAR_AOPHDR)-1)

/* for operators with variable-like names, MK_vname */
#define LUAR_VAROPHDR ("__LRvop_")
#define LUAR_VAROPHDR_LEN (sizeof(LUAR_VAROPHDR)-1)

#endif
