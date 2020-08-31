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

#include <curl/curl.h>
#include <curl/easy.h>

#include <cstring>
#include <cstdlib>
#include <string>

#include "services/http.h"
#include "utils/logging.h"

namespace http {

/* HTTP Services version 1.102 (06-17-2019) */

struct http_context {
  HttpRequest *req;
  HttpResponse *resp;
};

/* Modern Chrome on Windows 10 */
static const char *chrome_win10_ua = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3770.90 Safari/537.36";

#ifdef _WIN32
#define WAITMS(x) Sleep(x)
#else
/* Portable sleep for platforms other than Windows. */
#define WAITMS(x)                         \
  struct timeval wait { 0, (x) * 1000 };  \
  (void)select(0, NULL, NULL, NULL, &wait);
#endif

void
http_lib_startup(void)
{
  curl_global_init(CURL_GLOBAL_ALL);
}

void
http_lib_shutdown(void)
{
}

HttpExecutor::HttpExecutor()
{
  m_multi_handle = curl_multi_init();
}

HttpExecutor::~HttpExecutor()
{
  curl_multi_cleanup(m_multi_handle);
}

/* Private generic response reading function */
static size_t
dk_httpread(char *ptr, size_t size, size_t nmemb, HttpResponse *hr)
{
  size_t totalsz = size * nmemb;

  if (strstr(ptr, "HTTP/1.1 100 Continue"))
    return totalsz;

  hr->body.append((char *)ptr, totalsz);
  return totalsz;

}

const char *
http_get_error_str(int error_code)
{
  return curl_easy_strerror((CURLcode)error_code);
}

static int
curl_debug_func(CURL *hnd, curl_infotype info, char *data, size_t len,
    http_context *ctx)
{
  std::string hdr;
  std::string::size_type n;

  switch (info) {
  case CURLINFO_HEADER_OUT:
    if (ctx->req->verbose()) {
      std::string verb(data, len);
      log_msgraw(0, "H>: %s", verb.c_str());
    }
    ctx->req->req_hdrs.append(data, len);
    break;
  case CURLINFO_TEXT:
    if (ctx->req->verbose()) {
      std::string verb(data, len);
      log_msgraw(0, "T: %s", verb.c_str());
    }
    break;
  case CURLINFO_HEADER_IN:
    hdr = std::string(data, len);

    if (ctx->req->verbose()) {
      log_msgraw(0, "H<: %s", hdr.c_str());
    }
    n = hdr.find('\r');
    if (n != std::string::npos) {
      hdr.erase(n);
    }
    n = hdr.find('\n');
    if (n != std::string::npos) {
      hdr.erase(n);
    }
    ctx->resp->headers.push_back(hdr);
    break;
  case CURLINFO_DATA_IN:
    if (ctx->req->verbose()) {
      std::string verb(data, len);
      log_msgraw(0, "<: %s", verb.c_str());
    }
    break;
  case CURLINFO_DATA_OUT:
    if (ctx->req->verbose()) {
      std::string verb(data, len);
      log_msgraw(0, ">: %s", verb.c_str());
    }
    break;
  default:
    break;
  }
  return 0;
}

static CURLcode
easy_perform(CURLM *mhnd, CURL *hnd)
{
  CURLcode result = CURLE_OK;
  CURLMcode mcode = CURLM_OK;
  int done = 0; /* bool */

  if (curl_multi_add_handle(mhnd, hnd)) {
    return CURLE_FAILED_INIT;
  }

  while (!done && mcode == 0) {
    int still_running = 0;
    int rc;

    mcode = curl_multi_wait(mhnd, NULL, 0, 1000, &rc);

    if (mcode == 0) {
      if (rc == 0) {
        long sleep_ms;

        /* If it returns without any file descriptor instantly, we need to
         * avoid busy looping during periods where it has nothing particular
         * to wait for. */
        curl_multi_timeout(mhnd, &sleep_ms);
        if (sleep_ms) {
          if (sleep_ms > 1000)
            sleep_ms = 1000;
          WAITMS(sleep_ms);

        }
      }

      mcode = curl_multi_perform(mhnd, &still_running);
    }

    /* Only read still-running if curl_multi_perform returns ok */
    if (!mcode && still_running == 0) {
      CURLMsg *msg = curl_multi_info_read(mhnd, &rc);
      if (msg) {
        result = msg->data.result;
        done = 1;
      }
    }
  }

  if (mcode != 0) {
    if ((int)mcode == CURLM_OUT_OF_MEMORY)
      result = CURLE_OUT_OF_MEMORY;
    else
      result = CURLE_BAD_FUNCTION_ARGUMENT;
  }

  curl_multi_remove_handle(mhnd, hnd);

  return result;
}

void HttpRequest::set_content(const char *content_type)
{
  std::string ctype_hdr = "Content-Type: ";
  ctype_hdr.append(content_type);
  m_headers = curl_slist_append(m_headers, ctype_hdr.c_str());
}

HttpResponse HttpRequest::exec(const char *method, const char *data,
    HttpExecutor& executor)
{
  struct http_context ctx;
  HttpResponse resp;

  if (strcmp(method, "GET") == 0) {
    curl_easy_setopt(m_handle, CURLOPT_HTTPGET, 1);
  } else if (strcmp(method, "POST") == 0) {
    curl_easy_setopt(m_handle, CURLOPT_POST, 1);
  } else {
    curl_easy_setopt(m_handle, CURLOPT_CUSTOMREQUEST, method);
  }

  if (data != NULL) {
    curl_easy_setopt(m_handle, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(m_handle, CURLOPT_POSTFIELDSIZE, (long)strlen(data));
  } else {
    curl_easy_setopt(m_handle, CURLOPT_POSTFIELDSIZE, 0);
  }

  curl_easy_setopt(m_handle, CURLOPT_URL, m_url.c_str());
  curl_easy_setopt(m_handle, CURLOPT_HTTPHEADER, m_headers);
  curl_easy_setopt(m_handle, CURLOPT_USERAGENT, m_user_agent.c_str());

  ctx.resp = &resp;
  ctx.req = this;
  curl_easy_setopt(m_handle, CURLOPT_DEBUGFUNCTION, curl_debug_func);
  curl_easy_setopt(m_handle, CURLOPT_DEBUGDATA, &ctx);
  curl_easy_setopt(m_handle, CURLOPT_VERBOSE, 1);

	curl_easy_setopt(m_handle, CURLOPT_FAILONERROR, 0);

	/* Verification of SSL is disabled on Windows. This is a limitation of
	 * curl */
	curl_easy_setopt(m_handle, CURLOPT_SSL_VERIFYHOST, 0);
	curl_easy_setopt(m_handle, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, &resp);
	curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, dk_httpread);

  CURLcode res = easy_perform(executor.handle(), m_handle);
  if (res != CURLE_OK) {
    log_tmsg(0, "Failure performing request");
  }
  curl_easy_getinfo(m_handle, CURLINFO_RESPONSE_CODE, &resp.status_code);
  curl_easy_getinfo(m_handle, CURLINFO_TOTAL_TIME, &elapsed);
  resp.elapsed = elapsed;

  return resp;
}

static size_t
write_file(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  size_t written = fwrite(ptr, size, nmemb, stream);
  return written;
}

bool
HttpRequest::get_file(const char *file)
{
  FILE *fp;

  fp = fopen(file, "w");
  if (fp == NULL) {
    return false;
  }

  bool ret = get_file_fp(fp);

  fclose(fp);
  return ret;
}

bool
HttpRequest::get_file_fp(FILE *fp)
{
  long status_code;
  curl_easy_setopt(m_handle, CURLOPT_HTTPGET, 1);
  curl_easy_setopt(m_handle, CURLOPT_POSTFIELDSIZE, 0);

  curl_easy_setopt(m_handle, CURLOPT_URL, m_url.c_str());
  curl_easy_setopt(m_handle, CURLOPT_HTTPHEADER, m_headers);
  curl_easy_setopt(m_handle, CURLOPT_USERAGENT, m_user_agent.c_str());

  curl_easy_setopt(m_handle, CURLOPT_FAILONERROR, 0);

  curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, fp);
  curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, write_file);
  curl_easy_setopt(m_handle, CURLOPT_FOLLOWLOCATION, 1L);

  CURLcode res = easy_perform(NULL, m_handle);
  if (res != CURLE_OK) {
    log_tmsg(0, "Failure performing request");
  }
  curl_easy_getinfo(m_handle, CURLINFO_RESPONSE_CODE, &status_code);
  curl_easy_getinfo(m_handle, CURLINFO_TOTAL_TIME, &elapsed);

  return true;
}

