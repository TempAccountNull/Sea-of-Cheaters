#include "cheat.h"


#include <iostream>
#include <wincred.h>
#include <HookLib/HookLib.h>

LSTATUS hook_RegOpenKeyExA(
	HKEY   hKey,
	LPCSTR lpSubKey,
	DWORD  ulOptions,
	REGSAM samDesired,
	PHKEY  phkResult
);
decltype(hook_RegOpenKeyExA)* orignal_RegOpenKeyExA = nullptr;
LSTATUS hook_RegOpenKeyExA(
	HKEY   hKey,
	LPCSTR lpSubKey,
	DWORD  ulOptions,
	REGSAM samDesired,
	PHKEY  phkResult
)
{	
	std::cout << "RegOpenKeyExA -> " << lpSubKey << '\n';

	return orignal_RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}


LSTATUS hook_RegOpenKeyExW(
	HKEY   hKey,
	LPCWSTR lpSubKey,
	DWORD  ulOptions,
	REGSAM samDesired,
	PHKEY  phkResult
);
decltype(hook_RegOpenKeyExW)* orignal_RegOpenKeyExW = nullptr;
LSTATUS hook_RegOpenKeyExW(
	HKEY   hKey,
	LPCWSTR lpSubKey,
	DWORD  ulOptions,
	REGSAM samDesired,
	PHKEY  phkResult
)
{	
	std::wcout << "RegOpenKeyExW -> " << lpSubKey << '\n';
	
	return orignal_RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}



LSTATUS hook_RegOpenKeyA(
	HKEY   hKey,
	LPCSTR lpSubKey,
	PHKEY  phkResult
);
decltype(hook_RegOpenKeyA)* orignal_RegOpenKeyA = nullptr;
LSTATUS hook_RegOpenKeyA(
	HKEY   hKey,
	LPCSTR lpSubKey,
	PHKEY  phkResult
)
{
	std::cout << "RegOpenKeyA -> " << lpSubKey << '\n';

	return orignal_RegOpenKeyA(hKey, lpSubKey, phkResult);
}

LSTATUS hook_RegOpenKeyW(
	HKEY   hKey,
	LPCWSTR lpSubKey,
	PHKEY  phkResult
);
decltype(hook_RegOpenKeyW)* orignal_RegOpenKeyW = nullptr;
LSTATUS hook_RegOpenKeyW(
	HKEY   hKey,
	LPCWSTR lpSubKey,
	PHKEY  phkResult
)
{
	std::wcout << "RegOpenKeyW -> " << lpSubKey << '\n';

	return orignal_RegOpenKeyW(hKey, lpSubKey, phkResult);
}


BOOL hook_CredEnumerateA(
	LPCSTR       Filter,
	DWORD        Flags,
	DWORD* Count,
	PCREDENTIALA** Credential
);
decltype(hook_CredEnumerateA)* original_CredEnumerateA = nullptr;
BOOL hook_CredEnumerateA(
	LPCSTR       Filter,
	DWORD        Flags,
	DWORD* Count,
	PCREDENTIALA** Credential
)
{
	std::cout << "CredEnumerateA -> " << Filter << '\n';

	return false;

	return original_CredEnumerateA(Filter, Flags, Count, Credential);
}

BOOL hook_CredEnumerateW(
	LPCWSTR       Filter,
	DWORD        Flags,
	DWORD* Count,
	PCREDENTIALW** Credential
);
decltype(hook_CredEnumerateW)* original_CredEnumerateW = nullptr;
BOOL hook_CredEnumerateW(
	LPCWSTR       Filter,
	DWORD        Flags,
	DWORD* Count,
	PCREDENTIALW** Credential
)
{
	std::wcout << "CredEnumerateW -> " << Filter<< '\n';

	return false;

	return original_CredEnumerateW(Filter, Flags, Count, Credential);
}


BOOL hook_CredReadA(
	LPCSTR      TargetName,
	DWORD        Type,
	DWORD        Flags,
	PCREDENTIAL* Credential
);
decltype(hook_CredReadA)* original_CredReadA = nullptr;
BOOL hook_CredReadA(
	LPCSTR      TargetName,
	DWORD        Type,
	DWORD        Flags,
	PCREDENTIALA* Credential
)
{
	std::cout << "CredReadA -> " << (*Credential)->TargetName << '\n';

	return original_CredReadA(TargetName, Type, Flags, Credential);
}

BOOL hook_CredReadW(
	LPCWSTR      TargetName,
	DWORD        Type,
	DWORD        Flags,
	PCREDENTIALW* Credential
);
decltype(hook_CredReadW)* original_CredReadW = nullptr;
BOOL hook_CredReadW(
	LPCWSTR      TargetName,
	DWORD        Type,
	DWORD        Flags,
	PCREDENTIALW* Credential
)
{
	std::wcout << "CredReadW -> " << (*Credential)->TargetName << '\n';

	return original_CredReadW(TargetName, Type, Flags, Credential);
}


void install_hooks()
{
	printf("Hook RegOpenKeyExA: %d\n", SetHook(RegOpenKeyExA, hook_RegOpenKeyExA, reinterpret_cast<void**>(&orignal_RegOpenKeyExA)));
	printf("Hook RegOpenKeyExW: %d\n", SetHook(RegOpenKeyExW, hook_RegOpenKeyExW, reinterpret_cast<void**>(&orignal_RegOpenKeyExW)));

	printf("Hook RegOpenKeyA: %d\n", SetHook(RegOpenKeyA, hook_RegOpenKeyA, reinterpret_cast<void**>(&orignal_RegOpenKeyA)));
	printf("Hook RegOpenKeyW: %d\n", SetHook(RegOpenKeyW, hook_RegOpenKeyW, reinterpret_cast<void**>(&orignal_RegOpenKeyW)));

	printf("Hook CredEnumerateA: %d\n", SetHook(CredEnumerateA, hook_CredEnumerateA, reinterpret_cast<void**>(&original_CredEnumerateA)));
	printf("Hook CredEnumerateW: %d\n", SetHook(CredEnumerateW, hook_CredEnumerateW, reinterpret_cast<void**>(&original_CredEnumerateW)));

	printf("Hook CredReadA: %d\n", SetHook(CredReadA, hook_CredReadA, reinterpret_cast<void**>(&original_CredReadA)));
	printf("Hook CredReadW: %d\n", SetHook(CredReadW, hook_CredReadW, reinterpret_cast<void**>(&original_CredReadW)));
}

void uninstall_hooks()
{
	RemoveHook(RegOpenKeyExA);
	RemoveHook(RegOpenKeyExW);

	RemoveHook(RegOpenKeyA);
	RemoveHook(RegOpenKeyW);

	RemoveHook(CredEnumerateA);
	RemoveHook(CredEnumerateW);
	
	RemoveHook(CredReadA);
	RemoveHook(CredReadW);
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) 
{
	switch (fdwReason) 
	{
	case DLL_PROCESS_ATTACH: 
	{
		DisableThreadLibraryCalls(hinstDLL);
				
		if (!Cheat::Init(hinstDLL))
		{			
			return FALSE;
		}

		//install_hooks();

		break;
	}
	case DLL_PROCESS_DETACH: 
	{
		//uninstall_hooks();

		return Cheat::Remove();
	}
	}
	
	return TRUE;
}