/* 
   Features:
   -list: methods, functions, structs, enums, types, statics, includes
   -check syntax (no printing, other than errors)
   -code completion (normal clang operation)

   other:
   -whereis: funktioille, luokille, tyypeille, muuttujille, vakioille
    -missä määritelty ja missä käytetään.

   Suunnitelmaa:
   -laita listaan kaikki halutut eri elementit, tulosta lopuksi järjestyksessä
    -tarvitaan linkitetty lista, jossa lisäys, walk ja koko listan poisto

 */


#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <clang-c/Index.h>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*a))
#define container_of(ptr,type,member) ((char *)(ptr) - offsetof(type,member))

#define CPRA_NAME "cpra"
#define CPRA_VERSION "0.0.0"

struct ll {
  struct ll *n,*p;
};

void ll_inject(struct ll *e,struct ll *p,struct ll *n)
{
  n->p=e;
  p->n=e;
  e->n=n;
  e->p=p;  
}

void ll_rem(struct ll *e)
{
  e->p->n=e->n;
  e->n->p=e->p;
}

#define ll_init(e)        ll_inject((e), (e),     (e))
#define ll_prepend(lst,e) ll_inject((e), (lst),   (lst)->n)
#define ll_append(lst,e)  ll_inject((e), (lst)->p,(lst))

struct ll* ll_traverse(struct ll *list, void *data, 
		       int (callback)(struct ll*,void*))
{
  struct ll *next=list,*tmp;

  do {
    tmp=next->n;
    if(callback(next,data) == 0)
      return next;
    next=tmp;
  } while(next != list);

  return NULL;
}


/* -- */

struct element {
  XCursor cursor;

  struct ll link;
};

void testprog()
{
  struct aba {
    int elementti;
    struct ll link;
  } jeeje;

  printf("jj %p ja %p\n",&jeeje,container_of(&jeeje.link,struct aba,link));

  exit(7);
}

static char *const CPRA_CMDLINE_OPTS="cfstva:w:hV";

static char * const cpra_opt_help[] = {
  "syntax check",
  "list functions/methods",
  "list structs/classes",
  "list types/enums",
  "list static variables",
  "run code completion",
  "file:line:col",
  "where is element used",
  "[type|class|func|var|constant]",
  "display this help",
  "display program version"
};

enum cpra_opts {
  CPRA_OPT_CHECK=0,
  CPRA_OPT_FUNCS,
  CPRA_OPT_STRUCTS,
  CPRA_OPT_TYPES,
  CPRA_OPT_VARIABLES,
  CPRA_OPT_COMPLETE,
  CPRA_OPT_WHEREIS,
  CPRA_OPT_HELP,
  CPRA_OPT_VERSION,
  CPRA_OPT_COUNT,
} ;

static char * const cpra_opt_enabled="enabled";
static char *cpra_opts_status[CPRA_OPT_COUNT];

/* sizeof(CPRA_CMDLINE_OPTS)]; */

void cpra_version_quit()
{
  printf("%s v.%s\nBuilt: %s %s\n",CPRA_NAME,CPRA_VERSION,__DATE__,__TIME__);
  exit(1);
}

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

int cpra_cmdline_parse(int argc, const char * const argv[])
{
  int opt,i,limit=ARRAY_SIZE(cpra_opts_status);

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

  for(i=0;i<limit;i++) {
    if(cpra_opts_status[i])
      printf("%c %s\n",CPRA_CMDLINE_OPTS[i],cpra_opts_status[i]);
  }

  return optind;
}


enum CXChildVisitResult cb(CXCursor cursor,
			   CXCursor parent,
			   CXClientData client_data)
{
  const char * str = clang_getCString(clang_getTypeKindSpelling(clang_getCursorType(cursor).kind));
  /* const char * s3r = clang_getCString(clang_getCursorUSR(cursor)); */
  const char * s3r = clang_getCString(clang_getCursorDisplayName(cursor));

  const char * s4r = clang_getCString(clang_getCursorKindSpelling(clang_getCursorKind(cursor)));

  CXSourceLocation loc = clang_getCursorLocation(cursor);
  unsigned int line,col;
  CXFile f;
  const char *s2r;
  clang_getSpellingLocation(loc,&f,&line,&col,NULL);
  s2r = clang_getCString(clang_getFileName(f));

  if(s2r != NULL)
    printf("kind %d:[%s] type [%s] isdef %d spelling {%s} at %s %d:%d\n",
	   cursor.kind,s4r,str,clang_isCursorDefinition(cursor),s3r,
	   s2r,line,col);

  return CXChildVisit_Recurse;
}


int main(int argc, const char * const argv[])
{
  int clang_argc=cpra_cmdline_parse(argc,argv);

  testprog();

  CXIndex ci = clang_createIndex(1,1);

  CXTranslationUnit ctu = 
    clang_createTranslationUnitFromSourceFile(ci,NULL,argc-clang_argc,
					      argv+clang_argc,0,NULL);

  clang_visitChildren(clang_getTranslationUnitCursor(ctu),
		      cb,NULL);
  
  clang_disposeTranslationUnit(ctu);
  clang_disposeIndex(ci);

  return 0;
}
