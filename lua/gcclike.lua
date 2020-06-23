-- GCC-like compiler wrapper

-- globals
-- boinc - boinc_api exported by C++
wu={} -- info about the workunit
bbl={} -- boinc t.brada library
bbl.dep={} -- dependencies
bbl.template={} -- templates for dependency management
bbl.detect={} -- detection routines for dependencies

wu.batch = 172
wu.type = "CoT"

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

-- unsatisfied dependency handler
function bbl.Unsatisfied(name, extra)
  local msg
  if extra then
    msg="Missing "..name..". "..extra.." "..dep.template.GetSupportLink()
  else
    "Unsatisfied dependency: "..name..". "..dep.template.GetSupportLink()
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
  -- undefined dependency
  local detect = bbl.detect[key]
  if type(detect) ~= function then
    -- no detect method - error
    bbl.Unsatisfied(key,nil)
    return nil
  else
    -- use detect method
    local d = detect()
    bbl.dep[key] = d
    return d
  end
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
    
    local r1, r2, r3 = os.execute(cmd)
    if r1 == false then
      commandFailed(cmd,r2,r3)
    end
  end

  function r.compileExe(self, out, inp)
    return self:runCompiler("",out,inp)
  end
  function r.compileObj(self, out, inp)
    return self:runCompiler("-c ",out,inp)
  end

  function r.commandFailed2(self,cmd, r2, r3)
    print("Compiler failed. "..r2.." "..r3)
    bbl.Failure("compile", cmd)
  end
  r.commandFailed=r.commandFailed2

  function r.check(self)
    write_file("temp.cpp","#include <vector>\nint main() {return 0;}\n")
    local good=true
    r.commandFailed = function(self, cmd, r2, r3)
      good= false
    end
    self:compileObj("temp.o",{"temp.cpp"})
    r.commandFailed=r.commandFailed2
    return good
  end

  return r
end

function bbl.detect.cxx()
  -- instantiate and check
  if bbl.template.CPPLikeGCC().check() then
    return bbl.template.CPPLikeGCC -- return constructor
  end
  local function clang()
    local r= bbl.template.CPPLikeGCC()
    r.cmd="clang++ -std=c++11"
    return r
  end
  if clang().check() then
    return clang
  end
  bbl.Unsatisfied("cxx","C++ compiler")
end

cxx=bbl.template.CPPLikeGCC()
-- cxx=bbl.dep.cxx()
cxx.cmd="echo "..cxx.cmd
cxx.define.A=1
cxx.define.B=true
cxx:compileObj("test.o",{"a.cpp","b.c"})
cxx:compileExe("test.exe",{"test.o"})

-- error reporting: a) error() - lua-like, but needs global pcall wrapper
-- kinds of errors: dependency not found, compile error
-- we will have a global error hook and function to report message to user
-- missing dependency function: a) not found at all b) detect function failed
--   the detect function can report more info than just name of the dep
-- compile error goes to generic_failure ( command, pwd, ... )
--   that will also include last n lines of stderr.txt
-- eh, boinc supports only one line of exit notice
-- the detect function can also invoke compiling of the dependency
