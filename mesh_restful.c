/*
Serval DNA HTTP RESTful interface
Copyright (C) 2013,2014 Serval Project Inc.
 
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "serval.h"
#include "conf.h"
#include "httpd.h"
#include "server.h"
#include "strbuf_helpers.h"

static HTTP_HANDLER restful_mesh_routablepeers_json;

struct mesh_peers_response {
  strbuf b;
  httpd_request *r;
};

int restful_mesh_(httpd_request *r, const char *remainder)
{
  r->http.response.header.content_type = CONTENT_TYPE_JSON;
  if (!is_rhizome_http_enabled())
    return 403;
  int ret = authorize_restful(&r->http);
  if (ret)
    return ret;
  const char *verb = HTTP_VERB_GET;
  http_size_t content_length = CONTENT_LENGTH_UNKNOWN;
  HTTP_HANDLER *handler = NULL;
  
  if (strcmp(remainder, "routablepeers.json") == 0) {
    handler = restful_mesh_routablepeers_json;
    verb = HTTP_VERB_GET;
    remainder = "";
  }

  if (handler == NULL)
    return 404;
  if (	 content_length != CONTENT_LENGTH_UNKNOWN
      && r->http.request_header.content_length != CONTENT_LENGTH_UNKNOWN
      && r->http.request_header.content_length != content_length) {
    http_request_simple_response(&r->http, 400, "Bad content length");
    return 400;
  }
  if (r->http.verb != verb)
    return 405;
  return handler(r, remainder);
}

static HTTP_CONTENT_GENERATOR restful_mesh_routablepeers_json_content;

static int restful_mesh_routablepeers_json(httpd_request *r, const char *remainder)
{
  if (*remainder)
    return 404;

  r->u.sidlist.phase = LIST_HEADER;
  r->u.sidlist.cn = 0;
  r->u.sidlist.in = 0;

  http_request_response_generated(&r->http, 200, CONTENT_TYPE_JSON, restful_mesh_routablepeers_json_content);
  return 1;
}

static HTTP_CONTENT_GENERATOR_STRBUF_CHUNKER restful_mesh_routablepeers_json_content_chunk;

static int restful_mesh_routablepeers_json_content(struct http_request *hr, unsigned char *buf, size_t bufsz, struct http_content_generator_result *result)
{
  return generate_http_content_from_strbuf_chunks(hr, (char *)buf, bufsz, result, restful_mesh_routablepeers_json_content_chunk);
}

static int append_sid(struct subscriber *subscriber, void *context)
{
  if (subscriber == my_subscriber)
    return 0;

  if (!context)
    return 0;
    
  struct mesh_peers_response *response = (struct mesh_peers_response*)context;

  if (response->r->u.sidlist.in != 0)
    strbuf_putc(response->b, ',');

  const sid_t sidp = subscriber->sid;

  strbuf_puts(response->b, "\n[");
  strbuf_json_string(response->b, alloca_tohex_sid_t(sidp));
  strbuf_puts(response->b, "]");

  response->r->u.sidlist.in++;

  return 0;
}

static int restful_mesh_routablepeers_json_content_chunk(struct http_request *hr, strbuf b)
{
  httpd_request *r = (httpd_request *) hr;
  // The "my_sid" and "their_sid" per-conversation fields allow the same JSON structure to be used
  // in a future, non-SID-specific request, eg, to list all conversations for all currently open
  // identities.
  const char *headers[] = {
    "sid"
  };

  struct mesh_peers_response foo;
  foo.r = r;
  foo.b = b;

  switch (r->u.sidlist.phase) {
    case LIST_HEADER:
      strbuf_puts(b, "{\n\"header\":[");
      unsigned i;
      for (i = 0; i != NELS(headers); ++i) {
        if (i)
          strbuf_putc(b, ',');
        strbuf_json_string(b, headers[i]);
      }
      strbuf_puts(b, "],\n\"rows\":[");
      if (!strbuf_overrun(b))
        r->u.sidlist.phase = LIST_ROWS;
      return 1;
    case LIST_ROWS:
      enum_subscribers(NULL, append_sid, &foo);
      // fall through...
    case LIST_END:
      strbuf_puts(b, "\n]\n}\n");
      if (!strbuf_overrun(b))
        r->u.sidlist.phase = LIST_DONE;
      // fall through...
    case LIST_DONE:
      return 0;
  }
  abort();
  return 0;
}

