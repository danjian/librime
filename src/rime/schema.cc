//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-05-08 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <rime/schema.h>

namespace rime {

Schema::Schema() : schema_id_(".default") {
  config_.reset(Config::Require("config")->Create("default"));
  FetchUsefulConfigItems();
}

Schema::Schema(const string& schema_id) : schema_id_(schema_id) {
  config_.reset(boost::starts_with(schema_id_, ".")
                    ? Config::Require("config")->Create(schema_id.substr(1))
                    : Config::Require("schema")->Create(schema_id));
  FetchUsefulConfigItems();
}

void Schema::FetchUsefulConfigItems() {
  if (!config_) {
    schema_name_ = schema_id_ + "?";
    return;
  }
  if (!config_->GetString("schema/name", &schema_name_)) {
    schema_name_ = schema_id_;
  }
  if (!config_->GetString("schema/layout", &layout_)) {
    layout_ = "";
  }
  if (!config_->GetString("schema/punctuation", &punctuation_)) {
    punctuation_ = "";
  }
  config_->GetInt("menu/page_size", &page_size_);
  if (page_size_ < 1) {
    page_size_ = 5;
  }
  config_->GetString("menu/alternative_select_keys", &select_keys_);
  config_->GetBool("menu/page_down_cycle", &page_down_cycle_);
  FetchOptions();
}

void Schema::FetchOptions() {
  options_.clear();
  if (!config_) {
    return;
  }
  if (auto list = config_->GetList("options")) {
    for (size_t i = 0; i < list->size(); ++i) {
      if (auto entry = As<ConfigMap>(list->GetAt(i))) {
        SchemaOptionInfo info;
        if (auto name = entry->GetValue("name")) {
          name->GetString(&info.name);
        }
        if (auto key = entry->GetValue("key")) {
          key->GetString(&info.key);
        }
        if (auto keys = As<ConfigList>(entry->Get("keys"))) {
          for (size_t j = 0; j < keys->size(); ++j) {
            if (auto item = keys->GetValueAt(j)) {
              string key_name;
              item->GetString(&key_name);
              info.keys.push_back(key_name);
            }
          }
        }
        if (auto lock_value = entry->GetValue("lock")) {
          lock_value->GetBool(&info.lock);
        }
        if (auto value = entry->GetValue("value")) {
          value->GetBool(&info.value);
        }
        options_.push_back(info);
      }
    }
  }
}

Config* SchemaComponent::Create(const string& schema_id) {
  return config_component_->Create(schema_id + ".schema");
}

}  // namespace rime
