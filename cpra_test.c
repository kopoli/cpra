#include <stdio.h>

/* test suite functionality */

struct {
  int count;
  int failed;
} cpra_test_statistics = { };

typedef int (*cpra_test)(void);

#define CPRA_ASSERT(test) do {					\
if(!(test)) {							\
  fprintf(stderr,"Test \"%s\": Assert(%d) [%s] at %s\n",	\
	  _test_name,_assert_count,#test,__FILE__);		\
  return 1;}							\
_assert_count++;						\
} while(0)

#define CPRA_TEST(NAME, ...)				\
  int _cpra_test_##NAME(void) {				\
    int _assert_count=0; char *_test_name=#NAME;	\
      {__VA_ARGS__}					\
      cpra_test_statistics.count++; return 0; }

static int cpra_test_runner(cpra_test *tests,int count)
{
  int i=0,ret=0,hasfailed=0;
  printf("running %d tests\n",count);

  for(i=0;i<count;i++) {
    ret=tests[i]();
    putchar(ret ? 'X' : '.');
    if(ret) hasfailed++;
  }
  printf("\nTests completed %d of %d (%d failed)\n",
	 count-hasfailed,count,hasfailed);

  return hasfailed;
}

#define CPRA_TEST_NAMES(NAME,...)  _cpra_test_##NAME,
#define CPRA_TEST_NAMES_STR(NAME,...)  #NAME,

#define CPRA_SUITE_GENERATE(SUITENAME)					\
  SUITENAME(CPRA_TEST)							\
  static cpra_test _cpra_suite_##SUITENAME[] = {SUITENAME(CPRA_TEST_NAMES)};

#define CPRA_SUITE_RUN(SUITENAME) \
  cpra_test_runner(_cpra_suite_##SUITENAME,ARRAY_SIZE(_cpra_suite_##SUITENAME))


/* the actual test suite */

struct ll_test {
  int data;
  struct ll link;
};

#define MAIN_CPRA_SUITE(T)				\
  T(first,{CPRA_ASSERT(0);})				\
T(second,{CPRA_ASSERT(1);})				\
  T(ll_init_test,					\
    struct ll a,b; ll_init(&a);				\
    CPRA_ASSERT((a.next == &a && a.prev == &a ));	\
    ll_append(&a,&b);					\
    CPRA_ASSERT(a.next == &b && b.prev == &a);		\
    CPRA_ASSERT(b.next == &a && a.prev == &b);		\
    ll_rem(&b);						\
    CPRA_ASSERT((a.next == &a && a.prev == &a ));	\

  T(ll_traverse_test,
    
    )

)

CPRA_SUITE_GENERATE(MAIN_CPRA_SUITE)

int main()
{
  return CPRA_SUITE_RUN(MAIN_CPRA_SUITE);
}


/* This code is included just before the real main function. Therefore disable
   it. */
#define main invalid_main
