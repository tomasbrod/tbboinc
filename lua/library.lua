-- The Brod Boinc Library

-- globals
-- boinc - boinc_api exported by C++
wu={} -- info about the workunit
bbl={} -- boinc t.brada library

bbl.dep={} -- known dependencies
bbl.template={} -- templates for dependency management
bbl.ddep={} -- dependency detection

-- base class for all compilers
function bbl.template.CompilerBase()
  return {
    warnings=1,
    optimiz=false,
    define={},
  }
end

-- link to support page used in notices
function bbl.SupportLink()
  return "https://boinc.tbrada.eu/"
end

-- report first use of dependencies
function bbl.PrintUsedDep(name,d)
  print("DEP: "..name)
end

-- unsatisfied dependency handler
function bbl.Unsatisfied(name, extra)
  local msg
  if extra then
    msg="Missing "..name..". "..extra.." "..dep.template.GetSupportLink()
  else
    msg="Unsatisfied dependency: "..name..". "..dep.template.GetSupportLink()
  end
  boinc.temp_exit( 300, msg, 1)
end

-- Default failure handler. Called via bbl.Failure or override when command fails.
function bbl.DefaultFailure(err, command)
  local msg = "Error "..err..", ("..command..")"
  boinc.finish(2, msg, 1)
end

-- Current failure handler. Actually called when command fails.
bbl.Failure=bbl.DefaultFailure

-- the bbl.dep metatable has unknown key lookup function
-- calls bbl.detect.<name>
-- templates are in dep.template.<name>
-- most dependencies will be static in the dep table and not checked
-- but some may be detected and checked using a compiler (which?)
-- user can always override detection, by creating static dep entry.

bbl.template.depmetatable={}
function bbl.template.depmetatable.__index(tab, key)
  -- new dependency
  local ddep = bbl.ddep[key] or return bbl.Unsatisfied(key,nil)
  local dep
  if ddep.static then
    dep= ddep.static
  elseif ddep.detect then
    dep= ddep.detect(key)
  elseif ddep.build or ddep.stdbuild then
    dep= bbl.template.GetBuildableDep(key)
  end
  dep or return bbl.Unsatisfied(key,nil)
  bbl.dep[key] = dep
  bbl.PrintUsedDep(key,dep)
  if ddep.check then
    ddep.check(key)
  end
end

setmetatable( bbl.dep, bbl.template.depmetatable )

function bbl.dump(o)
   if type(o) == 'table' then
      local s = '{ '
      for k,v in pairs(o) do
         if type(k) ~= 'number' then k = '"'..k..'"' end
         s = s .. '['..k..'] = ' .. bbl.dump(v) .. ','
      end
      return s .. '} '
   else
      return tostring(o)
   end
end

function bbl.try_command(cmd)
  print("try_command",cmd)
  local r1, r2, r3 = os.execute(cmd)
  if r1 == true then
    print("success")
  else
    print("failure", r2, r3)
  end
end

function bbl.write_to_file(name, content)
  local fh = io.open(name, "w")
  if not fh then
    error("Failed to open file "..name.." for writing")
  end
  local res, msg = fh:write(content)
  if not res then
    error("Failed to write file "..name.." "..msg)
  end
  fh:close()
end

function bbl.template.CPPLikeGCC()
  r=bbl.template.CompilerBase()

  r.cmd="g++ -std=c++11"

  function r.runCompiler(self, pre, out, inp)
    local cmd= self.cmd.." -o "..out
    if self.warnings then
      cmd=cmd.." -Wall"
    end
    if self.optimiz then
      cmd=cmd.." -O3"
    end
    if pre then
      cmd=cmd.." "..pre
    end
    for _,sourcefile in pairs(inp) do
      cmd=cmd.." "..sourcefile
    end
    for var,val in pairs(self.define) do
      if val ~= true then
        cmd=cmd.." -D"..var.."="..val
      else
        cmd=cmd.." -D"..var
      end
    end

    -- todo: quote
    print(cmd)

    local r1, r2, r3 = os.execute(cmd)
    if not r1 then
      self:commandFailed("CXX -o "..out,r2,r3)
    end
  end

  function r.compileExe(self, out, inp)
    return self:runCompiler("",out,inp)
  end
  function r.compileObj(self, out, inp)
    return self:runCompiler("-c ",out,inp)
  end

  function r.commandFailed2(self, cmd, r2, r3)
    print("Compiler failed. "..r2.." "..r3)
    bbl.Failure("compile", cmd)
  end
  r.commandFailed=r.commandFailed2

  function r.check(s)
    bbl.write_to_file("compile_test.cpp","#include <vector>\nint main() {return 0;}\n")
    local good=true
    s.commandFailed = function(self, cmd, r2, r3)
      good= false
    end
    s:compileObj("compile_test.o",{"compile_test.cpp"})
    s.commandFailed=s.commandFailed2
    return good
  end

  return r
end

bbl.ddep = { detect = function(key)
  -- todo: more general solution to inst and check

  local function watcom()
    local r= bbl.template.CPPLikeGCC()
    r.cmd="watcom++ -std=c++11"
    return r
  end
  if false and watcom():check() then
    return watcom
  end

  if bbl.template.CPPLikeGCC():check() then
    return bbl.template.CPPLikeGCC -- return constructor
  end

  local function clang()
    local r= bbl.template.CPPLikeGCC()
    r.cmd="clang++ -std=c++11"
    return r
  end
  if clang():check() then
    return clang
  end

  bbl.Unsatisfied("cxx","C++ compiler")
end }

function bbl.template.GetBuildableDep(key)
  -- get artifact
  -- get remote version
  -- if outdated or check fails, build
  -- ?
end

-- Some standard static dependencies
bbl.detect.m = { l="m" }
bbl.detect.pthread = { l="pthread" }

