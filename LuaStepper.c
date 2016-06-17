/******************************************************************************/
// Steps to Include Lua:
// 1. Make the include directory of the include files supplied in Lua or copy them
//    to the project and add them to the project. The files are:
//    a. lauxlib.h
//    b. lua.h
//    c. luaconf.h
//    d. lualib.h
//    e. lua.hpp - this is for C++ just include this and all files above are included with the
//       extern C directive for C++ programs
// 2. Set the library directory to the C libraries supplied by Lua eg. where the file Lua5.1.lib
//    is placed
// 3. In the Linker Input options add Lua5.1.lib in the additional dependancies

#if defined __WIN32__ || defined WIN32
# include <windows.h>
# define _EXPORT __declspec(dllexport)
#else
# define _EXPORT
#endif

//#include <stdio.h>
//#include <string.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define numTasks	4
#define urgentTasks 2
#define LS_DEFSTEPS 10

#define VERSION     "1.16.06.17"

struct Task
{
    // Cannot store strings from Lua in C without dynamic memory allocation
//	const char *iniCodeString;	// Code to be run in the Virtual Machine initially
//	const char *codeString;		// Code to be stepped through
	int numSteps;			// number os steps to be run each time
	lua_State *luaVM;			// Lua Virtual machine where the code will be executed
	lua_State *thread;			// Lua thread which executes the code in the Lua Virtual machine
	short finished;			// Flag to indicate if the thread has finished execution
    short suspended;        // Flag to indicate if the thread is suspended and not to be stepped
};


lua_State *parentState;     // Used to access the parent callback function from the parentState registry

struct Task taskq[numTasks+urgentTasks];

// hook function to yield a thread
int hookFunc(lua_State *L, lua_Debug *dbg)
{
    // Get the __ls_ table to check if the hook yield needs to be bypassed
	lua_pushlightuserdata(L,hookFunc); // pointer to hookFunc is the key in the registry to the table __ls_
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_getfield(L,-1,"hookBypass");
	if(lua_isnil(L,-1))
	{
		lua_pop(L,2);   // Leave the stack balanced
        //printf("hookFunc\n");
		return lua_yield(L,0);
	}
	else
	{
		lua_pop(L,2);   // Leave the stack balanced
		//printf("hookFunc yield bypass\n");
		return 0;
	}

}	// int hookFunc(lua_State *L, lua_Debug *dbg) ends

static int callParentFunc(lua_State *L)
{
    short i;
    if(!parentState)
    {
		lua_pushnil(L);
		lua_pushstring(L,"No Callback function defined by parent.");
		return 2;	// 2 results pushed on the stack
    }
    int numArgs = lua_gettop(L);
	if(numArgs < 1)
	{
		// Return message saying that function that called name is needed
		lua_pushnil(L);
		lua_pushstring(L,"Need at least caller function name.");
		return 2;	// 2 results pushed on the stack
	}
	if (!lua_isstring(L,1))
    {
		lua_pushnil(L);
		lua_pushstring(L,"1st argument should be a string containing name of the caller function.");
		return 2;	// 2 results pushed on the stack
    }
    // rest of the arguments can either be string number or boolean
    for(i=2;i<=numArgs;i++)
    {
        if(!lua_isstring(L,i) && !lua_isnumber(L,i) && !lua_isboolean(L,i))
        {
            lua_pushnil(L);
            lua_pushstring(L,"Only String, number or boolean arguments allowed.");
            return 2;	// 2 results pushed on the stack
        }
    }
    // Now call the parent callback function
    lua_pushlightuserdata(parentState,callParentFunc); // pointer to callParentFunc is the key in the registry to the call back function
    lua_gettable(parentState,LUA_REGISTRYINDEX);    // Get the call back function
    // Push the task index which is calling this parent functions
    for(i=0;i<numTasks+urgentTasks;i++)
    {
        if (taskq[i].thread == L)
        {
            lua_pushnumber(parentState,i);
            break;
        }
    }
    // Now push all the arguments
    for(i=1;i<=numArgs;i++)
    {
        if(lua_isstring(L,i))
        {
            lua_pushstring(parentState,lua_tostring(L,i));
        }
        else if(lua_isnumber(L,i))
        {
            lua_pushnumber(parentState,lua_tonumber(L,i));
        }
        else // if(lua_isboolean(L,i))
        {
            lua_pushboolean(parentState,lua_toboolean(L,i));
        }
    }
    // Now call the function
    switch(lua_pcall(parentState,numArgs+1,2,0))
    {
        case LUA_OK:
            // Pop the string from the stack
            if(lua_isnil(parentState,-2))
            {
                lua_pushnil(L);
                lua_pushstring(L,lua_tostring(parentState,-1));
                return 2;
            }
            lua_pushstring(L,lua_tostring(parentState,-2));
			if(!lua_isnil(parentState,-1))
			{
				lua_pushstring(L,lua_tostring(parentState,-1));
				return 2;
			}
			return 1;
            break;
        case LUA_ERRRUN:
        case LUA_ERRMEM:
        case LUA_ERRGCMM:
            // Pop the error message from the stack
            lua_pushnil(L);
            lua_pushstring(L,lua_tostring(parentState,-1));
            return 2;
            break;
    }
    return 0;
}

