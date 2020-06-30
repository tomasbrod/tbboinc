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
