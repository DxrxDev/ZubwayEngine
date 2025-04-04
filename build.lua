#! /usr/bin/lua

buildTargets = {}

if #arg == 0 then
	buildTargets[1] = "all"
end

for i = 1, #arg do
	buildTargets[i] = arg[i]
end

platform = {
	invalid = -1,
	linux = 0,
	windows = 1,
	macos = 2
}
CURRENT_PLATFORM = platform.linux

warngings = {
	none = 0,
	all = 1
}

function CreateProject( name, outdir, type )
	if CURRENT_PLATFORM ~= platform.linux then
		os.exit(-1)
	end

	if type:lower() == "app" then
		return {
			name = name,
			outdir = outdir,
			program = "g++",
			special = ""
		}
	elseif type:lower() == "inter" then
		return {
			name = name,
			outdir = outdir,
			program = "g++",
			special = "i"
		}
	elseif type:lower() == "shader" then
		return {
			name = name,
			outdir = outdir,
			program = "glslc",
			special = "s"
		}
	end

	print (
		"INVALID PROJECT TYPE\n" ..
		"Project name: " .. name .. "\n" ..
		"Project type \"" .. type .. " doesn't exist!\n"
	)
	os.exit(-1)
end

function CreateCommandApp( proj )
	ret = proj.program .. " -o" .. proj.outdir .. "/" .. proj.name .. " "
	
	numFiles = #proj.files
	for i = 1, numFiles do
		ret = ret .. proj.files[i] .. " "
	end

	if proj.files == nil or proj.files == {} then
		return {ret}
	end
	numLibraries = #proj.libraries
	for i = 1, numLibraries do
		ret = ret .. "-l" .. proj.libraries[i] .. " "
	end

	return {ret}
end

function CreateCommandInter( proj )
	ret = {}
	
	if proj.files == nil or proj.files == {} then
		assert(nil, "Lua assert line 85")
	end
	for i = 1, #proj.files do
		ret[i] = 
			proj.program .. " -o" .. 
			proj.outdir .. "/" .. 
			proj.files[i] .. ".o -c " ..
			proj.files[i] .. " "

		if CURRENT_PLATFORM == platform.linux then
			ret[i] = ret[i] .. "-D_PLATFORM_LINUX "
		end

		if proj.warngings ~= nil then
			if proj.warngings == warngings.all then
				ret[i] = ret[i] .. " -Wall "
			end
		end

		if proj.includedirs == nil or proj.includedirs == {} then
		else
			for j = 1, #proj.includedirs do
				ret[i] = ret[i] .. "-I" .. proj.includedirs[j] .. " "
			end
		end
	end

	return ret
end

function CreateCommandShader( proj )
	ret = proj.program .. " -o" .. proj.outdir .. "/" .. proj.name .. ".spirv "

	numFiles = #proj.files
	for i = 1, numFiles do
		ret = ret .. proj.files[i] .. " "
	end

	return {ret}
end

function CreateCommand( proj )
	if proj.outdir == nil then
		proj.outdir = ""
	end

	if proj.special == "" then
		return CreateCommandApp(proj)
	elseif proj.special == "i" then
		return CreateCommandInter(proj)
	elseif proj.special == "s" then
		return CreateCommandShader(proj)
	end

	assert(nil, "Failed creating command on line 137")
end

function RunProject( proj )
	run = CreateCommand( proj )
	for i = 1, #run do
		print( "Running command \""..run[i].."\"" )
		os.execute( run[i] )
	end
end

function getobjs(proj)
	ret = {}
	for i = 1, #proj.files do
		ret[i] = proj.outdir .. "/" .. proj.files[i] .. ".o"
	end
	return ret
end

print "Starting build process..."

wingfx = CreateProject( "wingfx", "out/int", "inter" )
wingfx.files ={
	"ZubwayEngine/window.cpp",
	"ZubwayEngine/graphics.cpp",
	"ZubwayEngine/box2D.cpp",
	"ZubwayEngine/raymath.cpp",
	"ZubwayEngine/cute_png.cpp"
}
wingfx.warngings = warngings.all

app = CreateProject( "app", "out/int", "inter" )
app.files = {
	"App/pong.cpp",
}
app.includedirs = {
	"ZubwayEngine"
}
app.warngings = warngings.all

game = CreateProject( "StonesToBridges", "out", "app" )
game.files = {
	"out/int/App/pong.cpp.o"
}

listofobjs = getobjs(wingfx)
offset = #game.files
for i = 1, #listofobjs do
	game.files[i+offset] = listofobjs[i]
end

for i = 1, #game.files do
	if game.files[i] == nil then
		print("OBJ FILE: NIL")
	else
		print("OBJ FILE: " .. game.files[i])
	end
end

game.libraries = {
	"dl", "m", "xcb", "xcb-icccm", "xcb-keysyms", "vulkan"
}
game.warngings = warngings.all

RunProject(wingfx)
RunProject(app)
RunProject(game)