/*
// To reset the task at the given index
static int resetTask(lua_State *L)
{
   	short checkIndex;
   	const char *iniCode, *code;
   	int numSteps;
	int numArgs = lua_gettop(L);
	if(!numArgs)
	{
		// Return message saying need at least the script needed to add as Task
		lua_pushnil(L);
		lua_pushstring(L,"Need the task index which has to be reset");
		return 2;	// 2 results pushed on the stack
	}
	checkIndex = luaL_checkinteger(L,1);
	if(taskq[checkIndex].luaVM)
	{
	    lua_Debug ar;
	    lua_pushthread(taskq[checkIndex].thread);
	    lua_getinfo(taskq[checkIndex].luaVM,">S",&ar);
	    printf(ar.source);

	    iniCode = taskq[checkIndex].iniCodeString;
	    code = taskq[checkIndex].codeString;
	    numSteps = taskq[checkIndex].numSteps;

        lua_close( taskq[checkIndex].luaVM );
        //printf("LuaVM closed");
        taskq[checkIndex].codeString = NULL;
        taskq[checkIndex].iniCodeString = NULL;
        taskq[checkIndex].luaVM = NULL;
        taskq[checkIndex].thread = NULL;
        taskq[checkIndex].numSteps = 0;
        taskq[checkIndex].finished = 0;
        taskq[checkIndex].suspended = 0;
	    // Push these in the lua stack and call add task
	    numArgs = lua_gettop(L);
	    lua_pop(L,numArgs);     // Empty the stack

	    lua_pushstring(L,code);
	    lua_pushinteger(L,numSteps);
	    if(iniCode)
        {
            lua_pushstring(L,iniCode);
        }
        numArgs = addTask(L);
	}       // if(taskq[checkIndex].luaVM) ends
    return 0;
}      // static int resetTask(lua_State *L) ends
*/
static int addTask(lua_State *L)
{
	//printf("addTask\n");
	short i, flag, numSteps,urgent=0;
	const char *code,*iniCode, *err, *msg;
	int numArgs = lua_gettop(L);
	iniCode = NULL;
	if(numArgs<1)
	{
		// Return message saying need at least the script needed to add as Task
		lua_pushnil(L);
		lua_pushstring(L,"Need at least the script as a string as the 1st argument to call addTask");
		return 2;	// 2 results pushed on the stack
	}
    if(!lua_isstring(L,1))
    {
        lua_pushnil(L);
        lua_pushstring(L,"1st argument should be a string of the script code to add as a task.");
        return 2;	// 2 results pushed on the stack
    }
    code = lua_tostring(L,1);
	if(numArgs==1)
	{
		numSteps = LS_DEFSTEPS;
	}
	else
	{
        if(!lua_isstring(L,2))
        {
            lua_pushnil(L);
            lua_pushstring(L,"2nd argument should be a string of the script code to execute as initialization.");
            return 2;	// 2 results pushed on the stack
        }
        iniCode = lua_tostring(L,2);
		if(numArgs>2)
		{
            if(!lua_isnumber(L,3))
            {
                lua_pushnil(L);
                lua_pushstring(L,"3rd argument should be a number for the number of string steps to be executed every time runLoop is executed.");
                return 2;	// 2 results pushed on the stack
            }
            numSteps = lua_tointeger(L,3);
            if(numSteps<2)
                numSteps = 2;
            if(numArgs>3)
            {
                if(!lua_isboolean(L,4))
                {
                    lua_pushnil(L);
                    lua_pushstring(L,"4th argument should be boolean indicating whether this task is an urgent task.");
                    return 2;	// 2 results pushed on the stack
                }
                urgent = lua_toboolean(L,4);
            }
		}
		else
            numSteps = LS_DEFSTEPS;
	}
	//printf("Arguments read");
	// Find an empty task slot
	flag = 0;
	//printf("Urgent %d\n",urgent);
    if(urgent)
    {
        for(i=numTasks;i<numTasks+urgentTasks;i++)
        {
            if(!taskq[i].luaVM)
            {
                flag = 1;
                break;
            }
        }	// for i ends here
    }
    else
    {
        for(i=0;i<numTasks;i++)
        {
            if(!taskq[i].luaVM)
            {
                flag = 1;
                break;
            }
        }	// for i ends here
    }
	if(!flag)
	{
		// Return message saying que is full
		lua_pushnil(L);
		lua_pushstring(L,"Task List is full. Please empty a task index using closeTask");
		return 2;	// 2 results pushed on the stack
	}
	// add the task to the list here
	taskq[i].luaVM = luaL_newstate();
	//printf("New Lua state created");

	if (NULL == taskq[i].luaVM)
	{
		lua_pushnil(L);
		lua_pushstring(L,"Cannot initialize Lua Virtual Machine");
		return 2;	// 2 results pushed on the stack
	}


	//luaL_openlibs(taskq[i].luaVM);
	// Include the base, coroutine, table, string, bit and math libraries
	// Skipping the package, io, os and debug libraries

    luaL_requiref(taskq[i].luaVM, "_G", luaopen_base, 1);                        // Base Library
    luaL_requiref(taskq[i].luaVM, LUA_COLIBNAME, luaopen_coroutine, 1);          // Coroutine library
    luaL_requiref(taskq[i].luaVM, LUA_TABLIBNAME, luaopen_table, 1);             // Table library
    luaL_requiref(taskq[i].luaVM, LUA_IOLIBNAME, luaopen_io, 1);                 // io library
    luaL_requiref(taskq[i].luaVM, LUA_OSLIBNAME, luaopen_os, 1);                 // os library
    luaL_requiref(taskq[i].luaVM, LUA_STRLIBNAME, luaopen_string, 1);            // String library
    luaL_requiref(taskq[i].luaVM, LUA_BITLIBNAME, luaopen_bit32, 1);             // Bit library
    luaL_requiref(taskq[i].luaVM, LUA_MATHLIBNAME, luaopen_math, 1);             // Math library
    luaL_requiref(taskq[i].luaVM, LUA_LOADLIBNAME, luaopen_package, 1);          // Package library
    lua_pop(taskq[i].luaVM, 6);  // remove libraries from the stack
//
	static const struct luaL_Reg threadFuncs[] = {
			//{"create",coroutineCreate},
			//{"resume",coroutineResume},
			//{"yield",coroutineYield},
			{"callParentFunc",callParentFunc},
			{NULL,NULL}
	};
	luaL_newlib(taskq[i].luaVM, threadFuncs); // Just returns the module as a table
	lua_setglobal(taskq[i].luaVM,"__ls_");    // Put the table in the global space
	// Now put the table in the registry as well for access by LuaStepper
	lua_pushlightuserdata(taskq[i].luaVM,hookFunc); // pointer to hookFunc is the key in the registry to the table __ls_
	lua_getglobal(taskq[i].luaVM,"__ls_");     // Get the table back on the top of the stack to put it in the registry
	lua_settable(taskq[i].luaVM,LUA_REGISTRYINDEX);

    // Initialize the value of __ls_.getswitchstatus as 1
    lua_pushlightuserdata(taskq[i].luaVM,hookFunc); // pointer to hookFunc is the key in the registry to the table __ls_
    lua_gettable(taskq[i].luaVM,LUA_REGISTRYINDEX);

    // Now put the variable name on the stack
    lua_pushstring(taskq[i].luaVM,"getswitchstatus");
    lua_pushnumber(taskq[i].luaVM,1);
    lua_settable(taskq[i].luaVM,-3);
    // Now pop the table to balance the stack
    lua_pop(taskq[i].luaVM,1);


	taskq[i].thread = lua_newthread(taskq[i].luaVM);

	//taskq[i].codeString = code;
	//taskq[i].iniCodeString = iniCode;
	taskq[i].numSteps = numSteps;
	taskq[i].finished = 0;
	taskq[i].suspended = 0;

	//printf(code);
	//printf(iniCode);
	//printf("%d",numSteps);

	// Execute tabletostring function addition and this code first of all:

    msg =   "_LS={} "
            "function _LS.tableToString(t) "
			"	if type(t) ~= 'table' then return nil, 'Expected table parameter' end "
			"	local rL = {cL = 1} "
			"	rL[rL.cL] = {} "
			"	local tabIndex = {} "
			"	local latestTab = 0 "
			"	local result = {} "
			"	do "
			"		rL[rL.cL]._f,rL[rL.cL]._s,rL[rL.cL]._var = pairs(t) "
			"		result[#result + 1] = 't0={}' "
			"		rL[rL.cL].t = t "
			"		rL[rL.cL].tabIndex = 0 "
			"		tabIndex[t] = rL[rL.cL].tabIndex "
			"		while true do "
			"			local key "
			"			local k,v = rL[rL.cL]._f(rL[rL.cL]._s,rL[rL.cL]._var) "
			"			rL[rL.cL]._var = k "
			"			if not k and rL.cL == 1 then "
			"				break "
			"			elseif not k then "
			"				rL.cL = rL.cL - 1 "
			"				if rL[rL.cL].vNotDone then "
			"					key = 't'..rL[rL.cL].tabIndex..'[t'..tostring(rL[rL.cL+1].tabIndex)..']' "
			"					result[#result + 1] = '\\n'..key..'=' "
			"					v = rL[rL.cL].vNotDone "
			"				end "
			"				rL[rL.cL+1] = nil "
			"			else "
			"				if type(k) == 'number' then "
			"					key = 't'..rL[rL.cL].tabIndex..'['..tostring(k)..']' "
			"					result[#result + 1] = '\\n'..key..'=' "
			"				elseif type(k) == 'string' then "
			"					key = 't'..rL[rL.cL].tabIndex..'.'..tostring(k) "
			"					result[#result + 1] = '\\n'..key..'=' "
			"				else "
			"					if tabIndex[k] then "
			"						key = 't'..rL[rL.cL].tabIndex..'[t'..tabIndex[k]..']' "
			"						result[#result + 1] = '\\n'..key..'=' "
			"					else "
			"						latestTab = latestTab + 1 "
			"						result[#result + 1] = '\\nt'..tostring(latestTab)..'={}' "
			"						rL[rL.cL].vNotDone = v "
			"						rL.cL = rL.cL + 1 "
			"						rL[rL.cL] = {} "
			"						rL[rL.cL]._f,rL[rL.cL]._s,rL[rL.cL]._var = pairs(k) "
			"						rL[rL.cL].tabIndex = latestTab "
			"						rL[rL.cL].t = k "
			"						tabIndex[k] = rL[rL.cL].tabIndex "
			"					end "
			"				end "
			"			end "
			"			if key then "
			"				rL[rL.cL].vNotDone = nil "
			"				if type(v) == 'table' then "
			"					if tabIndex[v] then "
			"						result[#result + 1] = 't'..tabIndex[v] "
			"					else "
			"						latestTab = latestTab + 1 "
			"						result[#result + 1] = '{}\nt'..tostring(latestTab)..'='..key "
			"						rL.cL = rL.cL + 1 "
			"						rL[rL.cL] = {} "
			"						rL[rL.cL]._f,rL[rL.cL]._s,rL[rL.cL]._var = pairs(v) "
			"						rL[rL.cL].tabIndex = latestTab "
			"						rL[rL.cL].t = v "
			"						tabIndex[v] = rL[rL.cL].tabIndex "
			"					end "
			"				elseif type(v) == 'number' then "
			"					result[#result + 1] = tostring(v) "
			"				elseif type(v) == 'boolean' then "
			"					result[#result + 1] = tostring(v) "
			"				else "
			"					result[#result + 1] = string.format('%q',tostring(v)) "
			"				end "
			"			end "
			"		end "
			"	end "
			"	return table.concat(result) "
			"end "
            "do"
            "   local pack = package "
            "   local req = require "
            "   local oslib = os "
            "   local iolib = io "
            "	local coresume = coroutine.resume"
            "	local coyield = coroutine.yield"
            "	local yieldToRes"
            "  	local resToYield"
            "	local lstep = __ls_"
            "	local function modresume(...)"
            "		resToYield = table.pack(...)"
            "		local cor = resToYield[1]"
            "		table.remove(resToYield,1)"
            "		coresume(cor)"
            "		while not lstep.hookBypass do"
            "			coresume(cor)"
            "		end"
            "		lstep.hookBypass = nil"
            "		local retValues = yieldToRes"
            "		yieldToRes = nil"
            "		return table.unpack(retValues)"
            "	end"
            "	local function modyield(...)"
            "		yieldToRes = table.pack(...)"
            "		lstep.hookBypass = true"
            "		coyield()"
            "		local retValues = resToYield"
            "		resToYield = nil"
            "		return table.unpack(retValues)"
            "	end"
            "   lstep.setTable = function(n,s) "
            "       local sf = {} "
            "       local f,msg = load(s,nil,nil,sf) "
            "       if not f then "
            "           return nil,msg "
            "       end "
            "       f() "
            "       if not sf.t0 or type(sf.t0) ~= 'table' then "
            "           return nil, 'Not a valid table generation code.' "
            "       end "
            "       _G[n]=sf.t0 "
            "       return true "
            "   end "
            "   lstep.runCode = function(s) "
            "       if type(s) ~= 'string' then "
            "           return nil,'String expected' "
            "       end "
            "       local sf = {package=pack,require=req,os=oslib,io=iolib,lstep=lstep} "
            "       local sfm = {__index = _ENV,__newindex = function(t,k,v) _ENV[k]=v end} "
            "       setmetatable(sf,sfm) "
            "       local f,msg = load(s,nil,nil,sf) "
            "       if not f then "
            "           return nil,msg "
            "       end "
            "       local stat,msg = pcall(f) "
            "       if not stat then "
            "           return nil,msg "
            "       end "
            "       return true "
            "   end "
            "   lstep.getTaskData = function(k) "
            "       if type(_G[k]) == 'number' or type(_G[k]) == 'string' or type(_G[k]) == 'boolean' or type(_G[k]) == 'nil' then "
            "           return _G[k] "
            "       end "
            "       if type(_G[k]) == 'table' then "
            "           return _LS.tableToString(_G[k]),'TABLE' "
            "       end "
            "       return nil,'Cannot return function,userdata,thread datatype' "
            "   end "
            "   require = function(modName) "
            "       mod = tostring(modName) "
            "       lstep.require=nil "
            "       local ret,msg = lstep.callParentFunc('require',modName) "
            "       if ret then "
            "           return lstep.require "
			"		else "
			"			error(msg,2) "
            "       end "
            "   end "
            "	coroutine.resume = modresume"
            "	coroutine.yield = modyield"
            "	__ls_ = nil "
            "   package = nil "
            "   io = nil "
            "   os = nil "
            "end ";
//printf("\n\n\n\n\n\n%s\n\nTHIS IS MSG\n\n\n\n",msg);
    // run the LuaStepper initialization code
    //printf("load luastepper initialization code:\n%s\n",msg);
    switch(luaL_loadstring(taskq[i].thread,msg))
    {
        case LUA_OK:
            // No error so run the string
            //printf("Now running initialization code\n");
            switch(lua_pcall(taskq[i].thread,0,0,0))
            {
                case LUA_OK:
                    // Successfully executed
                    break;
                case LUA_ERRRUN:
                    // Run time error
                case LUA_ERRMEM:
                    // Memory Allocation error
                case LUA_ERRGCMM:
                    // Error Running Garbage collector
                    // Pop the error message
                    err = lua_tostring(taskq[i].thread,-1);

                    // Clean the task slot
                    lua_close( taskq[i].luaVM );
                    //taskq[i].codeString = NULL;
                    //taskq[i].iniCodeString = NULL;
                    taskq[i].luaVM = NULL;
                    taskq[i].thread = NULL;
                    taskq[i].numSteps = 0;
                    taskq[i].finished = 0;
                    taskq[i].suspended = 0;

                    lua_pushnil(L);
                    lua_pushstring(L,err);
                    return 2;	// 2 results pushed on the stack
                    break;
            }		// switch(lua_pcall(L,0,0,0)) ends here
            break;
        case LUA_ERRSYNTAX:
            // Syntax error, remove the task and clear the slot
        case LUA_ERRMEM:
            // Memory allocation error
        case LUA_ERRGCMM:
            // Error Running Garbage collector
            // Pop the error message
            err = lua_tostring(taskq[i].thread,-1);
            //printf("Error message: %s",err);

            // Clean the task slot
            lua_close( taskq[i].luaVM );
            //printf("LuaVM closed");
            //taskq[i].codeString = NULL;
            //taskq[i].iniCodeString = NULL;
            taskq[i].luaVM = NULL;
            taskq[i].thread = NULL;
            taskq[i].numSteps = 0;
            taskq[i].finished = 0;
            taskq[i].suspended = 0;

            lua_pushnil(L);
            lua_pushstring(L,err);
            return 2;	// 2 results pushed on the stack
            break;
    }		// switch(luaL_loadstring(taskq[i].thread,msg)) ends here

	// Run the initialization code
	if(iniCode)
	{
		switch(luaL_loadstring(taskq[i].thread,iniCode))
		{
			case LUA_OK:
				// No error so run the string
				//printf("Now running initialization code\n");
				switch(lua_pcall(taskq[i].thread,0,0,0))
				{
					case LUA_OK:
						// Successfully executed
						break;
					case LUA_ERRRUN:
						// Run time error
					case LUA_ERRMEM:
                        // Memory Allocation error
                    case LUA_ERRGCMM:
                        // Error Running Garbage collector
						// Pop the error message
						err = lua_tostring(taskq[i].thread,-1);

						// Clean the task slot
						lua_close( taskq[i].luaVM );
						//taskq[i].codeString = NULL;
						//taskq[i].iniCodeString = NULL;
						taskq[i].luaVM = NULL;
						taskq[i].thread = NULL;
						taskq[i].numSteps = 0;
						taskq[i].finished = 0;
						taskq[i].suspended = 0;

						lua_pushnil(L);
						lua_pushstring(L,err);
						return 2;	// 2 results pushed on the stack
						break;
				}		// switch(lua_pcall(L,0,0,0)) ends here
				break;
			case LUA_ERRSYNTAX:
				// Syntax error, remove the task and clear the slot
			case LUA_ERRMEM:
				// Memory allocation error
            case LUA_ERRGCMM:
                // Error Running Garbage collector
				// Pop the error message
				err = lua_tostring(taskq[i].thread,-1);

				// Clean the task slot
				lua_close( taskq[i].luaVM );
				//taskq[i].codeString = NULL;
				//taskq[i].iniCodeString = NULL;
				taskq[i].luaVM = NULL;
				taskq[i].thread = NULL;
				taskq[i].numSteps = 0;
				taskq[i].finished = 0;
				taskq[i].suspended = 0;

				lua_pushnil(L);
				lua_pushstring(L,err);
				return 2;	// 2 results pushed on the stack
				break;
		}		// switch(luaL_loadstring(taskq[i].thread,iniCode)) ends here
	}		// if(iniCode) ends here
	switch(luaL_loadstring(taskq[i].thread,code))
	{
		case LUA_OK:
			// No error
			break;
		case LUA_ERRSYNTAX:
			// Syntax error, remove the task and clear the slot
		case LUA_ERRMEM:
			// Memory allocation error
			// Pop the error message
			err = lua_tostring(taskq[i].thread,-1);

			// Clean the task slot
			lua_close( taskq[i].luaVM );

			//taskq[i].codeString = NULL;
			//taskq[i].iniCodeString = NULL;
			taskq[i].luaVM = NULL;
			taskq[i].thread = NULL;
			taskq[i].numSteps = 0;
			taskq[i].finished = 0;
			taskq[i].suspended = 0;

			lua_pushnil(L);
			lua_pushstring(L,err);
			return 2;	// 2 results pushed on the stack
			break;
	}
	lua_sethook(taskq[i].thread,(lua_Hook) hookFunc,LUA_MASKCOUNT,taskq[i].numSteps);

	lua_pushinteger(L,i);
	return 1;
}	// addTask function ends here