HttpRequest::HttpRequest(const std::string &url, bool verbose) :
  m_headers(NULL), m_url(url), m_verbose(verbose), m_user_agent(chrome_win10_ua)
{
  m_handle = curl_easy_init();
}

void
HttpRequest::set_cert(const std::string &cert, const std::string &key)
{
  curl_easy_setopt(m_handle, CURLOPT_SSLCERT, cert.c_str());
  curl_easy_setopt(m_handle, CURLOPT_SSLKEY, key.c_str());
}

void
HttpRequest::set_basic_auth(const std::string &user, const std::string &pass)
{
  curl_easy_setopt(m_handle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
  curl_easy_setopt(m_handle, CURLOPT_USERNAME, user.c_str());
  curl_easy_setopt(m_handle, CURLOPT_PASSWORD, pass.c_str());
}

void HttpRequest::set_ntlm(const std::string &username, const std::string& password)
{
  curl_easy_setopt(m_handle, CURLOPT_HTTPAUTH, CURLAUTH_NTLM);
  curl_easy_setopt(m_handle, CURLOPT_USERNAME, username.c_str());
  curl_easy_setopt(m_handle, CURLOPT_PASSWORD, password.c_str());
}

void
HttpRequest::add_header(const char *key, const char *value)
{
  char maxheader[2048];

  snprintf(maxheader, sizeof(maxheader), "%s: %s", key, value);

  m_headers = curl_slist_append(m_headers, maxheader);
}

HttpRequest::~HttpRequest()
{
  curl_easy_cleanup(m_handle);
  curl_slist_free_all(m_headers);
}

} // namespace http
