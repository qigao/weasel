add_library(WeaselIPC STATIC
  ActionLoader.cpp Committer.cpp Configurator.cpp ContextUpdater.cpp
  Deserializer.cpp PipeChannel.cpp ResponseParser.cpp Styler.cpp
  WeaselClientImpl.cpp)
target_link_libraries(WeaselIPC PUBLIC weasel_headers
  PRIVATE Boost::interprocess Boost::serialization Boost::wserialization Boost::thread)
set_target_properties(WeaselIPC PROPERTIES FOLDER "Libraries")
