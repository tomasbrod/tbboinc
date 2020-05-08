-- GCC-like compiler wrapper
dep={}
dep.template={}
wu={}

wu.batch = 172
wu.type = "CoT"

function dep.template.CompilerBase()
  return {
    warnings=1,
    optimiz=false,
    define={},
    
  }
end

function dep.template.CPPLikeGCC()
  r=dep.template.CompilerBase()

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

  function r.commandFailed(self,cmd, r2, r3)
    print("Compiler failed. "..r2.." "..r3)
    boinc.finish(1,"compiler failed")
  end

  function r.check(self)
    write_file("temp.cpp","#include <vector>\nint main() {return 0;}\n")
    self:compileObj("temp.o",{"temp.cpp"})
  end

  return r
end

cxx=dep.template.CPPLikeGCC()
cxx.cmd="echo "..cxx.cmd
cxx.define.A=1
cxx.define.B=true
cxx:compileObj("test.o",{"a.cpp","b.c"})
cxx:compileExe("test.exe",{"test.o"})
