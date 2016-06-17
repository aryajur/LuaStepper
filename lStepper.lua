-- Module lSteppper
-- Allows Lua programs to run and control multiple Lua scripts in parallel

local lstep_lib = require("LuaStepper")

local type = type
local setfenv = setfenv
local loadstring = loadstring
local load = load
local pairs = pairs
local tostring = tostring
local string = string
local package = package


local ver = "1.16.06.17"	-- Should correspond to the version of the C module

-- Create the module table here
local M = {}
package.loaded[...] = M
if setenv then
	setfenv(1,M)	-- Lua 5.1
else
	_ENV = M		-- Lua 5.2
end
-- Creaate the module table ends

-- Add the normal functions
addTask = lstep_lib.addTask
runLoop = lstep_lib.runLoop
taskStatus = lstep_lib.taskStatus
getNumOfTasks = lstep_lib.getNumOfTasks
closeTask = lstep_lib.closeTask
suspendTask = lstep_lib.suspendTask
resumeTask = lstep_lib.resumeTask
_VERSION = lstep_lib._VERSION
if _VERSION ~= ver then
	package.loaded[...] = "LuaStepper module version not correct. Lua Version: "..ver.." C library version: ".._VERSION
	return nil
end

runCode = lstep_lib.runCode
registerCallBack = lstep_lib.registerCallBack

getTaskData = function(index,key)
	local ret,msg = lstep_lib.getTaskData(index,key)
	if not ret then
		return nil,msg
	end
	if msg == 'TABLE' then
		-- Convert the string to a table and return that
		local sf = {}
		local f,msg
		if setfenv and loadstring then
			f,msg = loadstring(ret)
			if not f then
				return nil,msg
			end
			setfenv(f,sf)
		else
			f,msg = load(ret,nil,nil,sf)
			if not f then
				return nil,msg
			end
		end
		f()
		return sf.t0
	end
	return ret
end

-- Creates lua code for a table which when executed will create a table t0 which would be the same as the originally passed table
-- Handles the following types for keys and values:
-- Keys: Number, String, Table
-- Values: Number, String, Table, Boolean
-- It also handles recursive and interlinked tables to recreate them back
local function tableToString(t)
	local rL = {cL = 1}	-- Table to track recursion into nested tables (cL = current recursion level)
	rL[rL.cL] = {}
	local tabIndex = {}	-- Table to store a list of tables indexed into a string and their variable name
	local latestTab = 0
	do
		rL[rL.cL]._f,rL[rL.cL]._s,rL[rL.cL]._var = pairs(t)
		rL[rL.cL].str = "t0={}"	-- t0 would be the main table
		rL[rL.cL].t = t
		rL[rL.cL].tabIndex = 0
		tabIndex[t] = rL[rL.cL].tabIndex
		while true do
			local key
			local k,v = rL[rL.cL]._f(rL[rL.cL]._s,rL[rL.cL]._var)
			rL[rL.cL]._var = k
			if not k and rL.cL == 1 then
				break
			elseif not k then
				-- go up in recursion level
				--print("GOING UP:     "..rL[rL.cL].str.."}")
				rL[rL.cL-1].str = rL[rL.cL-1].str.."\n"..rL[rL.cL].str
				rL.cL = rL.cL - 1
				if rL[rL.cL].vNotDone then
					-- This was a key recursion so add the key string and then doV
					key = "t"..rL[rL.cL].tabIndex.."[t"..tostring(rL[rL.cL+1].tabIndex).."]"
					rL[rL.cL].str = rL[rL.cL].str.."\n"..key.."="
					v = rL[rL.cL].vNotDone
				end
				rL[rL.cL+1] = nil
			else
				-- Handle the key and value here
				if type(k) == "number" then
					key = "t"..rL[rL.cL].tabIndex.."["..tostring(k).."]"
					rL[rL.cL].str = rL[rL.cL].str.."\n"..key.."="
				elseif type(k) == "string" then
					key = "t"..rL[rL.cL].tabIndex.."."..tostring(k)
					rL[rL.cL].str = rL[rL.cL].str.."\n"..key.."="
				else
					-- Table key
					-- Check if the table already exists
					if tabIndex[k] then
						key = "t"..rL[rL.cL].tabIndex.."[t"..tabIndex[k].."]"
						rL[rL.cL].str = rL[rL.cL].str.."\n"..key.."="
					else
						-- Go deeper to stringify this table
						latestTab = latestTab + 1
						rL[rL.cL].str = rL[rL.cL].str.."\nt"..tostring(latestTab).."={}"	-- New table
						rL[rL.cL].vNotDone = v
						rL.cL = rL.cL + 1
						rL[rL.cL] = {}
						rL[rL.cL]._f,rL[rL.cL]._s,rL[rL.cL]._var = pairs(k)
						rL[rL.cL].tabIndex = latestTab
						rL[rL.cL].t = k
						rL[rL.cL].str = ""
						tabIndex[k] = rL[rL.cL].tabIndex
					end		-- if tabIndex[k] then ends
				end		-- if type(k)ends
			end		-- if not k and rL.cL == 1 then ends
			if key then
				rL[rL.cL].vNotDone = nil
				if type(v) == "table" then
					-- Check if this table is already indexed
					if tabIndex[v] then
						rL[rL.cL].str = rL[rL.cL].str.."t"..tabIndex[v]
					else
						-- Go deeper in recursion
						latestTab = latestTab + 1
						rL[rL.cL].str = rL[rL.cL].str.."{}" 
						rL[rL.cL].str = rL[rL.cL].str.."\nt"..tostring(latestTab).."="..key	-- New table
						rL.cL = rL.cL + 1
						rL[rL.cL] = {}
						rL[rL.cL]._f,rL[rL.cL]._s,rL[rL.cL]._var = pairs(v)
						rL[rL.cL].tabIndex = latestTab
						rL[rL.cL].t = v
						rL[rL.cL].str = ""
						tabIndex[v] = rL[rL.cL].tabIndex
						--print("GOING DOWN:",k)
					end
				elseif type(v) == "number" then
					rL[rL.cL].str = rL[rL.cL].str..tostring(v)
					--print(k,"=",v)
				elseif type(v) == "boolean" then
					rL[rL.cL].str = rL[rL.cL].str..tostring(v)				
				else
					rL[rL.cL].str = rL[rL.cL].str..string.format("%q",tostring(v))
					--print(k,"=",v)
				end		-- if type(v) == "table" then ends
			end		-- if doV then ends
		end		-- while true ends here
	end		-- do ends
	return rL[rL.cL].str
end

setTaskData = function(index,tab)
	if not type(tab) == 'table' then
		return nil, '2nd argument expected to be a table'
	end
	local setTabs = {}
	local setVals = {}
	for k,v in pairs(tab) do
		if type(v) == 'table' then
			setTabs[k] = v
		else
			setVals[k] = v
		end
	end
	-- Now set the simple Data first
	local ret,msg = lstep_lib.setTaskData(index,setVals)
	if not ret then
		return nil,msg
	end
	-- Now set the tables
	for k,v in pairs(setTabs) do
		ret,msg = lstep_lib.setTaskTable(index,k,tableToString(v))
		if not ret then
			return nil,msg
		end
	end
	
	return true		
end




