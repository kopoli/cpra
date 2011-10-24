/* 
   crpa - A small command line interface to libclang.

   Copyright (C) 2011  Kalle Kankare

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <clang-c/Index.h>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*a))

#ifndef offsetof
#define offsetof(st, m) ((size_t) ( (char *)&((st *)(0))->m - (char *)0 ))
#endif

#define container_of(ptr,type,member) ((char *)(ptr) - offsetof(type,member))

#define CPRA_NAME "cpra"
#define CPRA_VERSION "0.0.0"

void *cpra_alloc(size_t size)
{
  void *ret;

  if((ret=malloc(size)) == NULL) {
    perror("FATAL: malloc() failed");
    exit(2);
  }

  return ret;
}

struct ll {
  struct ll *next,*prev;
};

void ll_inject(struct ll *elem,struct ll *prev,struct ll *next)
{
  /* printf("injecting %p between %p and %p\n",elem,prev,next); */

  next->prev=elem;
  prev->next=elem;
  elem->next=next;
  elem->prev=prev;  
}

void ll_rem(struct ll *elem)
{
    elem->prev->next=elem->next;
    elem->next->prev=elem->prev;
}

#define ll_init(e)        ll_inject((e), (e),        (e))
#define ll_prepend(lst,e) ll_inject((e), (lst),      (lst)->next)
#define ll_append(lst,e)  ll_inject((e), (lst)->prev,(lst))
#define ll_empty(lst)     ((lst)->next == (lst))

struct ll* ll_traverse(struct ll *list, void *data, 
		       int (callback)(struct ll*,void*))
{
  struct ll *next=list,*tmp;

  do {
    tmp=next->next;

    /* printf("ll_traverse:list %p next %p, prev %p, next->next %p\n", */
    /* 	   list,next,next->prev,tmp); */

    if(callback(next,data) == 0)
      return next;
    next=tmp;
  } while(next != list);

  return NULL;
}

int ll_length_cb(struct ll *link, void *data)
{
  (*((int *)data))++;
  return 1;
}

int ll_length(struct ll* list)
{
  int count=0;
  ll_traverse(list,&count,ll_length_cb);
  return count;
}

/* command line definitions */

static char *const CPRA_CMDLINE_OPTS="fstvmirca:hV";

static char * const cpra_opt_help[] = {
  "list functions/methods",
  "list structs/classes",
  "list types/enums",
  "list static variables",
  "list macros",
  "list includes",
  "list references also for above",
  "syntax check",
  "run code completion",
  "file:line:col",
  "display this help",
  "display program version"
};

enum cpra_opts {
  /* this first part should be in the same order as enum cpra_element_id */
  CPRA_OPT_FUNCS=0,
  CPRA_OPT_STRUCTS,
  CPRA_OPT_TYPES,
  CPRA_OPT_VARIABLES,
  CPRA_OPT_MACROS,
  CPRA_OPT_INCLUDES,
  /* other functionality */
  CPRA_OPT_REFS,
  CPRA_OPT_CHECK,
  CPRA_OPT_COMPLETE,
  CPRA_OPT_HELP,
  CPRA_OPT_VERSION,
  CPRA_OPT_COUNT,
} ;

static char * const cpra_opt_enabled="enabled";
static char *cpra_opts_status[CPRA_OPT_COUNT];

/* -- */

struct cpra_element {
  CXCursor cursor;

  struct ll link;
};

/* how many different elements can there be to display */
enum cpra_element_id {
  CPRA_ELEM_FUNC=0,
  CPRA_ELEM_STRUCT,
  CPRA_ELEM_TYPE,
  CPRA_ELEM_VAR,
  CPRA_ELEM_MACRO,
  CPRA_ELEM_INCLUDE,
  CPRA_ELEM_COUNT
};
char *cpra_element_names[]={"Functions","Records","Types",
			    "Variables","Macros","Includes"};
static struct cpra_element *cpra_elements[CPRA_ELEM_COUNT]={};


struct cpra_element *cpra_element_get(struct ll* list)
{
  return (struct cpra_element *)container_of(list,struct cpra_element,link);
}

void cpra_element_add(struct cpra_element **cel, CXCursor cursor)
{
  struct cpra_element *elem;

  elem=cpra_alloc(sizeof(*elem));
  memcpy(&elem->cursor,&cursor,sizeof(cursor));

  /* printf("kind %d\n",cursor.kind); */
   
  if(*cel == NULL) {
    ll_init(&elem->link);
    *cel=elem;
  }
  else
    ll_append(&(*cel)->link,&elem->link);
}

void cpra_element_pop(struct cpra_element *cel)
{
  struct cpra_element *elem=cpra_element_get(cel->link.prev);

  ll_rem(&elem->link);
  free(elem);
}

