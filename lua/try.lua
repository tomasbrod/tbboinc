boinc.dofile("library.lua")

wu.batch = 172
wu.type = "CoT"

bbl.ddep.boincapi = { detect=function(key)
  bbl.write_to_file("config.h","")
  return {
    l={"boinc_api","boinc"},
    inc={"."}
  }
end }

cxx = bbl.dep.cxx()
cxx.warnings=0
cxx.optimiz=1
cxx:use(bbl.dep.boincapi)
cxx:use(bbl.dep.pthread)

cxx:compileExe("main.exe",{"kanon_app.cpp"})

boinc.exec("./main.exe",{})
