#pragma once
#include <ResponseParser.h>
#include <functional>

namespace weasel {

class Deserializer {
 public:
  typedef std::vector<std::wstring> KeyType;
  typedef std::shared_ptr<Deserializer> Ptr;
  typedef std::function<Ptr(ResponseParser* pTarget)> Factory;

  Deserializer(ResponseParser* pTarget) : m_pTarget(pTarget) {}
  virtual ~Deserializer() {}
  virtual void Store(KeyType const& key, std::wstring const& value) {}

  static void Initialize(ResponseParser* pTarget);
  static bool Require(std::wstring const& action, ResponseParser* pTarget);

 protected:
  ResponseParser* m_pTarget;

};

}  // namespace weasel
