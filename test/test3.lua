-- Test case to test the detection of the switch
ls = require("lStepper")
print("LuaStepper version is:",ls._VERSION)

script1 = [[
	print("Starting task 1")
	ws = require("waitswitch")
	while true do
		ws.waitswitch()
	end
]]

script2 = [[
	print("Starting task 2")
	ws = require("waitswitch")
	while true do
		ws.waitswitch()
	end
]]

function callBack(taskIndex,...)
	print("Now in callback")
	t = {...}
	for k,v in pairs(t) do
		print(k,v)
	end
	-- Send back a value
	if t[1] == 'require' and t[2] == "waitswitch" then
		print("RunCode result",ls.runCode(taskIndex,[[
		print("I am in the task now") 
		print("Dummy Require=",req) 
		lstep.require={
			waitswitch=function() 
				print("I am in waitswitch for task ]]..tostring(taskIndex)..[[")
				local oldstatus = lstep.getswitchstatus
				while lstep.getswitchstatus*oldstatus ~= -1 do
				end
				print("Switch happenned! And now back in task ]]..tostring(taskIndex)..[[")
			end
		}
		]]))
		return "DONE"
	end	
	return nil,"No functionality defined for this case"
end

print("Register call back", ls.registerCallBack(callBack))

print("AddTask",ls.addTask(script1))
print("AddTask",ls.addTask(script2))

for i = 1,40 do
	x,y=ls.runLoop()
--	print("Task 0 return status",x[0][1],x[0][2])
end
