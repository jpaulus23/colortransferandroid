/// C-runtime patch
#include <stdlib.h>
#include <stdio.h>

extern "C"
{
    // used deep inside FreeImage
    void* lfind( const void * key, const void * base, size_t num, size_t width, int (*fncomparison)(const void *, const void * ) )
    {
        char* Ptr = (char*)base;

        for ( size_t i = 0; i != num; i++, Ptr+=width )
        {
            if ( fncomparison( key, Ptr ) == 0 ) return Ptr;
        }

        return NULL;
    }

    // used in libcompress
    int fseeko64(FILE *stream, off64_t offset, int whence)
    {
        return fseek( stream, int( offset & 0xFFFFFFFF ), whence );
    }

    // used in libcompress
    off64_t ftello64(FILE *stream)
    {
        return ftell( stream );
    }
} // extern C