-- 工作区的xmake.lua
set_project("weasel")

-- 定义全局变量
set_xmakever("2.9.4")
set_languages("c++17")
set_runtimes("MT")  -- 设置运行时库为静态链接，避免/MD与/MT冲突
add_defines("UNICODE", "_UNICODE")
add_defines("WINDOWS")
add_defines("MSVC")
add_defines(
  "VERSION_MAJOR=" .. (os.getenv("VERSION_MAJOR") or "0"),
  "VERSION_MINOR=" .. (os.getenv("VERSION_MINOR") or "0"),
  "VERSION_PATCH=" .. (os.getenv("VERSION_PATCH") or "0"),
  "PRODUCT_VERSION=" .. (os.getenv("PRODUCT_VERSION") or "0.0.0.0")
)

-- One pinned checkout supplies both WTL and clean-room ATL/COM Server.
-- CMake consumers use OpenWTL::WTL; xmake consumes these same source headers.
local openwtl_root = path.join(os.projectdir(), "external", "OpenWTL")
assert(os.isdir(path.join(openwtl_root, "alt", "include")) and
       os.isdir(path.join(openwtl_root, "wtl", "Include")),
       "Missing pinned external/OpenWTL submodule")
add_includedirs(path.join(openwtl_root, "alt", "include"))
add_includedirs(path.join(openwtl_root, "wtl", "Include"))
add_includedirs("$(projectdir)/include")
-- 设置Boost库的全局路径
boost_root = os.getenv("BOOST_ROOT")
boost_include_path = boost_root
boost_lib_path = boost_root .. "/stage/lib"
add_includedirs(boost_include_path)
add_linkdirs(boost_lib_path)
add_cxflags("/utf-8 /MP /O2 /Oi /Gm- /EHsc /MT /GS /Gy /fp:precise /Zc:wchar_t /Zc:forScope /Zc:inline /external:W3 /Gd /TP")
add_ldflags("/TLBID:1 /DYNAMICBASE /NXCOMPAT")

-- Do not import Microsoft's ATLMFC include/lib directories from VS. All
-- ATL/WTL source is owned by the pinned OpenWTL checkout above.
for include in string.gmatch(os.getenv("include") or "", "([^;]+)") do
  if not include:lower():find("atlmfc", 1, true) then
    add_includedirs(include)
  end
end

if is_arch("x64") then
  add_linkdirs("$(projectdir)/lib64")
elseif is_arch("x86") then
  add_linkdirs("$(projectdir)/lib")
end
add_links("shell32", "advapi32", "gdi32", "user32", "uuid", "ole32",
          "oleaut32", "comctl32", "shlwapi", "comdlg32", "uxtheme",
          "msimg32", "dwmapi")

includes("WeaselIPC", "WeaselUI", "WeaselTSF")

if is_arch("x64") or is_arch("x86") then
  includes("RimeWithWeasel", "WeaselIPCServer", "WeaselServer", "WeaselDeployer")
end

if is_arch("x86") then
  includes("WeaselSetup")
end

if is_mode("debug") then
  includes("test/TestWeaselIPC")
  includes("test/TestResponseParser")
else
  add_cxflags("/GL")
  add_ldflags("/LTCG /INCREMENTAL:NO", {force = true})
end

rule("subcmd")
  on_load(function(target)
    target:add("ldflags", "/SUBSYSTEM:CONSOLE")
  end)
rule("subwin")
  on_load(function(target)
    target:add("ldflags", "/SUBSYSTEM:WINDOWS")
  end)

rule("add_rcfiles")
  on_load(function(target)
    target:add("files", path.join(target:scriptdir(), "*.rc"),
      {defines = {"VERSION_MAJOR=" .. (os.getenv("VERSION_MAJOR") or "0"),
      "VERSION_MINOR=" .. (os.getenv("VERSION_MINOR") or "0"),
      "VERSION_PATCH=" .. (os.getenv("VERSION_PATCH") or "0"),
      "FILE_VERSION=" .. (os.getenv("FILE_VERSION") or "\"0.0.0.0\""),
      "PRODUCT_VERSION=" .. (os.getenv("PRODUCT_VERSION") or "\"0.0.0.0\"")
    }})
  end)
rule("use_weaselconstants")
  on_load(function(target)
    function check_include_weasel_constants_in_dir(dir)
      local files = os.files(path.join(dir, "**.h"))
      table.join2(files, os.files(path.join(dir, "**.cpp")))
      for _, file in ipairs(files) do
        local content = io.readfile(file)
        if content:find('#include%s+"WeaselConstants%.h"') or content:find('#include%s+<WeaselConstants%.h>') then
          return true
        end
      end
      return false
    end
    if check_include_weasel_constants_in_dir(target:scriptdir()) then
      target:add("rcflags", {
        "/dVERSION_MAJOR=" .. (os.getenv("VERSION_MAJOR") or "0"),
        "/dVERSION_MINOR=" .. (os.getenv("VERSION_MINOR") or "0"),
        "/dVERSION_PATCH=" .. (os.getenv("VERSION_PATCH") or "0"),
        "/dFILE_VERSION=" .. (os.getenv("FILE_VERSION") or "\"0.0.0.0\""),
        "/dPRODUCT_VERSION=" .. (os.getenv("PRODUCT_VERSION") or "\"0.0.0.0\"")
      })
    end
  end)
