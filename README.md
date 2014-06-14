LuaStepper
==========

C module to allow a Lua state to run multiple Lua scripts in parallel. This is platform independent and will work wherever Lua works.

## Description
LuaStepper is a module to help lua programs control simultaneous execution of multiple Lua threads without them needing to be coroutines.

## Thread creation details
* Each thread environment contains all standard lua libraries supplied with Lua except debug, package, io and os libraries. These libraries are excluded since they can make the thread get stuck so alternatives are provided when needed
* The environment has a table called _LS, which has the following functions and objects:
  * tableToString: ''FUNCTION'' : Function to convert a table to a string. Keys can be a number, string or table, values can be number, string table or boolean. Recursive tables are handled.
* Each thread runs in its separate lua state

## C API
The API provided by the C module is accessed by requiring 'LuaStepper'. Although this can be used directly the Lua interface ('lStepper') is the preferred way of accessing the LuaStepper module.
The API functions are as follows:
* __addTask__ - This adds a task to the task list. All the tasks in the task list are stepped through with the indicated number of steps when the runLoop function is executed. The first argument is the string containing the Lua script that is the task and the optional second argument is a lua script as a string which is executed without stepping before the actual code starts running in the environment. The optional third argument is  the number of steps to step the script each time runLoop executes a loop. The optional 4th argument is a boolean indicating whether this task has to be placed in the urgent quota. If successful it returns the task index in the list otherwise it returns nil with a error message. Urgent tasks are placed with task Indices beginning after the main task indices. This facility is provided to have an extra buffer to add urgent tasks without removing the normal tasks. Recommended way to use them would be add a urgent task. Suspend all the main tasks and then run the urgent task to get something done and then clean the urgent task slot and then resume the main tasks again. 
* __runLoop__ - This runs each task in the task List with their respective indicated number of steps. The optional argument to the function is an integer which specifies the number of times the loop has to run, by default it is run just once. Returns nil if there are no tasks otherwise returns a table with keys as the task index and the values are tables with 2 elements. The status of the task:
  * __GOOD__ - Task is running good
  * __FIN__ - Task Finished
  * __ERROR__ - Task produced an error and stopped. The second element is the error message
  * __NE-PREVERR__ - Task was not executed because it had a previous error. Second element is the Error  code type i.e."LUA_ERRRUN" or "LUA_ERRMEM"
  * __NE-FIN__ - Task was not executed because it finished already
  * __NE-SUSPEND__ - Task was not executed because it was suspended before
In case of multiple loop runs the status is what was generated in the last iteration.
* __suspendTask__ - Suspends a task's execution till resumed by called resumeTask. It takes the task index and returns true if suspend was successful otherwise nil if the task did not exist or had already finished or suspended.
* __resumeTask__ - Resumes a suspended task's execution. It takes the task index and returns true if resume was successful otherwise nil if the task did not exist or was not suspended or was finished.
* __taskStatus__ - This function takes the task index and returns the task status (at most 2 arguments) if the task is present otherwise returns nil and a message
  * __GOOD__ - Task is running good
  * __SUSPENDED__ - Task is in suspended state and is not being executed when runLoop is called
  * __NE-PREVERR__ - Task exited with an error when it was last executed. Second argument is the Error  code type i.e."LUA_ERRRUN" or "LUA_ERRMEM"
  * __NE-FIN__ - Task finished already
* __setTaskData__ - This function takes the task Index and a table containing key value pairs that have to be set in the task's Lua environment. It would overwrite same keys all values in the Global environment of the task. Returns true if successful otherwise nil and error message. The keys can be number or string while the values can be string, number, boolean.
* __setTaskTable__ - This function takes the task Index a table name (number or string) and a table generation script as a string (generated from tableToString function). Returns nil and a message in case of error or true if success.
* __getTaskData__ - This function takes the task Index and gets the value of a key from the global environment of the task. If successful it returns the value. If the value was a table it returns it in string form converted using tableToString function and a second argument = "__TABLE__". Otherwise it returns nil and a message.
* __getNumOfTasks__ - This function returns the number of tasks in the task List as the 1st return and number of urgent tasks in the list as the 2nd return
* __closeTask__ - This clears the task index of the task and makes it empty. Returns true if successful otherwise nil if a task was not there at the index.
* __version__ - This function returns the version number of the module.
* __registerCallBack__ - to register a callback function with LuaStepper. This function is accessible to the thread special functions.For now only require uses it. So when the thread code calls require it calls this call back function to tell the parent thread of the require request. It is called by giving a function as a argument. It returns true for success else nil and a error message. The call to the call back function is recommended to have the function name (string) as the 1st argument followed by the arguments passed to the function (only string, boolean and numbers allowed) when it is called from the thread. The call back function that is registered gets its 1st argument as the task Index which called it followed by the arguments that the call provided. The callback function is allowed only 2 return values both may be strings or the 1st can be nil.
* __runCode__ - Allows the parent thread to run some code in the thread environment without doing the stepping. This is useful to provide resources as requested by the thread in the thread environment. The function expects the 1st argument to be the task Index and the second argument a string of lua code to run in the task. Returns true if success. The environment in which the code runs is the global environment of the thread plus it has access to the following extra functions/tables which the thread cannot access:
  * package table - renamed as pack
  * require function - renamed as req
  * lstep table - table containing special functions used by the thread management functions. 

## The lstep table
1. The lstep table is only accessible via runCode or with functions provided to the app by runCode. 
2. The require function uses it to expect a return value put in lstep.require by the call back function (using runCode or otherwise)
3. The lstep table has variable called ''getswitchstatus'' which toggles between -1 and 1 for every switch that LuaStepper does away from the task. This can be used by the thread management functions when a switch has happenned


## Lua interface API
The API provided by the lua interface is accessed by requiring 'lStepper'. This is the recommended way to access the LuaStepper module. The only difference from the C API is as follows:
* __version__ - Here this is a string value containing the version number. So a function need not be called to get the version number.
* __setTaskTable__ is not there since setTaskData handles tables as well
* __setTaskData__ - This function takes the task Index and a table containing key value pairs that have to be set in the task's Lua environment. It would overwrite same keys all values in the Global environment of the task. Returns true if successful otherwise nil and error message. The keys can be number or string while the values can be string, number, boolean or table.
* __getTaskData__ - This function takes the task Index and gets the value of a key from the global environment of the task. If successful it returns the value. Otherwise it returns nil and a message.


