//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// test <stdio.h>

#include <stdio.h>
#include <type_traits>

#ifndef BUFSIZ
#error BUFSIZ not defined
#endif

#ifndef EOF
#error EOF not defined
#endif

#ifndef FILENAME_MAX
#error FILENAME_MAX not defined
#endif

#ifndef FOPEN_MAX
#error FOPEN_MAX not defined
#endif

#ifndef L_tmpnam
#error L_tmpnam not defined
#endif

#ifndef NULL
#error NULL not defined
#endif

#ifndef SEEK_CUR
#error SEEK_CUR not defined
#endif

#ifndef SEEK_END
#error SEEK_END not defined
#endif

#ifndef SEEK_SET
#error SEEK_SET not defined
#endif

#ifndef TMP_MAX
#error TMP_MAX not defined
#endif

#ifndef _IOFBF
#error _IOFBF not defined
#endif

#ifndef _IOLBF
#error _IOLBF not defined
#endif

#ifndef _IONBF
#error _IONBF not defined
#endif

#ifndef stderr
#error stderr not defined
#endif

#ifndef stdin
#error stdin not defined
#endif

#ifndef stdout
#error stdout not defined
#endif

#include <cstdarg>

#pragma clang diagnostic ignored "-Wformat-zero-length"

int main()
{
    FILE* fp = 0;
    fpos_t fpos = {0};
    size_t s = 0;
    char* cp = 0;
    va_list va;
    static_assert((std::is_same<decltype(remove("")), int>::value), "");
    static_assert((std::is_same<decltype(rename("","")), int>::value), "");
    static_assert((std::is_same<decltype(tmpfile()), FILE*>::value), "");
    static_assert((std::is_same<decltype(tmpnam(cp)), char*>::value), "");
    static_assert((std::is_same<decltype(fclose(fp)), int>::value), "");
    static_assert((std::is_same<decltype(fflush(fp)), int>::value), "");
    static_assert((std::is_same<decltype(fopen("", "")), FILE*>::value), "");
    static_assert((std::is_same<decltype(freopen("", "", fp)), FILE*>::value), "");
    static_assert((std::is_same<decltype(setbuf(fp,cp)), void>::value), "");
    static_assert((std::is_same<decltype(vfprintf(fp,"",va)), int>::value), "");
    static_assert((std::is_same<decltype(fprintf(fp," ")), int>::value), "");
    static_assert((std::is_same<decltype(fscanf(fp,"")), int>::value), "");
    static_assert((std::is_same<decltype(printf("\n")), int>::value), "");
    static_assert((std::is_same<decltype(scanf("\n")), int>::value), "");
    static_assert((std::is_same<decltype(snprintf(cp,0,"p")), int>::value), "");
    static_assert((std::is_same<decltype(sprintf(cp," ")), int>::value), "");
    static_assert((std::is_same<decltype(sscanf("","")), int>::value), "");
    static_assert((std::is_same<decltype(vfprintf(fp,"",va)), int>::value), "");
    static_assert((std::is_same<decltype(vfscanf(fp,"",va)), int>::value), "");
    static_assert((std::is_same<decltype(vprintf(" ",va)), int>::value), "");
    static_assert((std::is_same<decltype(vscanf("",va)), int>::value), "");
    static_assert((std::is_same<decltype(vsnprintf(cp,0," ",va)), int>::value), "");
    static_assert((std::is_same<decltype(vsprintf(cp," ",va)), int>::value), "");
    static_assert((std::is_same<decltype(vsscanf("","",va)), int>::value), "");
    static_assert((std::is_same<decltype(fgetc(fp)), int>::value), "");
    static_assert((std::is_same<decltype(fgets(cp,0,fp)), char*>::value), "");
    static_assert((std::is_same<decltype(fputc(0,fp)), int>::value), "");
    static_assert((std::is_same<decltype(fputs("",fp)), int>::value), "");
    static_assert((std::is_same<decltype(getc(fp)), int>::value), "");
    static_assert((std::is_same<decltype(getchar()), int>::value), "");
    static_assert((std::is_same<decltype(gets(cp)), char*>::value), "");
    static_assert((std::is_same<decltype(putc(0,fp)), int>::value), "");
    static_assert((std::is_same<decltype(putchar(0)), int>::value), "");
    static_assert((std::is_same<decltype(puts("")), int>::value), "");
    static_assert((std::is_same<decltype(ungetc(0,fp)), int>::value), "");
    static_assert((std::is_same<decltype(fread((void*)0,0,0,fp)), size_t>::value), "");
    static_assert((std::is_same<decltype(fwrite((const void*)0,0,0,fp)), size_t>::value), "");
    static_assert((std::is_same<decltype(fgetpos(fp, &fpos)), int>::value), "");
    static_assert((std::is_same<decltype(fseek(fp, 0,0)), int>::value), "");
    static_assert((std::is_same<decltype(fsetpos(fp, &fpos)), int>::value), "");
    static_assert((std::is_same<decltype(ftell(fp)), long>::value), "");
    static_assert((std::is_same<decltype(rewind(fp)), void>::value), "");
    static_assert((std::is_same<decltype(clearerr(fp)), void>::value), "");
    static_assert((std::is_same<decltype(feof(fp)), int>::value), "");
    static_assert((std::is_same<decltype(ferror(fp)), int>::value), "");
    static_assert((std::is_same<decltype(perror("")), void>::value), "");
}
