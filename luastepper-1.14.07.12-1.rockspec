package = "LuaStepper"
version = "1.16.06.15-3"
source = {
	url = "git://github.com/aryajur/LuaStepper.git",
	tag = "1.14.07"
}
description = {
	summary = "Allow Lua scripts to run multiple Lua scripts in parallel, platform independent without using threading.",
	detailed = [[
		C module to allow a Lua state to run multiple Lua scripts in parallel. This is platform independent and will work wherever Lua works. LuaStepper is a module to help lua programs control simultaneous execution of multiple Lua threads without them needing to be coroutines.
	]],
	homepage = "http://www.amved.com/milindsweb/LuaStepper.html", 
	license = "MIT" 
}
dependencies = {
	"lua >= 5.1",
}
build = {
	type = "builtin",
	modules = {
		lstepper = "lstepper.lua"
		LuaStepper = "LuaStepper.c"
	}
}