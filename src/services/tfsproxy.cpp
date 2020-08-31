/*
 * Copyright (c) 2017 Devin Smith <devin@devinsmith.net>
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

#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "services/http.h"
#include "services/tfsproxy.h"
#include "utils/cJSON.h"
#include "utils/logging.h"
#include "utils/web.h"

using namespace http;

TfsProxy::TfsProxy(const std::string &baseurl, const std::string &branch,
    const std::string &username, const std::string &password) :
  _baseurl(baseurl), _branch(branch), _username(username),
  _password(password)
{
}

TfsProxy::~TfsProxy()
{
}

cJSON *TfsProxy::sendReq(const char *method, std::string &url,
  const char *body) const
{
  HttpRequest req(url, false);
  req.set_ntlm(_username, _password);
  req.set_content("application/json");

  HttpResponse res = req.exec(method, body);

  // Validate that status code is "successful" where successful means
  // 200 OK or 201 Created.
  if (res.status_code != 200 && res.status_code != 201) {
    log_tmsg(0, "Made request to %s, and status code is %d\n%s\n", url.c_str(),
        res.status_code, res.body.c_str());
    return NULL;
  }

  return cJSON_Parse(res.body.c_str());
}

bool TfsProxy::GetChangesAfter(const std::string &changeset,
  std::vector<ChangesetInfo> &changes)
{
  std::string url = _baseurl;
  std::string api_url = "/changesets?searchCriteria.fromId=" + changeset +
    "&searchCriteria.itemPath=" + _branch + "&$orderby=id asc";

  url.append(utils::UrlEncode(api_url));

  cJSON *data = sendReq("GET", url, NULL);
  if (data == NULL) {
    return false;
  }

  // Iterate through all changesets.
  cJSON *countItem = cJSON_GetObjectItem(data, "count");
  if (countItem != NULL && countItem->type == cJSON_Number) {
    changes.reserve(countItem->valueint);
  }

  cJSON *valueArray = cJSON_GetObjectItem(data, "value");
  if (valueArray == NULL || valueArray->type != cJSON_Array)
    return false;

  for (cJSON *value = valueArray->child; value != NULL;
      value = value->next) {
    ChangesetInfo ci;

    // Parse value objects
    cJSON *changesetId = cJSON_GetObjectItem(value, "changesetId");
    if (changesetId != NULL && changesetId->type == cJSON_Number) {
      ci.ChangesetId = changesetId->valueint;
    }

    cJSON *authorObj = cJSON_GetObjectItem(value, "author");
    if (authorObj != NULL && authorObj->type == cJSON_Object) {
      cJSON *authorName = cJSON_GetObjectItem(authorObj, "displayName");
      if (authorName != NULL && authorName->type == cJSON_String) {
        ci.Author = authorName->valuestring;
      }
    }

    changes.push_back(ci);
  }

  // Debugging.
  //  printf("%s\n", cJSON_Print(data));

  cJSON_Delete(data);

  return true;
}

bool TfsProxy::GetChangesetComment(ChangesetInfo &changeset)
{
  char changesetId[32];
  std::string url = _baseurl;

  snprintf(changesetId, sizeof(changesetId), "%d", changeset.ChangesetId);

  url.append("/changesets/");
  url.append(changesetId);

  cJSON *data = sendReq("GET", url, NULL);
  if (data == NULL) {
    return false;
  }

  // Iterate through all changesets.
  cJSON *comment = cJSON_GetObjectItem(data, "comment");
  if (comment != NULL && comment->type == cJSON_String) {
    changeset.Comment = comment->valuestring;
  }

  // Debugging.
  //  printf("%s\n", cJSON_Print(data));

  cJSON_Delete(data);

  return true;
}

bool TfsProxy::GetChangesetChanges(ChangesetInfo &changeset)
{
  char changesetId[32];
  std::string url = _baseurl;

  snprintf(changesetId, sizeof(changesetId), "%d", changeset.ChangesetId);

  url.append("/changesets/");
  url.append(changesetId);
  url.append("/changes?%24top=2000");

  cJSON *data = sendReq("GET", url, NULL);
  if (data == NULL) {
    return false;
  }

  // Iterate through all changes.
  cJSON *countItem = cJSON_GetObjectItem(data, "count");
  if (countItem != NULL && countItem->type == cJSON_Number) {
    changeset.changes.reserve(countItem->valueint);
  }

  cJSON *valueArray = cJSON_GetObjectItem(data, "value");
  if (valueArray == NULL || valueArray->type != cJSON_Array)
    return false;

  for (cJSON *value = valueArray->child; value != NULL;
      value = value->next) {
    ChangesetChange change;

    // Parse value objects (each contains an item and change type)
    cJSON *changeType = cJSON_GetObjectItem(value, "changeType");
    if (changeType != NULL && changeType->type == cJSON_String) {
      change.ChangeType = changeType->valuestring;
    }

    cJSON *itemObj = cJSON_GetObjectItem(value, "item");
    if (itemObj != NULL && itemObj->type == cJSON_Object) {

      // Grab version (int), path (string), and url (url).
      cJSON *itemAtt = cJSON_GetObjectItem(itemObj, "version");
      if (itemAtt != NULL && itemAtt->type == cJSON_Number) {
        change.Version = itemAtt->valueint;
      }

      itemAtt = cJSON_GetObjectItem(itemObj, "path");
      if (itemAtt != NULL && itemAtt->type == cJSON_String) {
        change.Path = itemAtt->valuestring;
      }

      itemAtt = cJSON_GetObjectItem(itemObj, "url");
      if (itemAtt != NULL && itemAtt->type == cJSON_String) {
        change.Url = itemAtt->valuestring;
      }
    }

    changeset.changes.push_back(change);
  }

  // Debugging.
  //  printf("%s\n", cJSON_Print(data));

  cJSON_Delete(data);

  return true;
}

bool TfsProxy::GetChangesetFile(ChangesetChange &change, const std::string &id)
{
  std::string new_url;

  // See: https://www.visualstudio.com/en-us/docs/integrate/api/tfvc/items
  //
  // The path can be specified as a query parameter as well.
  // This format should be used for certain files (like web.config) that are
  // not accessible by using the path as part of the URL due to the default
  // ASP .NET protection.

  if (change.Path.find("Web.config") != std::string::npos) {
    new_url = _baseurl;
    new_url.append("/items?path=%24");
    new_url.append(change.Path.substr(1));
    new_url.append("&versionType=Changeset&version=");
    new_url.append(id);
  } else {
    new_url = change.Url;
    std::string versionEquals("version=");
    std::size_t found = new_url.find(versionEquals);
    if (found != std::string::npos) {
      new_url.erase(found);
      new_url.append("version=");
      new_url.append(id);
    }
  }
  //printf("Getting %s\n", change.Path.c_str());
  //printf("Getting %s\n", change.Url.c_str());

  // Get specific changeset version.
  HttpRequest req(new_url);
  req.set_ntlm(_username, _password);

  // Filename will be the last part of the path after the final '/'
  std::string::size_type last_slash = change.Path.rfind('/');
  std::string filename;
  if (last_slash != std::string::npos) {
    filename = change.Path.substr(last_slash + 1);
  }

  return req.get_file(filename.c_str());
}

std::vector<TfFileInfo> TfsProxy::GetPathInfo(const std::string& project, const std::string &path) const
{
  std::string url = _baseurl;
  url += "/";
  url += project;
  url += "/_apis/tfvc/items?scopePath=";
  url += utils::UrlEncode(path);
  url += "&recursionLevel=OneLevel";

  std::vector<TfFileInfo> files;

  cJSON *data = sendReq("GET", url, NULL);
  if (data == nullptr) {
    return files;
  }

  cJSON *values = cJSON_GetObjectItem(data, "value");
  if (values != nullptr && values->type == cJSON_Array) {
    int num_items = cJSON_GetArraySize(values);
    for (int i = 0; i < num_items; i++) {

      // Parse each item.
      cJSON *itemObj = cJSON_GetArrayItem(values, i);
      TfFileInfo file;

      // Grab version (int), path (string), url (string), isFolder (bool).
      cJSON *itemAtt = cJSON_GetObjectItem(itemObj, "version");
      if (itemAtt != NULL && itemAtt->type == cJSON_Number) {
        file.Version = itemAtt->valueint;
      }

      itemAtt = cJSON_GetObjectItem(itemObj, "path");
      if (itemAtt != NULL && itemAtt->type == cJSON_String) {
        file.Path = itemAtt->valuestring;
      }
      if (file.Path.compare(path) == 0)
        continue;

      itemAtt = cJSON_GetObjectItem(itemObj, "url");
      if (itemAtt != NULL && itemAtt->type == cJSON_String) {
        file.Url = itemAtt->valuestring;
      }
      file.IsFolder = false;
      itemAtt = cJSON_GetObjectItem(itemObj, "isFolder");
      if (itemAtt != NULL && itemAtt->type == cJSON_True) {
        file.IsFolder = true;
      }
      files.push_back(file);
    }
  }

  return files;
}

void TfsProxy::GetDirectFile(const std::string& filename_url) const
{
  HttpRequest req(filename_url);
  req.set_ntlm(_username, _password);

  // Filename will be the last part of the path after the final '/'
  std::string::size_type last_slash = filename_url.rfind('/');
  std::string filename;
  if (last_slash != std::string::npos) {
    std::string::size_type query_string = filename_url.rfind('?');
    if (query_string != std::string::npos) {
      filename = filename_url.substr(last_slash + 1, query_string - last_slash - 1);
    } else {
      filename = filename_url.substr(last_slash + 1);
    }
  }

  req.get_file(filename.c_str());
}
