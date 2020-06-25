-- sample user override .lua

-- example of overriding compiler
bbl.dep.cxx= function()
  local r= bbl.template.CPPLikeGCC()
  r.cmd="watcom++ -std=c++11"
  return r
end