// Function to run some code in a task
static int runCode(lua_State *L)
{
	short tIndex;
    int numArgs = lua_gettop(L);
	if(numArgs < 2)
	{
		// Return message saying that task index and code to run are needed
		lua_pushnil(L);
		lua_pushstring(L,"Need a task index and a string of Lua code");
		return 2;	// 2 results pushed on the stack
	}
	if (!lua_isnumber(L,1))
    {
		lua_pushnil(L);
		lua_pushstring(L,"1st argument should be a number for the task Index");
		return 2;	// 2 results pushed on the stack
    }
	tIndex = lua_tonumber(L,1);
	if(!taskq[tIndex].luaVM)
    {
		lua_pushnil(L);
		lua_pushstring(L,"1st argument should be a valid TaskIndex. No task found at given index");
		return 2;	// 2 results pushed on the stack
    }
    if(!lua_isstring(L,2))
    {
		lua_pushnil(L);
		lua_pushstring(L,"2nd argument should be a string of Lua code.");
		return 2;	// 2 results pushed on the stack
    }

    // Call the function to run the code in the environment
    // The function is in the table in the registry
    // 1st get the table to the top of the stack

    lua_pushlightuserdata(taskq[tIndex].luaVM,hookFunc); // pointer to hookFunc is the key in the registry to the table __ls_
    lua_gettable(taskq[tIndex].luaVM,LUA_REGISTRYINDEX);

    // Now get the function from the table
    lua_getfield(taskq[tIndex].luaVM,-1,"runCode");

    // Now push the lua code string
    lua_pushstring(taskq[tIndex].luaVM,lua_tostring(L,2));

    // Call the function
    lua_call(taskq[tIndex].luaVM,1,2);

    // Now check the results if any error
    if(lua_isnil(taskq[tIndex].luaVM,-2))
    {
        // Error in the script
        lua_pushnil(L);
        lua_pushstring(L,lua_tostring(taskq[tIndex].luaVM,-1));
        return 2;
    }
    //lua_pop(taskq[tIndex].luaVM,3); // 2 results and the __ls_ table
    // Return true
    lua_pushboolean(L,1);
    return 1;
}

