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

struct ll* ll_traverse(struct ll *list, void *data, 
		       int (callback)(struct ll*,void*))
{
  struct ll *next=list,*tmp;

  do {
    tmp=next->next;
    if(callback(next,data) == 0)
      return next;
    next=tmp;
  } while(next != list);

  return NULL;
}


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
  CPRA_ELEM_COUNT
};
static struct cpra_element cpra_elements[CPRA_ELEM_COUNT]={};

void cpra_element_add(struct cpra_element *cel, CXCursor cursor)
{
  struct cpra_element *elem;

  if(!cel)
    return;

  elem=cpra_alloc(sizeof(*cel));
  elem->cursor=cursor;
  ll_append(&cel->link,&elem->link);
}

void cpra_element_pop(struct cpra_element *cel)
{
  struct cpra_element *elem=
    (struct cpra_element *)container_of(cel->link.prev,
					struct cpra_element,link);
  ll_rem(&elem->link);
  free(elem);
}

/*
  Tulostetaan:

  Yleisesti:
  -mitä kieltä käytetään: clang_getCursorLanguage

  Jokaisesta
  -nimi clang_getCursorDisplayName
  -tiedosto:rivi:sarake clang_getCursorLocation
                        clang_getSpellingLocation
  -onko määrittely: clang_isCursorDefinition

  funktioista:
  -paluuarvon tyyppi clang_getCursorResultType

  metodeista:
  -luokka

  (globaaleista) muuttujista:
  -tyyppi
  -(onko globaali) clang_getCursorSemanticParent == ctu

*/


static int cpra_element_display_cb(struct ll* list,void *data)
{
  struct cpra_element *elem=data;

  const char * s4r = clang_getCString(clang_getCursorKindSpelling(clang_getCursorKind(elem->cursor)));

  printf("elem %p cursorkind %s\n",elem,s4r);

  return 1;
}

void cpra_element_display(struct cpra_element *cel)
{
  ll_traverse(&cel->link,cel,cpra_element_display_cb);
}

void testprog()
{
  struct aba {
    int elementti;
    struct ll link;
  } jeeje;

  printf("jj %p ja %p\n",&jeeje,container_of(&jeeje.link,struct aba,link));

  exit(7);
}

static char *const CPRA_CMDLINE_OPTS="cfstvma:w:hV";

static char * const cpra_opt_help[] = {
  "syntax check",
  "list functions/methods",
  "list structs/classes",
  "list types/enums",
  "list static variables",
  "list macros",
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
  CPRA_OPT_MACROS,
  CPRA_OPT_COMPLETE,
  CPRA_OPT_WHEREIS,
  CPRA_OPT_HELP,
  CPRA_OPT_VERSION,
  CPRA_OPT_COUNT,
} ;

static char * const cpra_opt_enabled="enabled";
static char *cpra_opts_status[CPRA_OPT_COUNT];


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

  if(clang_getCursorKind(cursor) == CXCursor_FunctionDecl) {
    cpra_element_add(&cpra_elements[CPRA_ELEM_FUNC],cursor);
    printf("LÖYTYI!!!!!\n");
  }

  return CXChildVisit_Recurse;
}


int main(int argc, const char * const argv[])
{
  int clang_argc=cpra_cmdline_parse(argc,argv);
  int i;

  /* testprog(); */

  for(i=0;i<CPRA_ELEM_COUNT;i++)
    ll_init(&cpra_elements[i].link);

  CXIndex ci = clang_createIndex(1,1);

  CXTranslationUnit ctu = 
    clang_parseTranslationUnit(ci,NULL,argv+clang_argc,argc-clang_argc,
			       NULL,0,
			       CXTranslationUnit_DetailedPreprocessingRecord);
    /*
    clang_createTranslationUnitFromSourceFile(ci,NULL,argc-clang_argc,
					      argv+clang_argc,0,NULL);
    */

  clang_visitChildren(clang_getTranslationUnitCursor(ctu),
		      cb,NULL);
  
  printf("\nDisplaying elements\n");
  cpra_element_display(&cpra_elements[CPRA_ELEM_FUNC]);

  clang_disposeTranslationUnit(ctu);
  clang_disposeIndex(ci);

  return 0;
}
