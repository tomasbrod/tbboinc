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

