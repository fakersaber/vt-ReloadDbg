#pragma once

/*
	���� Intel VT
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS 
WINAPI
EnableIntelVT();

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS 
WINAPI
DisableIntelVT();

/*
	����VT������֧��
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
WINAPI
CheckHvSupported();

/*
	���Ӳ������֧��
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS 
WINAPI
CheckHvHardwareSupported();

/*
	���ϵͳ����֧��
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
WINAPI
CheckHvOsSupported();

/*
	����HV����Ҫ���ڴ�
*/
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
WINAPI
InitlizetionIntelHvContext(
	_In_opt_ PVOID Args
);

/*
	��ʼ��VMX����Ҫ���ڴ�ṹ
*/
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
WINAPI
InitiationVmxContext();

/*
	��ʽ����HV
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
NTSTATUS 
WINAPI
RunIntelHyperV();

_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
NTSTATUS
WINAPI
UnRunIntelHyperV();

/*
	��ʼ�� VMCS
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
NTSTATUS 
WINAPI
InitiationVmCsContext();

/*
	��ʼ�� MSR
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
NTSTATUS 
WINAPI
InitiationMsrContext();

/*
	��ʼ�������� VMCS ����
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
NTSTATUS
WINAPI
ExecuteVmxOn();

/*
	���� Intel VT
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
NTSTATUS
WINAPI
ExecuteVmlaunch();

/*
	���� VMX
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS 
WINAPI
ClearVmxContext();

/*
	��ֹHV��ʼ��
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
WINAPI
StopHvInitlizetion();

_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
WINAPI
VmxCsWrite(
	_In_ ULONG64 target_field, 
	_In_ ULONG64 value
);

_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG64
WINAPI
VmxCsRead(
	_In_ ULONG64 target_field
);

ULONG
WINAPI 
VmxAdjustControlValue(
	_In_ ULONG Msr, 
	_In_ ULONG Ctl
);

HvContextEntry*
WINAPI
GetHvContextEntry();

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
WINAPI
HvRestoreRegisters();

BOOLEAN
WINAPI
HvVmCall(
	_In_ ULONG CallNumber,	/*���*/
	_In_ ULONG64 arg1 = 0, 
	_In_ ULONG64 arg2 = 0, 
	_In_ ULONG64 arg3 = 0
);