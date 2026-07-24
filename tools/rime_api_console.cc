#include <iostream>
#include <string>

#include <rime_api.h>
#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;

static RimeApi* rime = nullptr;
static RimeSessionId session = 0;

static bool InitRime() {
  rime = rime_get_api();

  RIME_STRUCT(RimeTraits, traits);
  traits.app_name = "rime.http";

  rime->setup(&traits);

  rime->initialize(nullptr);

  if (rime->start_maintenance(True))
    rime->join_maintenance_thread();

  session = rime->create_session();

  return session != 0;
}

static void ReleaseRime() {
  if (session)
    rime->destroy_session(session);
  rime->finalize();
}

static json BuildResult() {
  json result;

  RIME_STRUCT(RimeCommit, commit);
  RIME_STRUCT(RimeContext, context);

  result["commit"] = "";
  result["preedit"] = "";
  result["candidates"] = json::array();

  if (rime->get_commit(session, &commit)) {
    if (commit.text)
      result["commit"] = commit.text;
    rime->free_commit(&commit);
  }

  auto start = std::chrono::steady_clock::now();

  bool ok = rime->get_context(session, &context);
  auto end = std::chrono::steady_clock::now();
  auto cost_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();
  result["get_context_us"] = cost_us;
  if (ok) {
    if (context.composition.preedit)
      result["preedit"] = context.composition.preedit;

    for (int i = 0; i < context.menu.num_candidates; i++) {
      json item;
      item["text"] = context.menu.candidates[i].text
                         ? context.menu.candidates[i].text
                         : "";

      item["comment"] = context.menu.candidates[i].comment
                            ? context.menu.candidates[i].comment
                            : "";

      result["candidates"].push_back(item);
    }

    rime->free_context(&context);
  }
  return result;
}

int main() {
  if (!InitRime()) {
    std::cerr << "Init Rime Failed." << std::endl;
    return -1;
  }

  httplib::Server server;

  server.Get("/", [](const httplib::Request&, httplib::Response& res) {
    res.set_content("Rime HTTP Server", "text/plain");
  });

  server.Get("/input", [](const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("input")) {
      res.status = 400;
      res.set_content("{\"error\":\"missing input\"}", "application/json");
      return;
    }

    if (!session) {
      session = rime->create_session();
    }

    std::string input = req.get_param_value("input");

    if (!rime->simulate_key_sequence(session, input.c_str())) {
      res.status = 500;
      res.set_content("{\"error\":\"simulate failed\"}", "application/json");
      return;
    }
    auto j = BuildResult();

    rime->destroy_session(session);
    session = 0;

    res.set_content(j.dump(2), "application/json");
  });

  std::cout << "Listen :8080" << std::endl;

  server.listen("0.0.0.0", 8080);

  ReleaseRime();

  return 0;
}