static int cpra_element_rm_cb(struct ll* list,void *data)
{
  struct cpra_element *elem=cpra_element_get(list);
  struct cpra_element *last=data;

  ll_rem(list);
  free(elem);

  if(elem == last)
    return 0;
  
  return 1;
}

void cpra_element_rm_all(struct cpra_element **cel)
{
  ll_traverse(&(*cel)->link,cpra_element_get(((*cel)->link.prev)),
	      cpra_element_rm_cb);
  *cel=NULL;
}

static void cpra_display_location(char const *format,CXCursor cursor)
{
  CXSourceLocation loc = clang_getCursorLocation(cursor);
  unsigned int line,col;
  CXFile file;
  CXString str;
  
  clang_getSpellingLocation(loc,&file,&line,&col,NULL);
  str=clang_getFileName(file);
  printf(format,clang_getCString(str),line,col);
  clang_disposeString(str);
}

static void cpra_display_name(CXCursor cursor, int depth);

static void cpra_display_type(CXType type)
{
  if(type.kind == CXType_Unexposed) {
    CXCursor cursor=clang_getTypeDeclaration(type);

    putchar(' ');
    cpra_display_name(cursor,0);
  }
  else {
    CXString str = clang_getTypeKindSpelling(type.kind);
    printf(" %s",clang_getCString(str));
    clang_disposeString(str);
  }
}

static void cpra_display_name(CXCursor cursor, int depth)
{
  CXString name;
  char const *str;

  if(0)
  {
    CXString type=clang_getCursorKindSpelling(cursor.kind);
    printf("depth %d kind %s\n",depth,clang_getCString(type));
    clang_disposeString(type);
  }

  if(depth > 10)
    exit(7);
  
  /* TODO should use: clang_isInvalid, clang_isTranslationUnit, clang_isUnexposed */
  switch(cursor.kind) {
  case CXCursor_InvalidFile:
  case CXCursor_UnexposedDecl:
    return;
  default:
    cpra_display_name(clang_getCursorSemanticParent(cursor),depth+1);
    break;
  }

  /*
    clang_getCursorSemanticParent appears to have a bug of sorts. The code:
    void f() {
      struct { int a; } s;
      int b;
      s.a=1;
    }
    The last refecence has the name with parents: f()::b::a
    b should not be its semantic parent (?)

    The following is a workaround:
   */
  if(depth != 0 && cursor.kind == CXCursor_VarDecl)
    return;

  name=clang_getCursorDisplayName(cursor);
  str=clang_getCString(name);

  if(0) 
  {
    printf("(");
    cpra_display_type(clang_getCursorType(cursor));
    printf(")");
  }

  /* if anonymous */
  if(strlen(str)==0)
    cpra_display_location("<anon-%s:%d:%d>",cursor);
  else 
    printf("%s",str);
  if(depth>0)
    printf("::");

  /* printf("%s%s",strlen(str)>0 ? str : "<anon>", depth > 0 ? "::" : ""); */
  /* clang_disposeString(name); */
}

