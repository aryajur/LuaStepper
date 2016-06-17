lStepper = require("lStepper")
print("lStepper version is:",lStepper._VERSION)

-- Define script 1
script1 = [[
while true do 
	print('A') 
end]]
-- Add script 1 to the queue with some print statements that are executed immediately
print("Add script 1. ID is -->",lStepper.addTask(script1,"print('Initialization step 1') print('Initialization step 2')"))

-- Define script 2
script2 = "while true do print('B') end"
-- Add script 2 to the queue with some print statements that are executed immediately
print("Add script 2. ID is -->",lStepper.addTask(script2,"print('Initialization step x') print('Initialization step y')"))

-- Define script 3
script3 = [[
			test1='Hello' 
			test2=23 
			test3=34.45 
			test10 = {Hello=test1}
			while(true) do 
				print('script3 test5=',test5) 
				if test4 and test4==test3 then
					break 
				end 
			end 
			while(true) do
				print('script3 next loop') 
				if test5 and type(test5)=='table' and test5.test6 then 
					print('test6=',test5.test6) 
					break 
				end 
			end]]
-- Add script 3 to the queue with a print statement initialization and a variable setting that is then printed in the script
print("Add script 3. ID is -->",lStepper.addTask(script3,"print('Adding Script 3') test5='script3'",5,true))	-- 5 means the number os codes to execute for each step of this script
																						-- This script is also placed in the urgent task pile by setting true
print("Number of tasks=",lStepper.getNumOfTasks())	-- displays 2   1   -- 2 regular 1 urgent
print("Start stepper loop")
for i=1,50 do
	x,y=lStepper.runLoop()	-- Run all the tasks for 1 step
	if not x then
		print(y)
		break
	end
	if((x[0][1] == "FIN" or x[0][1]:sub(1,2) == "NE") and (x[1][1] == "FIN" or x[1][1]:sub(1,2) == "NE")) then
		print("Both scripts ended")
		break
	end
	if i==5 then
		print("i=5, script 1 status is ",lStepper.taskStatus(0))
		print("i=5, script 2 status is ",lStepper.taskStatus(1))
	end
	if i==30 then
		print("i=30, script 1 status is ",lStepper.taskStatus(0))
		print("i=30, script 2 status is ",lStepper.taskStatus(1))
	end
	if i==20 then
		print("B suspended")
		lStepper.suspendTask(1)
	end
	if i==40 then
		print("B resumed")
		lStepper.resumeTask(1)
	end
	if i==5 then
		x,y = lStepper.getTaskData(4,"test3")	-- Get the value of test3 from script 3
		if x then
			print("Value from script3 test3=",x)
			print(lStepper.setTaskData(4,{test4=x}))	-- Set value of test4 to be same as test3 in script 3
		else
			print("Error getting data:",y)
		end
	end
	if i==10 then
		local t = {
			test6='Added Table!'
		}
		print(lStepper.setTaskData(4,{test5=t}))
		x=lStepper.getTaskData(4,"test10")
		print(x)
		if type(x) == 'table' then
			print("test10 in script 3 is a table:")
			for k,v in pairs(x) do
				print(k,v)
			end
		end
	end
end

print("END",lStepper.taskStatus(0))
print("END",lStepper.taskStatus(1))
print("END",lStepper.taskStatus(4))
lStepper.closeTask(0)
lStepper.closeTask(1)
lStepper.closeTask(4)
