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

#ifndef __TFSPROXY_H__
#define __TFSPROXY_H__

#include <string>
#include <vector>

#include "models/ChangesetInfo.h"
#include "models/TfFileInfo.h"
#include "utils/cJSON.h"

class TfsProxy {
public:
  TfsProxy(const std::string &baseurl,
      const std::string &branch,
      const std::string &username,
      const std::string &password);
  ~TfsProxy();

  std::vector<TfFileInfo> GetPathInfo(const std::string& project, const std::string& path) const;
  void GetDirectFile(const std::string& filename) const;

  bool GetChangesAfter(const std::string &changeset,
    std::vector<ChangesetInfo> &changes);

  bool GetChangesetComment(ChangesetInfo &changeset);
  bool GetChangesetChanges(ChangesetInfo &changeset);

  bool GetChangesetFile(ChangesetChange &change, const std::string &id);

private:
  cJSON *sendReq(const char *method, std::string &url, const char *body) const;

  std::string _baseurl;
  std::string _branch;
  std::string _username;
  std::string _password;
};

#endif /* __TFSPROXY_H__ */

