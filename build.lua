-- x, bob = pcall(require, "bob")

--[[
for k,v in pairs(arg) do
	print( "key "..k..", value "..v )
end

if arg[1] == "--help" then
	print "yeah"	
end
]]

platform = {
	invalid = -1,
	linux = 0,
	windows = 1,
	macos = 2
}

warngings = {
	none = 0,
	all = 1
}

function CreateProject( name, outdir, lang, plat )
	if lang:lower() ~= "c" or plat ~= platform.linux then
		print ( "Build system only supports C on Linux" )
		os.exit( -1 )
	end

	return {
		name = name,
		outdir = outdir,
		program = "g++",
		platform = platform.linux
	}
end

function CreateCommand( proj )
	if proj.outdir == nil then
		proj.outdir = ""
	end

	ret = proj.program .. " -o" .. proj.outdir .. "/" .. proj.name .. " "

	if proj.platform == platform.linux then
		ret = ret .. "-D_PLATFORM_LINUX "
	end

	numFiles = #proj.files
	for i = 1, numFiles do
		ret = ret .. proj.files[i] .. " "
	end

	numIncDirs = #proj.includedirs
	for i = 1, numIncDirs do
		ret = ret .. "-I" .. proj.includedirs[i] .. " "
	end

	numLibraries = #proj.libraries
	for i = 1, numLibraries do
		ret = ret .. "-l" .. proj.libraries[i] .. " "
	end

	if proj.warngings ~= nil then
		if proj.warngings == warngings.all then
			ret = ret .. " -Wall "
		end
	end

	return ret
end

print "Starting build process..."

game = CreateProject( "app", "out", "c", platform.linux )
game.files = {
	"App/pong.cpp", "ZubwayEngine/window.cpp", "ZubwayEngine/graphics.cpp", "ZubwayEngine/raymath.cpp", "ZubwayEngine/box2D.cpp"
}
game.includedirs = {
	"ZubwayEngine"
}
game.libraries = {
	"dl", "m", "xcb", "xcb-icccm", "xcb-keysyms", "vulkan"
}
game.warngings = warngings.all

run = CreateCommand(game)

print( "Running command \""..run.."\"" )
os.execute( run )

