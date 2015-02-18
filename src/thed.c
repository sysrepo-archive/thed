/*
 * Copyright (C) 2015 Cisco Systems, Inc.
 *
 * Author: Luka Perkov <luka.perkov@sartura.hr>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <roxml.h>

#include "thed.h"

static uv_tcp_t server;
static uv_tcp_t client;
static struct sockaddr_in bind_addr;

static char *xml_generate_dump()
{
	___debug(".");

	node_t *root, *child;
	char buf[BUFSIZ];
	char *xml = NULL;
	
	root = roxml_load_buf("<data />");
	if (!root) return NULL;
	
	child = roxml_get_chld(root, "data", 0);

	for (int i = 0; i < 5000; i++) {
		snprintf(buf, BUFSIZ, "%d", i);
		roxml_add_node(child, 0, ROXML_ELM_NODE, "item", buf);
	}
	
	roxml_commit_changes(root, NULL, &xml, 0);

	roxml_close(root);
	roxml_release(RELEASE_ALL);

	return xml;
}

void on_server_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
	___debug(".");

	buf->base = malloc(suggested_size);
	if (buf->base)
		buf->len = suggested_size;
	else
		buf->len = 0;
}

void on_server_close(uv_handle_t *handle)
{
	___debug(".");
}

static void on_server_write(uv_write_t *req, int status)
{
	___debug(".");

	write_req_t *wr = (write_req_t *) req;

	free(wr->buf.base);
	free(wr);

	uv_close((uv_handle_t *) &client, on_server_close);

	if (status == 0)
		return;

	__debug("%s\n", uv_err_name(status));
}

void on_server_new_connection(uv_stream_t *server, int status)
{
	___debug(".");

	write_req_t *wr;
	char *data;
	int rc;

	if (status == -1) {
		return;
	}

	uv_tcp_init(uv_default_loop(), &client);

	client.data = server;

	rc = uv_accept(server, (uv_stream_t *) &client);
	if (rc) {
		uv_close((uv_handle_t *) &client, on_server_close);
		return;
	}

	wr = (write_req_t *) malloc(sizeof *wr);
	if (!wr) {
		__debug("no memory");
		return;
	}

	data = xml_generate_dump();
	wr->buf = uv_buf_init(data, strlen(data));

	uv_write(&wr->req, (uv_stream_t *) &client, &wr->buf, 1, on_server_write);
}

void tcp_server_close()
{
	___debug(".");

	uv_close((uv_handle_t *) &server, on_server_close);
}

int tcp_server_init(const char* addr, int port)
{
	___debug(".");

	int rc = 0;

	rc = uv_ip4_addr(addr, port, &bind_addr);
	if (rc) goto out;

	rc = uv_tcp_init(uv_default_loop(), &server);
	if (rc) goto out;

	uv_tcp_bind(&server, (struct sockaddr *) &bind_addr, 0);
	rc = uv_listen((uv_stream_t *) &server, 128, on_server_new_connection);
	if (rc) goto out;

out:
	if (rc) __debug("%s", uv_err_name(rc));
	return rc;
}


int main(int argc, char *const argv[])
{
	___debug(".");

	int rc = 0;


	if (!uv_default_loop()) {
		__debug("failed to allocate default loop");
		return EXIT_FAILURE;
	}

	rc = tcp_server_init("127.0.0.1", 9000);
	if (rc)	goto exit;

	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	__debug("%s has stopped", PROJECT_NAME);

	rc = EXIT_SUCCESS;
exit:
	uv_loop_close(uv_default_loop());

	return rc;
}
