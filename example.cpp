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

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "http.h"

int connectsocket(const char* host, int port) {

	addrinfo* result = NULL;
	sockaddr_in addr = {0};
	int s;

	if (getaddrinfo(host, NULL, NULL, &result))
		goto error;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	for (addrinfo* ai = result; ai != NULL; ai = ai->ai_next) {
		if (ai->ai_family != AF_INET)
			continue;

		const sockaddr_in *ai_in = (const sockaddr_in*)ai->ai_addr;
		addr.sin_addr = ai_in->sin_addr;
		break;
	}

	if (addr.sin_addr.s_addr == INADDR_ANY)
		goto error;

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == -1)
		goto error;

	if (connect(s, (const sockaddr*)&addr, sizeof(addr)))
		goto error;

	return s;

error:
	if (s != -1)
		close(s);
	if (result)
		freeaddrinfo(result);
	return -1;
}

int main() {

	int conn = connectsocket("nothings.org", 80);
	if (conn < 0) {
		fprintf(stderr, "Failed to connect socket\n");
		return -1;
	}

	const char request[] = "GET / HTTP/1.0\r\nContent-Length: 0\r\n\r\n";
	int len = send(conn, request, sizeof(request) - 1, 0);
	if (len != sizeof(request) - 1) {
		fprintf(stderr, "Failed to send request\n");
		close(conn);
		return -1;
	}

	HttpRoundTripper rt;
	httpInit(&rt);

	bool needmore = true;
	char buffer[1024];
	while (needmore) {
		const char* data = buffer;
		int ndata = recv(conn, buffer, sizeof(buffer), 0);
		if (ndata <= 0) {
			fprintf(stderr, "Error receiving data\n");
			close(conn);
			return -1;
		}

		while (needmore && ndata) {
			int read;
			needmore = httpHandleData(&rt, data, ndata, &read);
			ndata -= read;
			data += read;
		}
	}

	if (rt.state != HttpRoundTripper::close) {
		fprintf(stderr, "Error parsing data\n");
		close(conn);
		return -1;
	}

	printf("Response: %d\n", rt.response.code);
	if (!rt.response.body.empty()) {
		printf("%s\n", &rt.response.body[0]);
	}

	close(conn);
	return 0;
}
