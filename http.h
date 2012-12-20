/*-
 * Copyright 2012 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HTTP_HTTP_H
#define HTTP_HTTP_H

#if defined(__cplusplus)
extern "C" {
#endif

struct http_funcs
{
	void* (*malloc)(int size);
	void (*free)(void* ptr);
	void (*body)(void* opaque, const char* data, int size);
	void (*header)(void* opaque, const char* key, int nkey, const char* value, int nvalue);
	void (*code)(void* opqaue, int code);
};

struct http_roundtripper
{
	struct http_funcs funcs;
	char *scratch;
	void *opaque;
	int code;
	int parsestate;
	int contentlength;
	int state;
	int nscratch;
	int nkey;
	int nvalue;
	int chunked;
};

void http_init(struct http_roundtripper* rt, struct http_funcs, void* opaque);
void http_free(struct http_roundtripper* rt);
int http_data(struct http_roundtripper* rt, const char* data, int size, int* read);
int http_iserror(struct http_roundtripper* rt);

#if defined(__cplusplus)
}
#endif

#endif
