#include "stdafx.h"
#include "Deserializer.h"
#include "ActionLoader.h"
#include "Committer.h"
#include "ContextUpdater.h"
#include "Configurator.h"
#include "Styler.h"

using namespace weasel;

void Deserializer::Initialize(ResponseParser* pTarget) {
  // Each response begins with only its action loader enabled.
  Require(L"action", pTarget);
}

bool Deserializer::Require(std::wstring const& action,
                           ResponseParser* pTarget) {
  if (!pTarget)
    return false;

  // Fixed factories need no mutable process-wide registration state.
  Factory factory;
  if (action == L"action")
    factory = ActionLoader::Create;
  else if (action == L"commit")
    factory = Committer::Create;
  else if (action == L"ctx")
    factory = ContextUpdater::Create;
  else if (action == L"status")
    factory = StatusUpdater::Create;
  else if (action == L"config")
    factory = Configurator::Create;
  else if (action == L"style")
    factory = Styler::Create;
  else
    return false;

  pTarget->deserializers[action] = factory(pTarget);
  return true;
}
