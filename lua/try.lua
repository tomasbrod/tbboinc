boinc.dofile("library.lua")

wu.batch = 172
wu.type = "CoT"

bbl.try_command("locale")
cxx = bbl.dep.cxx()
_=bbl.dep.m
_=bbl.dep.pthread
cxx.define.A=1
cxx.define.B=true
cxx:compileObj("a.o",{"a.cpp"})
cxx:compileObj("b.o",{"b.c"})
print("using false compiler")
cxx.cmd="false "..cxx.cmd
cxx:compileExe("test.exe",{"a.o","b.o"})

-- error reporting: a) error() - lua-like, but needs global pcall wrapper
-- kinds of errors: dependency not found, compile error
-- we will have a global error hook and function to report message to user
-- missing dependency function: a) not found at all b) detect function failed
--   the detect function can report more info than just name of the dep
-- compile error goes to generic_failure ( command, pwd, ... )
--   that will also include last n lines of stderr.txt
-- eh, boinc supports only one line of exit notice
-- the detect function can also invoke compiling of the dependency

-- built artifact leaves prj/lua/artifact.lua descriptor
-- additional requirements on artifacts
-- descriptor [revision,files]
-- check method
-- detect/build method
-- the dep loader must know its a buildable:

-- detect: find dep installed on client
-- check: check (duh) that dep meets requirements
  -- exe with check support: --check "reqirements"
  -- other: custom method

-- vbl.dep: known dependencies
-- vbl.ddep: stuff
  -- dep table (static definition)
  -- detect method
  -- check method
  -- downloadable flag
  -- build method (if used)

function get_artifact_descr(name)
  local path = get_artifact_dir() .. "/"..name..".lua"
  local descr = dofile(path)
  return descr
end

function get_artifact(name)
  local desc = get_artifact_descr(name)
  if descr and check_artifact(name, descr) then
    return descr end
  local descr = artifact_detect_or_build(name)
  if check_artifact(name, descr) then
    return descr end
    else Failure() end
end

-- another type of exe: exact version of code linked from the WU
-- {maybe cached on the client}
-- the source archive IS part of the WU [or GIT]