// This function allows the master to register a call back function
//   which the thread functions can call to request resources or do tasks
static int registerCallBack(lua_State *L)
{
    int numArgs = lua_gettop(L);
	if(numArgs < 1)
	{
		// Return message saying that a function is needed
		lua_pushnil(L);
		lua_pushstring(L,"Need a function to register for call back");
		return 2;	// 2 results pushed on the stack
	}
	if(!lua_isfunction(L,1))
    {
		lua_pushnil(L);
		lua_pushstring(L,"Need a function to register for call back");
		return 2;	// 2 results pushed on the stack
    }
    // Put the function in the registry
	lua_pushlightuserdata(L,callParentFunc); // pointer to callParentFunc is the key in the registry to the call back function
	lua_pushvalue(L,1);     // Get the callback function back on the top of the stack to put it in the registry
	lua_settable(L,LUA_REGISTRYINDEX);
    parentState = L;
    lua_pushboolean(L,1);
    return 1;
}

// Function to retrieve data from a task
static int getTaskData(lua_State *L)
{
	short tIndex;
    int numArgs = lua_gettop(L);
	if(numArgs < 2)
	{
		// Return message saying that a global key and task Index are needed
		lua_pushnil(L);
		lua_pushstring(L,"Need a task index, and global key");
		return 2;	// 2 results pushed on the stack
	}
	if (!lua_isnumber(L,1))
    {
		lua_pushnil(L);
		lua_pushstring(L,"1st argument should be a number for the task Index");
		return 2;	// 2 results pushed on the stack
    }
	tIndex = lua_tonumber(L,1);
	if(!taskq[tIndex].luaVM)
    {
		lua_pushnil(L);
		lua_pushstring(L,"1st argument should be a valid TaskIndex. No task found at given index");
		return 2;	// 2 results pushed on the stack
    }
    if(!lua_isnumber(L,2) && !lua_isstring(L,2))
    {
		lua_pushnil(L);
		lua_pushstring(L,"2nd argument should be a number or a string as as the key of the gobal table whose value to return.");
		return 2;	// 2 results pushed on the stack
    }
    //printf("Arguments are good");
    // Call the function to generate the table in the task
    // The function is in the table in the registry
    // 1st get the table to the top of the stack

    lua_pushlightuserdata(taskq[tIndex].luaVM,hookFunc); // pointer to hookFunc is the key in the registry to the table __ls_
    lua_gettable(taskq[tIndex].luaVM,LUA_REGISTRYINDEX);    // Get the __ls_ table
    //printf("Got the __ls_ table on top of stack");
    // Now get the function from the table
    lua_getfield(taskq[tIndex].luaVM,-1,"getTaskData");
    //printf("Got the function getTaskData");
    // Push the Key
    if(lua_isnumber(L,2))
         // Key is a number
        lua_pushnumber(taskq[tIndex].luaVM,lua_tonumber(L,2));
    else
        // Key is a string
        lua_pushstring(taskq[tIndex].luaVM,lua_tostring(L,2));

    //printf("Pushed the key and now call function");
    // Call the function
    lua_call(taskq[tIndex].luaVM,1,2);
    //printf("Now check the results");
    // Now check the results if any error
    if(lua_isnil(taskq[tIndex].luaVM,-2))
    {
        // Error in the script
        lua_pushnil(L);
        lua_pushstring(L,lua_tostring(taskq[tIndex].luaVM,-1));
        return 2;
    }

    if(lua_isstring(taskq[tIndex].luaVM,-1))
    {
        // Result is a table lua code
        lua_pushstring(L,lua_tostring(taskq[tIndex].luaVM,-2)); // Table script
        lua_pushstring(L,"TABLE");
        return 2;
    }

    // Result is a lua value
    if(!lua_isnumber(taskq[tIndex].luaVM,-2) && !lua_isstring(taskq[tIndex].luaVM,-2) && !lua_isboolean(taskq[tIndex].luaVM,-2) && !lua_isnil(taskq[tIndex].luaVM,-2))
    {
        lua_pushnil(L);
        lua_pushstring(L,"Value not a number, string, boolean, nil or table");
        return 2;
    }

    // Return the value
    if(lua_isnumber(taskq[tIndex].luaVM,-2))
        lua_pushnumber(L,lua_tonumber(taskq[tIndex].luaVM,-2));
    else if(lua_isstring(taskq[tIndex].luaVM,-2))
        lua_pushstring(L,lua_tostring(taskq[tIndex].luaVM,-2));
    else if(lua_isboolean(taskq[tIndex].luaVM,-2))
        lua_pushboolean(L,lua_toboolean(taskq[tIndex].luaVM,-2));
    else
        lua_pushnil(L);
    //printf("Now pop the stack to equalize");
    //lua_pop(taskq[tIndex].luaVM,3);   //2 return values and the __ls_ table
    return 1;
}