/* TODO 
   - Types of function calls 

   - testcode.c exposes a bug within an instantiation of a global structure (raken ja
     raken_var)
*/
static int cpra_element_display_cb(struct ll* list,void *data)
{
  struct cpra_element *elem=cpra_element_get(list);
  enum cpra_element_id id=(enum cpra_element_id)data;

  CXCursor tr_cursor;

    /* don't display empty elements */
  {
    CXString element_name=clang_getCursorDisplayName(elem->cursor);
    int empty=clang_getCString(element_name)[0] == 0;

    clang_disposeString(element_name);

    if(empty) 
      return 1;
  }

  /* for types and names search for the definition */
  tr_cursor = clang_getCursorDefinition(elem->cursor);
  if(clang_equalCursors(clang_getNullCursor(),tr_cursor))
    tr_cursor = elem->cursor;

  if(id == CPRA_ELEM_VAR || id == CPRA_ELEM_FUNC) {
    int ptrdepth=0;
    CXType type = id == CPRA_ELEM_VAR ? clang_getCursorType(tr_cursor) :
      clang_getCursorResultType(tr_cursor);

    while(type.kind == CXType_Pointer) {
      ptrdepth++;
      type=clang_getPointeeType(type);
    }

    if(clang_isConstQualifiedType(type))
      printf(" const");
    if(clang_isVolatileQualifiedType(type))
      printf(" volatile");
    if(clang_isRestrictQualifiedType(type))
      printf(" restrict");

    cpra_display_type(type);
    for(;ptrdepth>0;ptrdepth--)
      putchar('*');
  }
  else if (id == CPRA_ELEM_STRUCT) {
    char *type="";
    
    switch(tr_cursor.kind) {
    case CXCursor_StructDecl:
      type="Struct"; break;
    case CXCursor_UnionDecl:
      type="Union"; break;
    case CXCursor_ClassDecl:
      type="Class"; break;
    case CXCursor_Namespace:
      type="Namespace"; break;
    default:
      type="UnknownRecord"; break;
    }
    printf(" %s",type);
  }

  putchar(' ');
  cpra_display_name(tr_cursor,0);

  /* linkage */
  if(id != CPRA_ELEM_MACRO && id != CPRA_ELEM_INCLUDE)
  {
    char const *linkage[] = {"Invalid","auto","static","AnonNamespace","global"};
    int linkvalue=clang_getCursorLinkage(tr_cursor);
    if(linkvalue > 0)
      printf(" linkage: %s",linkage[linkvalue]);

  }

  /*is definition */
  if(clang_isCursorDefinition(elem->cursor))
    printf(" IsDef");

  /* If references are enabled, is declaration */
  if(cpra_opts_status[CPRA_OPT_REFS] && clang_isDeclaration(elem->cursor.kind))
    printf(" IsDecl");

  /* display location */
  cpra_display_location(" at %s %d:%d",elem->cursor);

  if(0) {
    CXSourceLocation loc = clang_getCursorLocation(elem->cursor);
    unsigned int line,col;
    CXFile file;
    CXString str;

    clang_getSpellingLocation(loc,&file,&line,&col,NULL);
    str=clang_getFileName(file);

    printf(" at %s %d:%d",clang_getCString(str),line,col);

    clang_disposeString(str);
  }

  puts("");

  return 1;
}

void cpra_element_display(struct cpra_element *cel,enum cpra_element_id id)
{
  ll_traverse(&cel->link,(void *)id,cpra_element_display_cb);
}

void cpra_version_quit()
{
  printf("%s v.%s\nBuilt: %s %s\n",CPRA_NAME,CPRA_VERSION,
	 __DATE__,__TIME__);
  exit(1);
}

/**
 * @brief Displays the help text and exits the program 
 *
 * @param name The name of the run program. eg. argv[0].
 */
void cpra_help_quit(const char *const name)
{
  unsigned int i;
  const unsigned int limit=ARRAY_SIZE(cpra_opt_help);
  
  printf("Usage: %s -%s -- [clang options ...]\n"
	 "Options:\n",name,CPRA_CMDLINE_OPTS);
  for(i=0;i<limit;i++)
    if(i<limit-1 && CPRA_CMDLINE_OPTS[i+1] == ':') {
      printf("  -%c %s\n"
	     "       %s\n",CPRA_CMDLINE_OPTS[i],cpra_opt_help[i+1],
	     cpra_opt_help[i]);
      i++;
    }
    else
      printf("  -%c   %s\n",CPRA_CMDLINE_OPTS[i],cpra_opt_help[i]);
  exit(1);
}

/**
 * @brief Parses the command line. The enabled options are saved at the array
 * cpra_opts_status.
 * @return The number of the first non-option argument.
 */
int cpra_cmdline_parse(int argc, const char * const argv[])
{
  int opt,i,limit=ARRAY_SIZE(cpra_opts_status);

  if(argc == 1)
    cpra_help_quit(argv[0]);

  memset(cpra_opts_status,0,sizeof(cpra_opts_status));

  while((opt = getopt(argc,(char *const *)argv,CPRA_CMDLINE_OPTS)) != -1 ) {
    if(opt == '?' || opt == 'h')
      cpra_help_quit(argv[0]);
    else if(opt == 'V')
      cpra_version_quit();

    for(i=0;i<limit;i++)
      if(opt == CPRA_CMDLINE_OPTS[i]) {
	if(i<limit-1 && CPRA_CMDLINE_OPTS[i+1] == ':')
	  cpra_opts_status[i]=optarg;
	else
	  cpra_opts_status[i]=cpra_opt_enabled;
      }
  }

  //TODO debug
  /* for(i=0;i<limit;i++) { */
  /*   if(cpra_opts_status[i]) */
  /*     printf("%c %s\n",CPRA_CMDLINE_OPTS[i],cpra_opts_status[i]); */
  /* } */

  return optind;
}

