lStepper = require("lStepper")
print("lStepper version is:",lStepper._VERSION)


--script1 = "for i=1,10 do print('Script 1 i:',i) end"
script1 = [[
function D()
	while true do
		for i=1,3 do
			print('D')
		end
		print(coroutine.yield(4,5,6))
		--print("Resuming D again")
	end
end

function co()
	local d = coroutine.create(D)
	--print("I am resuming in B")
	while true do 
		for i=1,10 do
			print('B') 
			if i==5 then
				print(coroutine.resume(d,1,2,3))
			end
		end 
		--print("Now yielding B in script")
		coroutine.yield()
		--print("Resuming B again")
	end
end

c = coroutine.create(co)
print("\nCoroutine created") 

coroutine.resume(c)

while true do 
	for i=1,2 do  
		print('A') 
	end
	coroutine.resume(c) 
end]]
--script1 = "print('I am done')"
print(lStepper.addTask(script1,"print('Initialization step 1') print('Initialization step 2')"))

--script2 = "for i=1,20 do print('Script 2 i:',i) end"
script2 = "while true do print('C') end"
--script2 = "print('I am done')"
print(lStepper.addTask(script2,"print('Initialization step x') print('Initialization step y')"))

script3 = [[
			test1='Hello' 
			test2=23 
			test3=34.45 
			test10 = {Hello=test1}
			while(true) do 
				print('script3') 
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
print(lStepper.addTask(script3,"print('Adding Script 3')",5,true))

print(lStepper.getNumOfTasks())
print("Start stepper loop")
--while(true) do
for i=1,50 do
	x,y=lStepper.runLoop()
	--print(x[0][1],x[0][2],x[1][1],x[1][2])
	if not x then
		print(y)
		break
	end
	if((x[0][1] == "FIN" or x[0][1]:sub(1,2) == "NE") and (x[1][1] == "FIN" or x[1][1]:sub(1,2) == "NE")) then
		print("Both scripts ended")
		break
	end
	if i==5 then
		print("i=5",lStepper.taskStatus(0))
		print("i=5",lStepper.taskStatus(1))
	end
	if i==30 then
		print("i=30",lStepper.taskStatus(0))
		print("i=30",lStepper.taskStatus(1))
	end
	if i==20 then
		print("C suspended")
		lStepper.suspendTask(1)
	end
	if i==40 then
		print("C resumed")
		lStepper.resumeTask(1)
	end
	if i==5 then
		x,y = lStepper.getTaskData(4,"test3")
		if x then
			print("Value from script3 test3=",x)
			print(lStepper.setTaskData(4,{test4=x}))
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
