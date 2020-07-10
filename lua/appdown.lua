boinc.dofile("library.lua")

wu.batch = 172
wu.type = "CoT"

-- how to download app?

-- where to put the binary?

-- how to check executable fitness?
-- exe -check "checkstring"

-- how to check for optional updates

-- how/where to download source?
-- mutex on building the same image
-- download and unpack in the slot dir
-- might run into wu disk size limit - use some temp dir

-- how to build it?
-- invoke build.lua in the downloaded thing

-- then check again

function get_exe( name, check )
  local path = get_image_path(name..".exe")
  if check_exe( path, check ) then return path end
  local tmp = get_build_dir( name )
  download_sources( tmp, name )
  dofile(tmp.."build.lua")
  if check_exe( path, check ) then return path else
    report_check_failure()
  end
end


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
  -- handle file not found -> nil
  -- warn on syntax error
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
-- this type of exe is not handled via the dependency system
