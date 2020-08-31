// Copyright (c) 2020 Devn Smith <devin@devinsmith.net>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#include "commands.h"
#include "configuration/configuration.h"
#include "services/http.h"
#include "utils/filesys.h"
#include "utils/logging.h"

static const char *_pname = "tf";

// Global configuration
Configuration AppConfig;

static void usage(int err)
{
  if (err) {
    fprintf(stderr, "Unknown command\n");
  }

  fprintf(stderr, "usage: %s (cmd)\n", _pname);
  fprintf(stderr, "\tclone    - get latest.\n");
  exit(err);
}

int main(int argc, char *argv[])
{
  int rc = AppConfig.Load(filesys::get_config_path(".tfsrc"));
  if (rc == -1) {
    fprintf(stderr, "This program requires a configuration file. Please refer "
        "to the documentation on how to create a configuration file.\n");
    exit(1);
  } else if (rc == -2) {
    exit(1);
  }

  if (argc < 2) {
    usage(0);
  }

  log_init();
  http::http_lib_startup();

  if (!execute_cmd(argv[1], &argv[2], argc - 2)) {
    usage(1);
  }

  http::http_lib_shutdown();

  return 0;
}