enum CXChildVisitResult cb(CXCursor cursor,
			   CXCursor parent,
			   CXClientData client_data)
{
  enum cpra_element_id cid=CPRA_ELEM_COUNT;
  unsigned int line, col;
  CXFile f;
  CXString str;
  CXSourceLocation loc = clang_getCursorLocation(cursor);

  /* do not include compiler defaults */
  clang_getSpellingLocation(loc,&f,&line,&col,NULL);
  str=clang_getFileName(f);
  if(!clang_getCString(str)) {
    clang_disposeString(str);
    return CXChildVisit_Continue;
  }
  clang_disposeString(str);

  switch(cursor.kind) {
  case CXCursor_FunctionDecl:
  case CXCursor_Constructor:
  case CXCursor_Destructor:
  case CXCursor_CXXMethod:
    cid=CPRA_ELEM_FUNC;
    break;

  case CXCursor_StructDecl:
  case CXCursor_UnionDecl:
  case CXCursor_ClassDecl:
  case CXCursor_Namespace:
    cid=CPRA_ELEM_STRUCT;
    break;
    
  case CXCursor_EnumDecl:
  case CXCursor_TypedefDecl:
    cid=CPRA_ELEM_TYPE;
    break;

  case CXCursor_VarDecl:
  case CXCursor_ParmDecl:
  case CXCursor_FieldDecl:
    cid=CPRA_ELEM_VAR;
    break;

  case CXCursor_MacroDefinition:
    cid=CPRA_ELEM_MACRO;
    break;

  case CXCursor_InclusionDirective:
    cid=CPRA_ELEM_INCLUDE;
    break;

  default:
    break;
  }
  
  /* list also references of above types */
  if(cpra_opts_status[CPRA_OPT_REFS]) {
    switch(cursor.kind) {
    case CXCursor_CallExpr:
      cid=CPRA_ELEM_FUNC;
      break;

    case CXCursor_TypeRef:
      cid=CPRA_ELEM_TYPE;
      break;
    
    case CXCursor_MemberRef:
    case CXCursor_MemberRefExpr:
    case CXCursor_DeclRefExpr:
      cid=CPRA_ELEM_VAR;
      break;

    default:
      break;

    }
  }

  if(cid != CPRA_ELEM_COUNT) {

    /* TODO DEBUG */
    if(0)
    {
      const char * str = clang_getCString(clang_getTypeKindSpelling(clang_getCursorType(cursor).kind));
      const char * s3r = clang_getCString(clang_getCursorDisplayName(cursor));
      const char * s4r = clang_getCString(clang_getCursorKindSpelling(clang_getCursorKind(cursor)));

      CXSourceLocation loc = clang_getCursorLocation(cursor);
      unsigned int line,col;
      CXFile f;
      const char *s2r;
      clang_getSpellingLocation(loc,&f,&line,&col,NULL);
      s2r = clang_getCString(clang_getFileName(f));

      printf("kind %d:[%s] type [%s] isdef %d spelling {%s} at %s %d:%d\n",
	     cursor.kind,s4r,str,clang_isCursorDefinition(cursor),s3r,
	     s2r,line,col);
      printf("adding element %d\n",cid);
    }

    cpra_element_add(&cpra_elements[cid],cursor);
  }  

  return CXChildVisit_Recurse;
}

#ifdef CPRA_TESTING

#include "cpra_test.c"

#else 

int main(int argc, const char * const argv[])
{
  int i,ret=0;
  int clang_argc=cpra_cmdline_parse(argc,argv);

  struct {
    int index;
    int translation;
  } s = {};
  
  CXIndex ci = clang_createIndex(1,1);
  s.index=1;

  CXTranslationUnit ctu = 
    clang_parseTranslationUnit(ci,NULL,argv+clang_argc,argc-clang_argc,
			       NULL,0,
			       CXTranslationUnit_DetailedPreprocessingRecord);
  if(!ctu) {
    fprintf(stderr,"Error: Could not create translation unit.\n"
	    "Check that there are no more than one file to be parsed in the"
	    " arguments\n");
    
    ret=1;
    goto end;
  }
  s.translation=1;

  clang_visitChildren(clang_getTranslationUnitCursor(ctu),
		      cb,NULL);

  if(cpra_opts_status[CPRA_OPT_CHECK]) {
    int ret=clang_getNumDiagnostics(ctu);
    printf("Warnings/errors: %d\n",ret);
    return !!ret;
  }

  {
    CXString str=clang_getTranslationUnitSpelling(ctu);
    printf("File: %s\n",clang_getCString(str));
    clang_disposeString(str);
  }
  
  printf("\nDisplaying elements\n");
  for(i=0;i<CPRA_ELEM_COUNT;i++) {
    if(!cpra_elements[i] || !cpra_opts_status[i])
      continue;
    
    printf("%s\n",cpra_element_names[i]);

    cpra_element_display(cpra_elements[i],i);
    cpra_element_rm_all(&cpra_elements[i]);
  }

 end: 
  if(s.translation)
    clang_disposeTranslationUnit(ctu);
  if(s.index)
    clang_disposeIndex(ci);

  return ret;
}

#endif
