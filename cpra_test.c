/* 
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

/* test suite interface */

/* verbose assertion */
#define CPRA_ASSERT(test) do {					\
if(!(test)) {							\
  fprintf(stderr,"Test \"%s\": Assert(%d) [%s] at %s\n",	\
	  _test_name,_assert_count,#test,__FILE__);		\
  return 1;}							\
_assert_count++;						\
} while(0)

/* generate a test suite. The SUITENAME must be a macro of the following format:
   
   #define SUITE(T) \
     T(Test_name_1, Test_body_1;) \
     T(Test_name_2, Test_body_2;) \     

 */
#define CPRA_SUITE_GENERATE(SUITENAME)					\
  SUITENAME(CPRA_TEST)							\
  static cpra_test _cpra_suite_##SUITENAME[] = {SUITENAME(CPRA_TEST_NAMES)};

/*  run a suite generated with CPRA_SUITE_GENERATE. */
#define CPRA_SUITE_RUN(SUITENAME)			\
  cpra_test_runner(#SUITENAME,_cpra_suite_##SUITENAME,	\
		   ARRAY_SIZE(_cpra_suite_##SUITENAME))


/* test suite generation internals */

struct {
  int count;
  int failed;
} cpra_test_statistics = { };
typedef int (*cpra_test)(void);

#define CPRA_TEST(NAME, ...)				\
  int _cpra_test_##NAME(void) {				\
    int _assert_count=0; char *_test_name=#NAME;	\
      {__VA_ARGS__}					\
      cpra_test_statistics.count++; return 0; }

static int cpra_test_runner(char *name,cpra_test *tests,int count)
{
  int i=0,ret=0,hasfailed=0;
  printf("running suite %s\n",name);

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


/* the actual test suite */

struct ll_test {
  int d;
  struct ll l;
};

/* Sums the data elements  */
int lt_cb(struct ll* l,void* d)
{
  struct ll_test *lt = (struct ll_test *)container_of(l,struct ll_test,l);
  int *ptr = (int*)d;
  
  *ptr += lt->d;
  return 1;
}

#define MAIN_CPRA_SUITE(T)				\
  T(ll_init_test,					\
    struct ll a,b; ll_init(&a);				\
    CPRA_ASSERT((a.next == &a && a.prev == &a ));	\
    ll_append(&a,&b);					\
    CPRA_ASSERT(a.next == &b && b.prev == &a);		\
    CPRA_ASSERT(b.next == &a && a.prev == &b);		\
    ll_rem(&b);						\
    CPRA_ASSERT((a.next == &a && a.prev == &a ));)	\
							\
  T(ll_traverse_test,					\
    int val=0;						\
    struct ll_test a,b;					\
    a.d=1; b.d=2;					\
    ll_init(&a.l); ll_append(&a.l,&b.l);		\
    ll_traverse(&a.l,&val,lt_cb);			\
    CPRA_ASSERT(val == a.d+b.d);			\
    CPRA_ASSERT(ll_length(&a.l) == 2);)			\
							\
  T(cpra_element_test,					\
    struct cpra_element *cel=NULL;			\
    CXCursor c={};					\
    cpra_element_add(&cel,c);				\
    CPRA_ASSERT(cel != NULL);				\
    CPRA_ASSERT(ll_length(&cel->link) == 1);		\
    cpra_element_add(&cel,c);				\
    CPRA_ASSERT(ll_length(&cel->link) == 2);		\
    cpra_element_pop(cel);				\
    CPRA_ASSERT(ll_length(&cel->link) == 1);		\
    cpra_element_pop(cel);)				\
  T(cpra_element_rm_all_test,				\
    struct cpra_element *cel=NULL;			\
    CXCursor c={};					\
    cpra_element_add(&cel,c);				\
    cpra_element_rm_all(&cel);				\
    CPRA_ASSERT(cel == NULL);				\
    cpra_element_add(&cel,c);				\
    cpra_element_add(&cel,c);				\
    cpra_element_add(&cel,c);				\
    cpra_element_rm_all(&cel);				\
    CPRA_ASSERT(cel == NULL);)

/*



*/

CPRA_SUITE_GENERATE(MAIN_CPRA_SUITE)

int main(void)
{
  return CPRA_SUITE_RUN(MAIN_CPRA_SUITE);
}

