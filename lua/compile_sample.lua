-- compile_sample.lua

boinc.dofile("library.lua")
wu.batch = 172
wu.type = "CoT"
include_user_config()

dep={}

function dep.cpp11(name)
  r={
    cmd="g++",
    compileExe = function(self, out, inp)
      local cmd=self.cmd.." -o "..out.." "..inp[0]
      local r1, r2, r3 = os.execute(cmd)
      if r1 == false then
        commandFailed(cmd,r2,r3)
      end
    end,
    commandFailed = function(cmd, r2, r3)
      print("Compiler failed. "..r2.." "..r3)
      boinc.finish(1,"compiler failed")
    end,      
  }
  return r
end

cxx = dep.cpp11("app")

cxx.warnings=1
cxx.optimiz=2
cxx.addLib("m")
cxx.compileExe("sample.exe",{"sample.cpp"})
try_command("./sample.exe")

-- better syntax for app build and exec
app=prepare_app({name="sample", ver=100})
-- call existing exe with arg "--check boinc sample 100"
-- download from https:://boinc.tbrada.eu/download/sample_100.tar.gz and extract
-- call lua within the sources (to build it)
-- delete sources
-- call the check again
app:exec()
