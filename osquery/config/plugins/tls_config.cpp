/**
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed as defined on the LICENSE file found in the
 *  root directory of this source tree.
 */

// clang-format off
// Keep it on top of all other includes to fix double include WinSock.h header file
// which is windows specific boost build problem
#include <osquery/remote/utility.h>
// clang-format on

#include <osquery/config/config.h>
#include <osquery/config/plugins/tls_config.h>
#include <osquery/dispatcher.h>
#include <osquery/enroll.h>
#include <osquery/flags.h>
#include <osquery/registry.h>
#include <osquery/remote/requests.h>
#include <osquery/remote/serializers/json.h>
#include <osquery/utils/json/json.h>
#include <osquery/utils/chars.h>

#include <sstream>
#include <vector>

namespace osquery {

CLI_FLAG(uint64,
         config_tls_max_attempts,
         3,
         "Number of attempts to retry a TLS config/enroll request");

/// Config retrieval TLS endpoint (path) using TLS hostname.
CLI_FLAG(string,
         config_tls_endpoint,
         "",
         "TLS/HTTPS endpoint for config retrieval");

DECLARE_bool(tls_node_api);
DECLARE_bool(enroll_always);

REGISTER(TLSConfigPlugin, "config", "tls");

Status TLSConfigPlugin::setUp() {
  if (FLAGS_enroll_always && !FLAGS_disable_enrollment) {
    // clear any cached node key
    clearNodeKey();
    auto node_key = getNodeKey("tls");
    if (node_key.size() == 0) {
      // Could not generate a node key, continue logging to stderr.
      return Status(1, "No node key, TLS config failed.");
    }
  }

  uri_ = TLSRequestHelper::makeURI(FLAGS_config_tls_endpoint);
  return Status(0, "OK");
}

Status TLSConfigPlugin::genConfig(std::map<std::string, std::string>& config) {
  std::string json;
  JSON params;
  if (FLAGS_tls_node_api) {
    // The TLS node API morphs some verbs and variables.
    params.add("_get", true);
  }

  auto s = TLSRequestHelper::go<JSONSerializer>(
      uri_, params, json, FLAGS_config_tls_max_attempts);
  if (s.ok()) {
    if (FLAGS_tls_node_api) {
      // The node API embeds configuration data (JSON escaped).

      JSON tree;
      Status parse_status = tree.fromString(json);
      if (!parse_status.ok()) {
        VLOG(1) << "Could not parse JSON from TLS config node API";
      }

      // Re-encode the config key into JSON.
      auto it = tree.doc().FindMember("config");
      config["tls_plugin"] =
          unescapeUnicode(it != tree.doc().MemberEnd() && it->value.IsString()
                              ? it->value.GetString()
                              : "");
    } else {
      config["tls_plugin"] = json;
    }
  }

  return s;
}
}
