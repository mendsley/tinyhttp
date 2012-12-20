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

#include "http.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "header.h"
#include "chunk.h"

static void appendResponseData(HttpRoundTripper* rt, const void* data, int ndata)
{
	HttpBuffer* b = &rt->response.body;
	b->insert(b->end(), (const unsigned char*)data, (const unsigned char*)data + ndata);
}

static int min(int a, int b)
{
	return a > b ? b : a;
}

namespace {
	struct State {
		enum Enum
		{
			header,
			chunk_header,
			chunk_data,
			raw_data,
			close,
			error,
		};
	};
}

void httpInit(HttpRoundTripper* rt)
{
	rt->response.body.clear();
	rt->response.cookies.clear();
	rt->response.code = 0;
	rt->parsestate = 0;
	rt->state = State::header;
	rt->contentlength = -1;
	rt->chunked = false;
}

bool httpHandleData(HttpRoundTripper* rt, const char* data, int size, int* read)
{
	const int initialSize = size;
	while (size)
	{
		switch (rt->state)
		{
		case State::header:
			switch (http_parse_header_char(&rt->parsestate, *data))
			{
			case http_header_status_done:
				if (rt->parsestate != 0)
					rt->state = State::error;
				else if (rt->chunked)
				{
					rt->contentlength = 0;
					rt->state = State::chunk_header;
				}
				else if (rt->contentlength == 0)
					rt->state = State::close;
				else if (rt->contentlength > 0)
					rt->state = State::raw_data;
				else
					rt->state = State::error;
				break;

			case http_header_status_code_character:
				rt->response.code = rt->response.code * 10 + *data - '0';
				break;

			case http_header_status_key_character:
				rt->lastkey.push_back(tolower(*data));
				break;

			case http_header_status_value_character:
				rt->lastvalue.push_back(tolower(*data));
				break;

			case http_header_status_store_keyvalue:
				if (rt->lastkey == "transfer-encoding")
					rt->chunked = (rt->lastvalue == "chunked");
				else if (rt->lastkey == "content-length")
					rt->contentlength = atoi(rt->lastvalue.c_str());
				else if (rt->lastkey == "set-cookie")
				{
					const char* values = strchr(rt->lastvalue.c_str(), '=');
					if (values)
					{
						const char* params = strchr(values, ';');
						if (!params)
							params = rt->lastvalue.c_str() + rt->lastvalue.size();

						rt->response.cookies.insert(
							HttpResponse::CookieMap::value_type(
								  std::string(rt->lastvalue.c_str(), values)
								, std::string(values + 1, params)
							)
						);
					}
				}

				rt->lastkey.clear();
				rt->lastvalue.clear();
				break;
			}

			--size;
			++data;
			break;

		case State::chunk_header:
			if (!http_parse_chunked(&rt->parsestate, &rt->contentlength, *data))
			{
				if (rt->contentlength == -1)
					rt->state = State::error;
				else if (rt->contentlength == 0)
					rt->state = State::close;
				else
					rt->state = State::chunk_data;
			}

			--size;
			++data;
			break;

		case State::chunk_data:
			{
				const int chunksize = min(size, rt->contentlength);
				appendResponseData(rt, data, chunksize);
				rt->contentlength -= chunksize;
				size -= chunksize;
				data += chunksize;

				if (rt->contentlength == 0)
				{
					rt->contentlength = 1;
					rt->state = State::chunk_header;
				}
			}
			break;

		case State::raw_data:
			{
				const int chunksize = min(size, rt->contentlength);
				appendResponseData(rt, data, chunksize);
				rt->contentlength -= chunksize;
				size -= chunksize;
				data += chunksize;

				if (rt->contentlength == 0)
					rt->state = State::close;
			}
			break;

		case State::close:
		case State::error:
			break;
		}

		if (rt->state == State::error || rt->state == State::close)
		{
			*read = initialSize - size;
			return false;
		}
	}

	*read = initialSize - size;
	return true;
}

bool httpIsError(HttpRoundTripper* rt)
{
	return rt->state == State::error;
}
