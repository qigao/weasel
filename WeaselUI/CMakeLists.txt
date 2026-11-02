add_library(WeaselUI STATIC
  d2d.cpp FullScreenLayout.cpp HorizontalLayout.cpp Layout.cpp
  StandardLayout.cpp VerticalLayout.cpp VHorizontalLayout.cpp
  WeaselPanel.cpp WeaselUI.cpp)
target_link_libraries(WeaselUI PUBLIC weasel_headers
  PRIVATE d2d1 dwrite d3d11 dxgi dxguid dcomp shcore windowscodecs usp10)
set_target_properties(WeaselUI PROPERTIES FOLDER "Libraries")
