#include "Plugin.h"
#include "engine/core/util/PathUtil.h"
#include "engine/core/log/Log.h"

#ifdef ECHO_PLATFORM_WINDOWS
#define DYNLIB_HANDLE         HMODULE
#define DYNLIB_LOAD( a)       LoadLibraryEx( a, NULL, LOAD_WITH_ALTERED_SEARCH_PATH)
#define DYNLIB_GETSYM( a, b)  GetProcAddress( a, b)
#define DYNLIB_UNLOAD( a)     FreeLibrary( a)
#else
#include <dlfcn.h>
#define DYNLIB_HANDLE                       void*
#define DYNLIB_LOAD( a)                     dlopen( a, RTLD_LOCAL|RTLD_LAZY)
#define DYNLIB_GETSYM( handle, symbolName)  dlsym( handle, symbolName)
#define DYNLIB_UNLOAD( a)                   dlclose( a)
#endif

namespace Echo
{
	// plugins
	static map<String, Plugin*>::type	g_plugins;

	Plugin::Plugin()
	{

	}

	Plugin::~Plugin()
	{

	}

	void Plugin::load(const char* path)
	{
		m_handle = DYNLIB_LOAD(path);
	}

	void Plugin::unload()
	{
		DYNLIB_UNLOAD(m_handle);
	}

	void* Plugin::getSymbol(const char* symbolName)
	{
		return (void*)DYNLIB_GETSYM(m_handle, symbolName);
	}

	void Plugin::loadAllPlugins()
	{
#ifdef ECHO_EDITOR_MODE
		typedef bool(*LOAD_PLUGIN_FUN)();

		// get plugin path
		String pluginDir = PathUtil::GetCurrentDir() + "/plugins";
		PathUtil::FormatPath(pluginDir, false);

		// get all plugins
		StringArray plugins;
		PathUtil::EnumFilesInDir( plugins, pluginDir, false, false, true);

		// iterate
		for (String& pluginPath : plugins)
		{
			String ext = PathUtil::GetFileExt(pluginPath, true);
			if(ext==".dll" || ext==".dylib")
			{
                String name = StringUtil::Replace(PathUtil::GetPureFilename(pluginPath, false), "lib", "");
				String symbolName = StringUtil::Format("load%sPlugin", name.c_str());

				Plugin* plugin = EchoNew(Plugin);
				plugin->load(pluginPath.c_str());
				LOAD_PLUGIN_FUN pFunc = (LOAD_PLUGIN_FUN)plugin->getSymbol(symbolName.c_str());
				if (pFunc)
				{
					(*pFunc)();
				}
				else
				{
					EchoLogError("Can't find symbol %s in plugin [%s]", symbolName.c_str(), pluginPath.c_str());
				}

				g_plugins[name] = plugin;
			}
		}
#endif
	}
}