static int setTaskTable(lua_State *L)
{
	short tIndex;
    int numArgs = lua_gettop(L);
	if(numArgs < 3)
	{
		// Return message saying that a table and task Index are needed
		lua_pushnil(L);
		lua_pushstring(L,"Need a task index table name and the table structure as a string of Lua code");
		return 2;	// 2 results pushed on the stack
	}
	if (!lua_isnumber(L,1))
    {
		lua_pushnil(L);
		lua_pushstring(L,"1st argument should be a number for the task Index");
		return 2;	// 2 results pushed on the stack
    }
	tIndex = lua_tonumber(L,1);
	if(!taskq[tIndex].luaVM)
    {
		lua_pushnil(L);
		lua_pushstring(L,"1st argument should be a valid TaskIndex. No task found at given index");
		return 2;	// 2 results pushed on the stack
    }
    if(!lua_isnumber(L,2) && !lua_isstring(L,2))
    {
		lua_pushnil(L);
		lua_pushstring(L,"2nd argument should be a number or a string as name of the table in the global space");
		return 2;	// 2 results pushed on the stack
    }
    if(!lua_isstring(L,3))
    {
		lua_pushnil(L);
		lua_pushstring(L,"3rd argument should be a string of Lua code to generate the table.");
		return 2;	// 2 results pushed on the stack
    }

    // Call the function to generate the table in the task
    // The function is in the table in the registry
    // 1st get the table to the top of the stack

    lua_pushlightuserdata(taskq[tIndex].luaVM,hookFunc); // pointer to hookFunc is the key in the registry to the table __ls_
    lua_gettable(taskq[tIndex].luaVM,LUA_REGISTRYINDEX);

    // Now get the function from the table
    lua_getfield(taskq[tIndex].luaVM,-1,"setTable");

    // Push the table name
    if(lua_isnumber(L,2))
         // Key is a number
        lua_pushnumber(taskq[tIndex].luaVM,lua_tonumber(L,2));
    else
        // Key is a string
        lua_pushstring(taskq[tIndex].luaVM,lua_tostring(L,2));
    // Now push the value i.e. the lua code string
    // Value is a string
    lua_pushstring(taskq[tIndex].luaVM,lua_tostring(L,3));

    // Call the function
    lua_call(taskq[tIndex].luaVM,2,2);

    // Now check the results if any error
    if(lua_isnil(taskq[tIndex].luaVM,-2))
    {
        // Error in the script
        lua_pushnil(L);
        lua_pushstring(L,lua_tostring(taskq[tIndex].luaVM,-1));
        return 2;
    }
    //lua_pop(taskq[tIndex].luaVM,3); // 2 results and the __ls_ table
    // Return true
    lua_pushboolean(L,1);
    return 1;
}

