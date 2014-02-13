
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     QSTRING = 258,
     ADDR = 259,
     MACADDR = 260,
     BOOL = 261,
     NUMBER = 262,
     DECIMAL = 263,
     NUMPAIR = 264,
     MIP6ENTITY = 265,
     DEBUGLEVEL = 266,
     DEBUGLOGFILE = 267,
     HOMEADDRESS = 268,
     HOMEAGENTADDRESS = 269,
     INITIALBINDACKTIMEOUTFIRSTREG = 270,
     INITIALBINDACKTIMEOUTREREG = 271,
     LINKNAME = 272,
     HAMAXBINDINGLIFE = 273,
     MNMAXHABINDINGLIFE = 274,
     MNMAXCNBINDINGLIFE = 275,
     MAXMOBPFXADVINTERVAL = 276,
     MINMOBPFXADVINTERVAL = 277,
     MNHOMELINK = 278,
     HAHOMELINK = 279,
     NONVOLATILEBINDINGCACHE = 280,
     SENDMOBPFXSOLS = 281,
     SENDUNSOLMOBPFXADVS = 282,
     SENDMOBPFXADVS = 283,
     IPSECPOLICYSET = 284,
     IPSECPOLICY = 285,
     IPSECTYPE = 286,
     USEALTCOA = 287,
     USEESP = 288,
     USEAH = 289,
     USEIPCOMP = 290,
     BLOCK = 291,
     USEMNHAIPSEC = 292,
     KEYMNGMOBCAPABILITY = 293,
     HOMEREGBINDING = 294,
     MH = 295,
     MOBPFXDISC = 296,
     TUNNELHOMETESTING = 297,
     TUNNELMH = 298,
     TUNNELPAYLOAD = 299,
     USEMOVEMENTMODULE = 300,
     USEPOLICYMODULE = 301,
     INTERNAL = 302,
     MNROPOLICY = 303,
     ICMP = 304,
     ANY = 305,
     DEFAULTBINDINGACLPOLICY = 306,
     BINDINGACLPOLICY = 307,
     MNADDRESS = 308,
     USECNBUACK = 309,
     INTERFACE = 310,
     IFNAME = 311,
     IFTYPE = 312,
     MNIFPREFERENCE = 313,
     MNUSEALLINTERFACES = 314,
     MNDISCARDHAPARAMPROB = 315,
     OPTIMISTICHANDOFF = 316,
     RFC5213TIMESTAMPBASEDAPPROACHINUSE = 317,
     RFC5213MOBILENODEGENERATEDTIMESTAMPINUSE = 318,
     RFC5213FIXEDMARLINKLOCALADDRESSONALLACCESSLINKS = 319,
     RFC5213FIXEDMARLINKLAYERADDRESSONALLACCESSLINKS = 320,
     RFC5213MINDELAYBEFOREBCEDELETE = 321,
     RFC5213MAXDELAYBEFORENEWBCEASSIGN = 322,
     RFC5213TIMESTAMPVALIDITYWINDOW = 323,
     RFC5213ENABLEMARLOCALROUTING = 324,
     DMMCMD = 325,
     DMMMAR = 326,
     PROXYDMMCMD = 327,
     PROXYDMMMAR = 328,
     CMDADDRESS = 329,
     CMDDMMNETWORKDEVICE = 330,
     CMDCORENETWORKADDRESS = 331,
     CMDCORENETWORKDEVICE = 332,
     MARADDRESSINGRESS = 333,
     MARADDRESSEGRESS = 334,
     MARDEVICEINGRESS = 335,
     MARDEVICEEGRESS = 336,
     OURADDRESS = 337,
     HOMENETWORKPREFIX = 338,
     PBULIFETIME = 339,
     PBALIFETIME = 340,
     RETRANSMISSIONTIMEOUT = 341,
     MAXMESSAGERETRANSMISSIONS = 342,
     TUNNELINGENABLED = 343,
     DYNAMICTUNNELINGENABLED = 344,
     PCAPSYSLOGASSOCIATIONGREPSTRING = 345,
     PCAPSYSLOGDEASSOCIATIONGREPSTRING = 346,
     INV_TOKEN = 347
   };
