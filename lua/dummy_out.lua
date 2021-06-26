print("Boinc LUA checking in")
local name="output.txt"
local fh = io.open(name, "w")
if not fh then
  error("Failed to open file "..name.." for writing")
end
local res, msg = fh:write(content)
if not res then
  error("Failed to write file "..name.." "..msg)
end
fh:close()
boinc.finish(0,"driver script finished")
