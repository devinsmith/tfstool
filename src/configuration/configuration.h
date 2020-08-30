/*
 * Copyright (c) 2014-2015 Devin Smith <devin@devinsmith.net>
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

#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include <string>
#include <vector>

struct ConfigValue {
  std::string section;
  std::string name;
  std::string value;
};

class Configuration {
public:

  // Returns 0 on success, otherwise returns -1 for file error and -2 for parsing error.
  int Load(const std::string &filename);

  void Set(const std::string &section, const std::string &name,
    const std::string &value);

  std::string Get(const std::string &section, const std::string &name);
private:
  std::vector<ConfigValue> values;
};

extern Configuration AppConfig; // in main.cpp, but exposed everywhere.

#endif /* __CONFIGURATION_H__ */