static int setTaskData(lua_State *L)
{
	short tIndex;
    int numArgs = lua_gettop(L);
	if(numArgs < 2)
	{
		// Return message saying that a table and task Index are needed
		lua_pushnil(L);
		lua_pushstring(L,"Need a task Index and a table with key value pairs of the data to be passed");
		return 2;	// 2 results pushed on the stack
	}
	if (!lua_isnumber(L,1))
    {
		lua_pushnil(L);
		lua_pushstring(L,"1st argument should be a number for the task Index");
		return 2;	// 2 results pushed on the stack
    }
	tIndex = lua_tointeger(L,1);
	if(!taskq[tIndex].luaVM)
    {
		lua_pushnil(L);
		lua_pushstring(L,"1st argument should be a valid TaskIndex. No task found at given index");
		return 2;	// 2 results pushed on the stack
    }
    if(!lua_istable(L,2))
    {
		lua_pushnil(L);
		lua_pushstring(L,"2nd argument should be a table.");
		return 2;	// 2 results pushed on the stack
    }
    //printf("Arguments are good\n");
    // Now traverse the table
    // table is in the stack at index 2
    lua_pushnil(L);  // first key
    while (lua_next(L, 2) != 0)
    {
        // uses 'key' (at index -2) and 'value' (at index -1)

        // Key must be a number or a string
        // Value can be a number. string, or boolean
        //printf("Check Key and Value\n");
        if(!lua_isnumber(L,-2) && !lua_isstring(L,-2))
        {
            lua_pushnil(L);
            lua_pushstring(L,"Table key is not a number or a string");
            return 2;	// 2 results pushed on the stack
        }
        if(!lua_isnumber(L,-1) && !lua_isstring(L,-1) && !lua_isboolean(L,-1))
        {
            lua_pushnil(L);
            lua_pushstring(L,"Table value is not a number, string, or boolean");
            return 2;	// 2 results pushed on the stack
        }
        lua_pop(L, 1);
    }
    //printf("All Key and Values are good, now get global\n");

    // Get the global table on the stack
    lua_rawgeti(taskq[tIndex].luaVM, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    //printf("Got global\n");
    lua_pushnil(L);
    while(lua_next(L,2) != 0)
    {
        //printf("Push Key and value on the stack\n");
        // uses 'key' (at index = -2) and 'value' (at index -1)
        if(lua_isnumber(L,-2))
             // Key is a number
            lua_pushnumber(taskq[tIndex].luaVM,lua_tonumber(L,-2));
        else
            // Key is a string
            lua_pushstring(taskq[tIndex].luaVM,lua_tostring(L,-2));
        // Now push the value
        if(lua_isnumber(L,-1))
            // Value is a number
            lua_pushnumber(taskq[tIndex].luaVM,lua_tonumber(L,-1));
        else if(lua_isstring(L,-1))
            // Value is a string
            lua_pushstring(taskq[tIndex].luaVM,lua_tostring(L,-1));
        else
            // Value is boolean
            lua_pushboolean(taskq[tIndex].luaVM,lua_toboolean(L,-1));
        // Set the key value pair in the task environment
        //printf("Add Key and Value on the global\n");
        lua_settable(taskq[tIndex].luaVM,-3);
        //printf("Added! Now pop the value\n");
        lua_pop(L,1);
        //printf("Popped now do the loop again\n");
    }
    //printf("All done now return\n");
    //lua_pop(taskq[tIndex].luaVM,1); // Pops the global table back
    // Return true
    lua_pushboolean(L,1);
    return 1;
}

// Function to kill the lua_state associated with the task
static int closeTask(lua_State *L)
{
	short tIndex;
	int numArgs = lua_gettop(L);
	if(numArgs < 1)
	{
		// Return message saying that a task index must be provided
		lua_pushnil(L);
		lua_pushstring(L,"Need the task index which has to be resumed");
		return 2;	// 2 results pushed on the stack
	}
	if(!lua_isnumber(L,1))
	{
		lua_pushnil(L);
		lua_pushstring(L,"1st argument should be a number indicating the task index.");
		return 2;	// 2 results pushed on the stack
	}
	tIndex = lua_tointeger(L,1);
	if(taskq[tIndex].luaVM)
    {
			lua_close( taskq[tIndex].luaVM );
            //printf("LuaVM closed");
			//taskq[tIndex].codeString = NULL;
			//taskq[tIndex].iniCodeString = NULL;
			taskq[tIndex].luaVM = NULL;
			taskq[tIndex].thread = NULL;
			taskq[tIndex].numSteps = 0;
			taskq[tIndex].finished = 0;
			taskq[tIndex].suspended = 0;
			lua_pushboolean(L,1);
			return 1;
    }
    lua_pushnil(L);
    return 1;
}

// Function to get the number of tasks in task List
static int getNumOfTasks(lua_State *L)
{
	//printf("getNumOfTasks\n");
	short i,totTasks=0,urgTasks=0;

	for(i=0;i<numTasks+urgentTasks;i++)
	{
	    if(i<numTasks)
        {
            if(taskq[i].luaVM)
                totTasks = totTasks + 1;
        }
        else
        {
            if(taskq[i].luaVM)
                urgTasks = urgTasks + 1;
        }
	}
	lua_pushinteger(L,totTasks);
	lua_pushinteger(L,urgTasks);
	return 2;
}	// static int getNumOfTasks(lua_State *L)

// Function to suspend a task
static int suspendTask(lua_State *L)
{
	short tIndex;
	int numArgs = lua_gettop(L);
	if(numArgs<1)
	{
		// Return message saying that a task index must be provided
		lua_pushnil(L);
		lua_pushstring(L,"Need the task index which has to be resumed");
		return 2;	// 2 results pushed on the stack
	}
	if(!lua_isnumber(L,1))
	{
		lua_pushnil(L);
		lua_pushstring(L,"1st argument should be a number indicating the task index.");
		return 2;	// 2 results pushed on the stack
	}
	tIndex = lua_tointeger(L,1);
	if(taskq[tIndex].luaVM && !taskq[tIndex].finished && !taskq[tIndex].suspended)
	{
	    taskq[tIndex].suspended = 1;
	    lua_pushboolean(L,1);
	    return 1;
	}
	else
	{
		lua_pushnil(L);
		lua_pushstring(L,"No task or already finished or already suspended.");
		return 2;	// 2 results pushed on the stack
	}
}   // static int suspendTask(lua_State *L) ends

// Function to resume a task
static int resumeTask(lua_State *L)
{
	short tIndex;
	int numArgs = lua_gettop(L);
	if(numArgs<1)
	{
		// Return message saying that a task index must be provided
		lua_pushnil(L);
		lua_pushstring(L,"Need the task index which has to be resumed");
		return 2;	// 2 results pushed on the stack
	}
	if(!lua_isnumber(L,1))
	{
		lua_pushnil(L);
		lua_pushstring(L,"1st argument should be a number indicating the task index.");
		return 2;	// 2 results pushed on the stack
	}
	tIndex = lua_tointeger(L,1);
	if(taskq[tIndex].luaVM && !taskq[tIndex].finished && taskq[tIndex].suspended)
	{
	    taskq[tIndex].suspended = 0;
	    lua_pushboolean(L,1);
	    return 1;
	}
	else
	{
		lua_pushnil(L);
		lua_pushstring(L,"No task or already finished or not suspended.");
		return 2;	// 2 results pushed on the stack
	}
}   // static int resumeTask(lua_State *L) ends

// Function to return the taskStatus
static int taskStatus(lua_State *L)
{
	//printf("taskStatus\n");
	short checkIndex;
	int numArgs = lua_gettop(L);
	if(numArgs<1)
	{
		// Return message saying need at least the script needed to add as Task
		lua_pushnil(L);
		lua_pushstring(L,"Need the task index for which to check the status");
		return 2;	// 2 results pushed on the stack
	}
	if(!lua_isnumber(L,1))
	{
		lua_pushnil(L);
		lua_pushstring(L,"1st argument should be a number indicating the task index.");
		return 2;	// 2 results pushed on the stack
	}
	checkIndex = lua_tointeger(L,1);
	if(taskq[checkIndex].luaVM && !taskq[checkIndex].finished)
	{
	    if(taskq[checkIndex].suspended)
        {
				lua_pushstring(L,"SUSPENDED");
				return 1;	// 1 results pushed on the stack
        }
		// This slot has a task. Check the thread status
		switch(lua_status(taskq[checkIndex].thread))
		{
			case LUA_OK:
			case LUA_YIELD:
				// Thread was not started or is in suspended mode
				lua_pushstring(L,"GOOD");
				return 1;	// 1 results pushed on the stack
				break;
			case LUA_ERRMEM:
				lua_pushstring(L,"NE-PREVERR");
				lua_pushstring(L,"LUA_ERRMEM");
				return 2;
				break;
			case LUA_ERRRUN:
				lua_pushstring(L,"NE-PREVERR");
				lua_pushstring(L,"LUA_ERRRUN");
				return 2;
		}
	}		// if(taskq[i].luaVM && !taskq[i].finished) else
	else
	{
		// Task not present or finished
		if(taskq[checkIndex].luaVM)
		{
			// Task is finished
			lua_pushstring(L,"NE-FIN");
			return 1;	// 1 results pushed on the stack
		}
	}		// if(taskq[i].luaVM && !taskq[i].finished) ends
	lua_pushnil(L);
	lua_pushstring(L,"No tasks exist");
	return 2;	// 2 results pushed on the stack
}	// static int taskStatus(lua_State *L) ends

static int runLoop(lua_State *L)
{
	//printf("runLoop\n");
	short i,statCount,k;
	int numLoops=1,j;
	struct taskStat
	{
		short index;
		const char* status;
		const char* msg;
	};
	int numArgs = lua_gettop(L);
	if(numArgs>0)
	{
	    if(!lua_isnumber(L,1))
        {
            lua_pushnil(L);
            lua_pushstring(L,"1st argument should be a number indicating the number of times to run the loop(default=1).");
            return 2;	// 2 results pushed on the stack
        }
		numLoops = lua_tointeger(L,1);
	}

    statCount = -1;
	struct taskStat Stats[numTasks+urgentTasks];	// To keep track of all entered Task Status in the structure
	for(j=0;j<numLoops;j++)
	{
		statCount = -1;		// To keep track of all entered Task Status in the structure
		for(i=0;i<(numTasks+urgentTasks);i++)
		{
			if(taskq[i].luaVM && !taskq[i].finished && !taskq[i].suspended)
			{
				// This slot has a task. Check the thread status
				switch(lua_status(taskq[i].thread))
				{
					case LUA_OK:
					case LUA_YIELD:
						// Thread was not started or is in suspended mode
						// Resume the thread
						//printf("Resume thread %d",i);
//						lua_getglobal(taskq[i].luaVM,"currco");
//						thread = lua_tothread(taskq[i].luaVM,-1);
//						if(thread)
//						{
//							printf("resuming coroutine here\n");
//							resret = lua_resume(thread,0);
//							printf("Return code: %d\n",resret);
//						}
//						else
//							resret = lua_resume(taskq[i].thread,0);
						switch(lua_resume(taskq[i].thread,NULL,0))
						{
							case LUA_OK:
								// Finished execution
								//printf("Finished execution\n");
								taskq[i].finished = 1;
								statCount = statCount + 1;
								Stats[statCount].index = i;
								Stats[statCount].status = "FIN";
								Stats[statCount].msg = NULL;
								break;
							case LUA_YIELD:
								//printf("Yield execution control in C\n");
								statCount = statCount + 1;
								Stats[statCount].index = i;
								Stats[statCount].status = "GOOD";
								Stats[statCount].msg = NULL;
								break;
							case LUA_ERRMEM:
							case LUA_ERRRUN:
								statCount = statCount + 1;
								Stats[statCount].index = i;
								Stats[statCount].status = "ERROR";
                                Stats[statCount].msg = lua_tostring(taskq[i].thread,-1);
								break;
						}
						// toggle the getswitchstatus variable in lstep table of the task
                        lua_pushlightuserdata(taskq[i].luaVM,hookFunc); // pointer to hookFunc is the key in the registry to the table __ls_
                        lua_gettable(taskq[i].luaVM,LUA_REGISTRYINDEX);

                        // Now put the variable name 2 times on the stack
                        lua_pushstring(taskq[i].luaVM,"getswitchstatus");
                        lua_pushstring(taskq[i].luaVM,"getswitchstatus");
                        // Get the value of lstep.getswitchstatus
                        lua_gettable(taskq[i].luaVM,-3);    // This places the getswitchstatus value in place of the top of teh stack "getswitchstatus"
                        k = lua_tonumber(taskq[i].luaVM,-1);
                        k = k * (-1);
                        lua_pop(taskq[i].luaVM,1);
                        lua_pushnumber(taskq[i].luaVM,k);
                        lua_settable(taskq[i].luaVM,-3);
                        // Now pop the table to balance the stack
                        lua_pop(taskq[i].luaVM,1);
						break;
					case LUA_ERRMEM:
						// Thread not run due to previous error
						statCount = statCount + 1;
						Stats[statCount].index = i;
						Stats[statCount].status = "NE-PREVERR";
						Stats[statCount].msg = "LUA_ERRMEM";
						break;
					case LUA_ERRRUN:
						// Thread not run due to previous error
						statCount = statCount + 1;
						Stats[statCount].index = i;
						Stats[statCount].status = "NE-PREVERR";
						Stats[statCount].msg = "LUA_ERRRUN";
						break;
				}
			}		// if(taskq[i].luaVM && !taskq[i].finished) else
			else
			{
				// Task not run
				if(taskq[i].luaVM && taskq[i].finished)
				{
					// Task is finished and not run
					statCount = statCount + 1;
					Stats[statCount].index = i;
					Stats[statCount].status = "NE-FIN";
					Stats[statCount].msg = NULL;
				}
				else if(taskq[i].luaVM && taskq[i].suspended)
				{
					// Task is finished and not run
					statCount = statCount + 1;
					Stats[statCount].index = i;
					Stats[statCount].status = "NE-SUSPEND";
					Stats[statCount].msg = NULL;
				}
			}		// if(taskq[i].luaVM && !taskq[i].finished) ends
		}		// for(i=0;i<numTasks;i++) ends
	}	// for(j=0;j<numLoops;j++)	ends

	if(statCount == -1)
	{
		lua_pushnil(L);
		return 1;
	}
	// Now create and return the status table for all tasks
	lua_newtable(L);

	for(i=0;i<=statCount;i++)
	{
		lua_pushinteger(L,Stats[i].index);
		// Create the status table for the task status
		lua_newtable(L);
		// Put the status on index 1
		lua_pushinteger(L,1);
		lua_pushstring(L,Stats[i].status);
		lua_settable(L,-3);
		if(Stats[i].msg)
		{
			// Add the msg string as well on index 2
			lua_pushinteger(L,2);
			lua_pushstring(L,Stats[i].msg);
			lua_settable(L,-3);
		}
		// Put this table in the main status table
		lua_settable(L,-3);
	}
	return 1;
}	// static int runLoop(lua_State *L)

/*
// Removed from version 1.14.07.12
// Function to return the Version string of LuaStepper
static int version(lua_State *L)
{
    lua_pushstring(L,VERSION);
    return 1;
}
*/

// Library entry function
int _EXPORT luaopen_LuaStepper(lua_State *L)
{
	static const struct luaL_Reg funcs[] = {
			{"addTask",addTask},
			{"runLoop",runLoop},
			{"taskStatus",taskStatus},
			{"getNumOfTasks",getNumOfTasks},
			{"closeTask",closeTask},
			{"suspendTask",suspendTask},
			{"resumeTask",resumeTask},
			{"getTaskData",getTaskData},
			{"setTaskData",setTaskData},
			{"setTaskTable",setTaskTable},
            {"registerCallBack",registerCallBack},
			//{"version",version},  // Removed version 1.14.07.12
			{"runCode",runCode},
			{NULL,NULL}
	};
	// Initialize the task List
	short i;
	for(i=0;i<(numTasks+urgentTasks);i++)
	{
		//taskq[i].codeString = NULL;
		//taskq[i].iniCodeString = NULL;
		taskq[i].thread = NULL;
		taskq[i].luaVM = NULL;
		taskq[i].numSteps = 0;
		taskq[i].finished = 0;
		taskq[i].suspended = 0;
	}
	luaL_newlib(L, funcs); // Just returns the module as a table
	// Set the _VERSION number
	lua_pushstring(L,"_VERSION");
	lua_pushstring(L,VERSION);
	lua_rawset(L,-3);
	return 1;
}	// function luaopen_LuaStepper ends here
