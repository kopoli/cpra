#include <stdio.h> 


#include <clang-c/Index.h>

enum CXChildVisitResult cb(CXCursor cursor,
			   CXCursor parent,
			   CXClientData client_data)
{
  CXString cxstr = clang_getTypeKindSpelling(clang_getCursorType(cursor).kind);
  const char * str = clang_getCString(cxstr);
  const char * s3r = clang_getCString(clang_getCursorDisplayName(cursor));

  CXSourceLocation loc = clang_getCursorLocation(cursor);
  unsigned int line,col;
  CXFile f;
  const char *s2r;
  clang_getSpellingLocation(loc,&f,&line,&col,NULL);
  s2r = clang_getCString(clang_getFileName(f));

  printf("kind %d type [%s] spelling {%s} at %s %d:%d\n",cursor.kind,str,s3r,
	 s2r,line,col);

  return CXChildVisit_Recurse;
}


int main(int argc, char *argv[])
{
  CXIndex ci = clang_createIndex(1,1);

  CXTranslationUnit ctu = 
    clang_createTranslationUnitFromSourceFile(ci,NULL,argc-1,argv+1,0,NULL);

  clang_visitChildren(clang_getTranslationUnitCursor(ctu),
		      cb,NULL);
  
  clang_disposeTranslationUnit(ctu);
  clang_disposeIndex(ci);

  return 0;
}
