ls = require("lStepper")
print("LuaStepper version is:",ls._VERSION)

testReqErr = nil	-- set this to true to test error generated by require when the call back function is not defined

script1 = [[
	print("SCRIPT: Require Function=",require) 
	print("SCRIPT: io library = ",io)
	print("SCRIPT: package table = ",package)
	print("SCRIPT: os library = ",os)	
	while true do
		if test1 then
			if not newMod then
				print("Call require")
				newMod = require('ThisFunc')
				print("NewMod:",newMod)
			else
				print("Call ModFunc",newMod.modFunc("paramone"))
			end
			print("param1=",param1)
			print("test1",test1)
		else
			print('A')
		end
	end
]]

print("AddTask",ls.addTask(script1))

for i = 1,5 do
	ls.runLoop()
end

function modFunc(taskIndex,param1)
	print("set param1 from modFunc in parent",ls.setTaskData(taskIndex,{param1=param1}))
end

function callBack(taskIndex,...)
	print("Now in callback")
	t = {...}
	for k,v in pairs(t) do
		print(k,v)
	end
	-- Send back a value
	if t[1] == 'require' and t[2] == "ThisFunc" then
		print("RunCode result",ls.runCode(taskIndex,[[
		print("I am in the task now executing runCode script") 
		print("Require Function=",require) 
		print("io library = ",io)
		print("package table = ",package)
		print("os library = ",os)
		lstep.require={
			modFunc=function(param1) 
				print("modFunc: I am in modFunc")
				print("modFunc: Param1=",param1) 
				print("modFunc: Require Function=",require) 
				print("modFunc: io library = ",io)
				print("modFunc: package table = ",package)
				print("os library = ",os)
				if type(param1) ~= "number" and type(param1) ~= "string" and type(param1)~= "boolean" then 
					return nil,"Invalid Argument"
				end
				local ret,msg = lstep.callParentFunc('modFunc',param1) 
				if not ret then
					return nil,msg
				else
					return true
				end
			end
		}
		test1="value of test1"]]))
		return "DONE"
	elseif t[1] == "modFunc" then
		modFunc(taskIndex,t[2])
		return "DONE"
	end	
	return nil,"No functionality defined for this case"
end

print("SetTaskData for test1",ls.setTaskData(0,{test1=true}))

if not testReqErr then
	print("Register call back", ls.registerCallBack(callBack))
end

for i = 1,40 do
	x,y=ls.runLoop()
	print("Task 0 return status",x[0][1],x[0][2])
	-- Define the call back function
	if i==10 and testReqError then
		print("Register call back", ls.registerCallBack(callBack))
	end
end
