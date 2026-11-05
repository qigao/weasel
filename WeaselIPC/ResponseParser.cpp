#include "stdafx.h"
#include <StringAlgorithm.hpp>
#include <WeaselIPC.h>
#include "Deserializer.h"
#include <exception>
#include <optional>
#include <type_traits>
#include <utility>

using namespace weasel;

ResponseParser::ResponseParser(std::wstring* commit,
                               Context* context,
                               Status* status,
                               Config* config,
                               UIStyle* style)
    : p_commit(commit),
      p_context(context),
      p_status(status),
      p_config(config),
      p_style(style) {}

bool ResponseParser::operator()(LPWSTR buffer, UINT length) {
  if (!buffer || length == 0 || length > kMaxResponseLength)
    return false;

  // Publication cannot allocate or throw after the first output is changed.
  static_assert(std::is_nothrow_swappable_v<std::wstring>);
  static_assert(std::is_nothrow_swappable_v<Context>);
  static_assert(std::is_nothrow_swappable_v<Status>);
  static_assert(std::is_nothrow_swappable_v<Config>);
  static_assert(std::is_nothrow_swappable_v<UIStyle>);
  try {
    std::optional<std::wstring> commit;
    std::optional<Context> context;
    std::optional<Status> status;
    std::optional<Config> config;
    std::optional<UIStyle> style;
    if (p_commit) commit.emplace(*p_commit);
    if (p_context) context.emplace(*p_context);
    if (p_status) status.emplace(*p_status);
    if (p_config) config.emplace(*p_config);
    if (p_style) style.emplace(*p_style);

    ResponseParser staged(commit ? &*commit : nullptr,
                          context ? &*context : nullptr,
                          status ? &*status : nullptr,
                          config ? &*config : nullptr,
                          style ? &*style : nullptr);
    Deserializer::Initialize(&staged);
    wbufferstream stream(buffer, length);
    std::wstring line;
    while (std::getline(stream, line) && !stream.eof()) {
      if (line == L".") {
        using std::swap;
        if (p_commit) swap(*p_commit, *commit);
        if (p_context) swap(*p_context, *context);
        if (p_status) swap(*p_status, *status);
        if (p_config) swap(*p_config, *config);
        if (p_style) swap(*p_style, *style);
        return true;
      }
      staged.Feed(line);
    }
  } catch (const std::exception&) {
    // Malformed archives and allocation failures are rejected at the host boundary.
  }
  return false;
}

void ResponseParser::Feed(const std::wstring& line) {
  // ignore blank lines and comments
  if (line.empty() || line.find_first_of(L'#') == 0)
    return;

  Deserializer::KeyType key;
  std::wstring value;

  // extract key (split by L'.') and value
  std::wstring::size_type sep_pos = line.find_first_of(L'=');
  if (sep_pos == std::wstring::npos)
    return;
  split(key, line.substr(0, sep_pos), L".");
  if (key.empty())
    return;
  value = line.substr(sep_pos + 1);

  // first part of the key serve as action type
  std::wstring const& action = key[0];

  // get required action deserializer instance
  std::map<std::wstring, Deserializer::Ptr>::iterator i =
      deserializers.find(action);
  if (i == deserializers.end()) {
    // line ignored... since corresponding deserializer is not active
    return;
  }

  // dispatch
  Deserializer::Ptr p = i->second;
  p->Store(key, value);
}
