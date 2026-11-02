add_library(RimeWithWeasel STATIC RimeWithWeasel.cpp WeaselUtility.cpp)
target_link_libraries(RimeWithWeasel PUBLIC WeaselUI Rime::Rime
  PRIVATE Boost::interprocess Boost::serialization Boost::wserialization Boost::thread)
set_target_properties(RimeWithWeasel PROPERTIES FOLDER "Libraries")
