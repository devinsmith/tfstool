/*
 * Copyright (c) 2012-2019 Devin Smith <devin@devinsmith.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef SERVICES_HTTP_H
#define SERVICES_HTTP_H

/* HTTP Services version 1.102 (06-17-2019) */
#include <curl/curl.h>

#include <string>
#include <vector>

namespace http {

// HTTP response codes.
const int STATUS_OK = 200;
const int STATUS_NOCONTENT = 204;
const int STATUS_FORBIDDEN = 403;
const int STATUS_ERROR = 500;

class HttpExecutor {
public:
  HttpExecutor();
  ~HttpExecutor();

  static HttpExecutor& default_instance() {
    static HttpExecutor executor;
    return executor;
  }

  CURLM* handle() { return m_multi_handle; }

private:
  CURLM* m_multi_handle;
};

/*
 * An HTTP response
 */
class HttpResponse {
public:
  std::string body;
  std::vector<std::string> headers;
  long status_code;
  double elapsed;
};

class HttpRequest {
public:
  HttpRequest(const std::string &url, bool verbose = false);
  ~HttpRequest();

  void add_header(const char *key, const char *value);
  void set_cert(const std::string &cert, const std::string &key);

  // Authorization schemes
  void set_ntlm(const std::string &username, const std::string &password);
  void set_basic_auth(const std::string &user, const std::string &password);

  bool get_file(const char *file);
  bool get_file_fp(FILE *fp);

  void set_content(const char *content_type);
  HttpResponse exec(const char *method, const char *data,
      HttpExecutor& executor = HttpExecutor::default_instance());
  bool verbose() { return m_verbose; }

  std::string resp_body;
  std::string req_hdrs;
  std::vector<std::string> resp_hdrs;
  double elapsed;
private:
  HttpRequest(const HttpRequest &); // avoid copy constructor

  CURL *m_handle; // curl easy handle.
  curl_slist *m_headers; // Curl headers to append to request.
  std::string m_url;
  bool m_verbose;
  std::string m_user_agent;
};

/* Public functions */
void http_lib_startup(void);
void http_lib_shutdown(void);

const char *http_get_error_str(int error_code);

} // namespace http

#endif /* SERVICES_HTTP_H */

