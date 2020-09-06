// This file is part of BOINC.
// http://boinc.berkeley.edu
// Copyright (C) 2008 University of California
//
// BOINC is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// BOINC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with BOINC.  If not, see <http://www.gnu.org/licenses/>.

#if   defined(_WIN32) && !defined(__STDWX_H__)
#include "boinc_win.h"
#elif defined(_WIN32) && defined(__STDWX_H__)
#include "stdwx.h"
#else
#include "config.h"
#include <string>
#include <cstring>
#include <cstdarg>
#endif

#include "error_numbers.h"
#include "str_replace.h"

#include "miofile.h"

using std::string;

MIOFILE::MIOFILE() {
    mf = 0;
    wbuf = 0;
    len = 0;
    stdio_stream = 0;
		#ifdef _USING_FCGI_
		fcgx_stream = 0;
		#endif
    buf = 0;
}

MIOFILE::~MIOFILE() {
}

void MIOFILE::init_mfile(MFILE* _mf) {
    mf = _mf;
}

void MIOFILE::init_file(FILE* _f) {
    mf = 0;
    stdio_stream = _f;
		#ifdef _USING_FCGI_
		fcgx_stream= 0;
		#endif
}

#ifdef _USING_FCGI_
void MIOFILE::init_file(FCGI_FILE* _f) {
    mf = 0;
    stdio_stream = _f->stdio_stream;
		fcgx_stream = _f->fcgx_stream;
}

void MIOFILE::init_file(FCGX_Stream* _f) {
    mf = 0;
    stdio_stream = 0;
		fcgx_stream = _f;
}
#endif

void MIOFILE::init_buf_read(const char* _buf) {
    buf = _buf;
}

void MIOFILE::init_buf_write(char* _buf, int _len) {
    wbuf = _buf;
    len = _len;
    wbuf[0] = 0;
}

bool MIOFILE::eof() {
    if (stdio_stream) {
        if (!::feof(stdio_stream)) {
            return false;
        }
    }
		#ifdef _USING_FCGI_
    if (fcgx_stream) {
        if (!FCGX_HasSeenEOF(fcgx_stream)) {
            return false;
        }
    }
		#endif
    return true;
}


int MIOFILE::printf(const char* format, ...) {
    int retval;

    va_list ap;
    va_start(ap, format);
    if (mf) {
        retval = mf->vprintf(format, ap);
    } else
    if (stdio_stream) {
        retval = ::vfprintf(stdio_stream, format, ap);
    } else
		#ifdef _USING_FCGI_
    if (fcgx_stream) {
        retval = FCGX_VFPrintF(fcgx_stream, format, ap);
    } else
		#endif
		{
        size_t cursize = strlen(wbuf);
        size_t remaining_len = len - cursize;
        retval = vsnprintf(wbuf+cursize, remaining_len, format, ap);
    }
    va_end(ap);
    return retval;
}

char* MIOFILE::fgets(char* dst, int dst_len) {
    if (stdio_stream) {
        return ::fgets(dst, dst_len, stdio_stream);
		}
#ifdef _USING_FCGI_
		else if(fcgx_stream) {
			return FCGX_GetLine(dst,dst_len,fcgx_stream);
		}
#endif
    const char* q = strchr(buf, '\n');
    if (!q) return 0;

    q++;
    int n = (int)(q - buf);
    if (n > dst_len-1) n = dst_len-1;
    memcpy(dst, buf, n);
    dst[n] = 0;

    buf = q;
    return dst;
}

int MIOFILE::_ungetc(int c) {
    if (stdio_stream) {
        return ::ungetc(c, stdio_stream);
		} else
#ifdef _USING_FCGI_
		if(fcgx_stream) {
			return FCGX_UnGetChar(c, fcgx_stream);
		} else
#endif
		{
        buf--;
        // NOTE: we assume that the char being pushed
        // is what's already there
        //*buf = c;
    }
    return c;
}

// copy from a file to static buffer
//
int copy_element_contents(MIOFILE& in, const char* end_tag, char* p, int len) {
    char buf[256];
    int n;

    strlcpy(p, "", len);
    while (in.fgets(buf, 256)) {
        if (strstr(buf, end_tag)) {
            return 0;
        }
        n = (int)strlen(buf);
        if (n >= len-1) return ERR_XML_PARSE;
        strlcat(p, buf, len);
        len -= n;
    }
    return ERR_XML_PARSE;
}

int copy_element_contents(MIOFILE& in, const char* end_tag, string& str) {
    char buf[256];

    str = "";
    while (in.fgets(buf, 256)) {
        if (strstr(buf, end_tag)) {
            return 0;
        }
        str += buf;
    }
    fprintf(stderr, "copy_element_contents(): no end tag\n");
    return ERR_XML_PARSE;
}