#endif
/* Tokens.  */
#define QSTRING 258
#define ADDR 259
#define MACADDR 260
#define BOOL 261
#define NUMBER 262
#define DECIMAL 263
#define NUMPAIR 264
#define MIP6ENTITY 265
#define DEBUGLEVEL 266
#define DEBUGLOGFILE 267
#define HOMEADDRESS 268
#define HOMEAGENTADDRESS 269
#define INITIALBINDACKTIMEOUTFIRSTREG 270
#define INITIALBINDACKTIMEOUTREREG 271
#define LINKNAME 272
#define HAMAXBINDINGLIFE 273
#define MNMAXHABINDINGLIFE 274
#define MNMAXCNBINDINGLIFE 275
#define MAXMOBPFXADVINTERVAL 276
#define MINMOBPFXADVINTERVAL 277
#define MNHOMELINK 278
#define HAHOMELINK 279
#define NONVOLATILEBINDINGCACHE 280
#define SENDMOBPFXSOLS 281
#define SENDUNSOLMOBPFXADVS 282
#define SENDMOBPFXADVS 283
#define IPSECPOLICYSET 284
#define IPSECPOLICY 285
#define IPSECTYPE 286
#define USEALTCOA 287
#define USEESP 288
#define USEAH 289
#define USEIPCOMP 290
#define BLOCK 291
#define USEMNHAIPSEC 292
#define KEYMNGMOBCAPABILITY 293
#define HOMEREGBINDING 294
#define MH 295
#define MOBPFXDISC 296
#define TUNNELHOMETESTING 297
#define TUNNELMH 298
#define TUNNELPAYLOAD 299
#define USEMOVEMENTMODULE 300
#define USEPOLICYMODULE 301
#define INTERNAL 302
#define MNROPOLICY 303
#define ICMP 304
#define ANY 305
#define DEFAULTBINDINGACLPOLICY 306
#define BINDINGACLPOLICY 307
#define MNADDRESS 308
#define USECNBUACK 309
#define INTERFACE 310
#define IFNAME 311
#define IFTYPE 312
#define MNIFPREFERENCE 313
#define MNUSEALLINTERFACES 314
#define MNDISCARDHAPARAMPROB 315
#define OPTIMISTICHANDOFF 316
#define RFC5213TIMESTAMPBASEDAPPROACHINUSE 317
#define RFC5213MOBILENODEGENERATEDTIMESTAMPINUSE 318
#define RFC5213FIXEDMARLINKLOCALADDRESSONALLACCESSLINKS 319
#define RFC5213FIXEDMARLINKLAYERADDRESSONALLACCESSLINKS 320
#define RFC5213MINDELAYBEFOREBCEDELETE 321
#define RFC5213MAXDELAYBEFORENEWBCEASSIGN 322
#define RFC5213TIMESTAMPVALIDITYWINDOW 323
#define RFC5213ENABLEMARLOCALROUTING 324
#define DMMCMD 325
#define DMMMAR 326
#define PROXYDMMCMD 327
#define PROXYDMMMAR 328
#define CMDADDRESS 329
#define CMDDMMNETWORKDEVICE 330
#define CMDCORENETWORKADDRESS 331
#define CMDCORENETWORKDEVICE 332
#define MARADDRESSINGRESS 333
#define MARADDRESSEGRESS 334
#define MARDEVICEINGRESS 335
#define MARDEVICEEGRESS 336
#define OURADDRESS 337
#define HOMENETWORKPREFIX 338
#define PBULIFETIME 339
#define PBALIFETIME 340
#define RETRANSMISSIONTIMEOUT 341
#define MAXMESSAGERETRANSMISSIONS 342
#define TUNNELINGENABLED 343
#define DYNAMICTUNNELINGENABLED 344
#define PCAPSYSLOGASSOCIATIONGREPSTRING 345
#define PCAPSYSLOGDEASSOCIATIONGREPSTRING 346
#define INV_TOKEN 347




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 127 "gram.y"

	char *string;
	struct in6_addr addr;
	struct in6_addr macaddr;
	char bool;
	unsigned int num;
	unsigned int numpair[2];
	double dec;



/* Line 1676 of yacc.c  */
#line 248 "gram.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


