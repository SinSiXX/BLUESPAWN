#pragma once

#include "hooking/hooks/ThreadCreation.h"

namespace BLUESPAWN::Agent::ThreadCreation{

	CreateRemoteThread_t _CreateRemoteThread{ nullptr };

	HANDLE WINAPI CreateRemoteThreadHook(
		HANDLE                 hProcess,
		LPSECURITY_ATTRIBUTES  lpThreadAttributes,
		SIZE_T                 dwStackSize,
		LPTHREAD_START_ROUTINE lpStartAddress,
		LPVOID                 lpParameter,
		DWORD                  dwCreationFlags,
		LPDWORD                lpThreadId
	){
		DWORD dwPID{ GetProcessId(hProcess) };
		LOG_DEBUG_MESSAGE(LOG_INFO, L"Detected CreateRemoteThread into process " << dwPID
						  << " (Address: " << lpStartAddress << L")");

		std::vector<Address> addresses{};
		auto status{ WalkStack(addresses) };
		if(!status){
			LOG_DEBUG_MESSAGE(LOG_ERROR, L"Failed to walk the stack in detected CreateRemoteThread!");
		}

		bool parent = false;
		for(DWORD dwParent = RequestParentPID(dwPID); dwParent; dwParent = RequestParentPID(dwParent)){
			if(RequestParentPID(dwPID) == GetCurrentProcessId()){
				parent = true;
			}
		}

		std::vector<Argument> args{
			Argument{ Value::Handle(hProcess) },
			Argument{ Value::Struct(lpThreadAttributes, lpThreadAttributes->nLength) },
			Argument{ Value::Number(dwStackSize) },
			Argument{ Value::Pointer(lpStartAddress) },
			Argument{ Value::Pointer(lpParameter) },
			Argument{ Value::Number(dwCreationFlags) },
			Argument{ Value::OutPointer(lpThreadId, false) },
		};

		Call call{ addresses, args };
		if(!parent){
			RecordCall(call, CallAction::Allowed);
			if(_CreateRemoteThread){
				return _CreateRemoteThread(hProcess, lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, 
										   dwCreationFlags, lpThreadId);
			}
		} else{
			RecordCall(call, CallAction::Blocked);
			SetLastError(ERROR_ACCESS_DENIED);
			return INVALID_HANDLE_VALUE;
		}
	}

	namespace{
		HookRegistration __CreateRemoteThread{ L"kernel32.dll", "CreateRemoteThread", CreateRemoteThreadHook,
											   reinterpret_cast<LPVOID*>(&_CreateRemoteThread) };
	}
}